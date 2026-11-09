// Phase 4 slice B (pro-4.0.0): AST-based symbol index for one SP document.
// See SymbolIndex.h for the design note.

#include "SymbolIndex.h"

#include "sp/ast/ASTNode.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/lexer/Token.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/sema/Type.h"

#include <algorithm>
#include <cctype>

namespace sp::lsp {

namespace {

bool isWordChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string joinParams(const std::vector<ast::Parameter>& params) {
    std::string out;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i) out += ", ";
        out += params[i].getName();
        if (!params[i].getType().empty()) {
            out += ": " + params[i].getType();
        }
    }
    return out;
}

} // namespace

bool SymbolIndex::build(const std::string& source, const std::string& uri) {
    m_valid = false;
    m_uri = uri;
    m_symbols.clear();
    m_uses.clear();
    m_scopes.clear();
    m_letHints.clear();

    m_lines.clear();
    {
        size_t start = 0;
        while (true) {
            size_t nl = source.find('\n', start);
            if (nl == std::string::npos) {
                m_lines.push_back(source.substr(start));
                break;
            }
            m_lines.push_back(source.substr(start, nl - start));
            start = nl + 1;
        }
    }

    m_diagStream.str("");
    m_diagStream.clear();
    m_diagEngine = std::make_unique<diagnostics::DiagnosticEngine>(m_diagStream);

    lexer::Lexer lexer(source, uri, m_diagEngine.get());
    auto tokens = lexer.tokenizeAll();
    if (m_diagEngine->hasErrors()) return false;

    parser::Parser parser(tokens, uri, m_diagEngine.get());
    m_program = parser.parseProgram();
    if (!m_program) return false;

    m_sema.reset();
    // Phase 4 slice B: do NOT run sema on a program with parse errors.
    // The parser's error recovery leaves null children in the AST, and
    // TypeChecker dereferences them (e.g. visit(IfStmt) -> checkExpr on a
    // null condition segfaults). spc never hits this because it bails on
    // parse errors before sema; the index must do the same. Without sema
    // the index falls back to structural inference (literals, new, calls),
    // so hover/inlay hints keep working on broken code — only sema-only
    // types are missing until the error is fixed. Diagnostics are
    // reported by computeDiagnostics, not here.
    if (!m_diagEngine->hasErrors()) {
        m_sema = std::make_unique<sema::SemanticAnalyzer>(*m_diagEngine);
        m_sema->analyze(*m_program);
    }

    int totalLines = static_cast<int>(m_lines.size());
    int topScope = addScope(-1, 1, totalLines);
    walkDecls(m_program->getDeclarations(), topScope);
    resolveAllUses();

    m_valid = true;
    return true;
}

int SymbolIndex::addScope(int parent, int startLine, int endLine) {
    Scope s;
    s.parent = parent;
    s.startLine = std::max(1, startLine);
    s.endLine = std::max(s.startLine, endLine);
    m_scopes.push_back(s);
    return static_cast<int>(m_scopes.size()) - 1;
}

int SymbolIndex::addSymbol(Symbol sym) {
    m_symbols.push_back(std::move(sym));
    return static_cast<int>(m_symbols.size()) - 1;
}

std::string SymbolIndex::signatureOf(const ast::FunctionDecl& fn, bool isMethod) const {
    std::string sig;
    if (isMethod) sig += "fn ";
    else sig += "fn ";
    sig += fn.getName() + "(" + joinParams(fn.getParams()) + ")";
    if (!fn.getReturnType().empty()) {
        sig += " -> " + fn.getReturnType();
    }
    return sig;
}

std::string SymbolIndex::inferType(const ast::ExpressionNode* expr, int scopeId) const {
    if (!expr) return "";
    // 1. Sema-inferred type (the authoritative source when available).
    // Phase 4 slice B: getNodeType() returns the Error type for nodes sema
    // never visited (e.g. in a program with parse errors), whose toString()
    // is "<type-error>". Never surface that as a hint — fall through to
    // structural inference instead. (The old string comparison checked for
    // "error" but the real spelling is "<type-error>", so it leaked.)
    if (m_sema) {
        auto t = m_sema->getNodeType(expr);
        if (t && !t->isError() && !t->isUnknown()) {
            std::string s = t->toString();
            if (!s.empty() && s != "any") return s;
            // "any" is still better than nothing for a call result we
            // cannot pin down; fall through to structural inference.
        }
    }
    // 2. Structural inference (works even when sema bailed out).
    switch (expr->getKind()) {
        case ast::ASTNodeKind::LiteralExpr: {
            const auto& lit = static_cast<const ast::LiteralExpr&>(*expr);
            switch (lit.getLiteralKind()) {
                case ast::LiteralKind::Integer: return "int";
                case ast::LiteralKind::Float: return "float";
                case ast::LiteralKind::String: return "string";
                case ast::LiteralKind::Char: return "char";
                case ast::LiteralKind::Boolean: return "bool";
                default: return "";
            }
        }
        case ast::ASTNodeKind::ArrayExpr: return "Array";
        case ast::ASTNodeKind::MapExpr: return "Map";
        case ast::ASTNodeKind::NewExpr:
            return static_cast<const ast::NewExpr&>(*expr).getTypeName();
        case ast::ASTNodeKind::CallExpr: {
            const auto& call = static_cast<const ast::CallExpr&>(*expr);
            const auto* callee = &call.getCallee();
            if (callee->getKind() == ast::ASTNodeKind::IdentifierExpr) {
                const std::string& name = static_cast<const ast::IdentifierExpr&>(*callee).getName();
                if (const Symbol* fn = findTopLevel(name)) {
                    return fn->type;
                }
            }
            if (callee->getKind() == ast::ASTNodeKind::MemberExpr) {
                const auto& mem = static_cast<const ast::MemberExpr&>(*callee);
                Use memberUse;
                memberUse.name = mem.getMember();
                memberUse.line = static_cast<int>(expr->getLocation().line);
                memberUse.scopeId = scopeId;
                memberUse.isMember = true;
                memberUse.receiver = &mem.getObject();
                if (const Symbol* m = resolveMemberUse(memberUse)) {
                    return m->type;
                }
            }
            return "";
        }
        default: return "";
    }
}

int SymbolIndex::findBlockEnd(int startLine0) const {
    // Find the line (0-based) of the '}' matching the first '{' at or
    // after startLine0. Strings, char literals and comments are skipped
    // so braces inside them do not count. -1 when not found.
    int total = static_cast<int>(m_lines.size());
    int depth = 0;
    bool seenOpen = false;
    for (int l = startLine0; l < total; ++l) {
        const std::string& line = m_lines[l];
        bool inStr = false, inChr = false, inLineComment = false, inBlockComment = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (inLineComment) break;
            if (inBlockComment) {
                if (c == '*' && i + 1 < line.size() && line[i + 1] == '/') {
                    inBlockComment = false;
                    ++i;
                }
                continue;
            }
            if (inStr) {
                if (c == '\\') { ++i; continue; }
                if (c == '"') inStr = false;
                continue;
            }
            if (inChr) {
                if (c == '\\') { ++i; continue; }
                if (c == '\'') inChr = false;
                continue;
            }
            if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                inLineComment = true;
                break;
            }
            if (c == '/' && i + 1 < line.size() && line[i + 1] == '*') {
                inBlockComment = true;
                ++i;
                continue;
            }
            if (c == '"') { inStr = true; continue; }
            if (c == '\'') { inChr = true; continue; }
            if (c == '{') {
                depth++;
                seenOpen = true;
            } else if (c == '}') {
                depth--;
                if (seenOpen && depth == 0) return l;
            }
        }
    }
    return -1;
}

void SymbolIndex::fixNameCol(Symbol& sym) const {
    // AST declaration locations point at the keyword (`fn`, `class`,
    // `let`, ...), not at the name. Without this, hover/definition
    // ranges cover the keyword and resolveAt misses the name itself.
    int c = findWordCol(sym.line - 1, sym.name, sym.col - 1);
    if (c >= 0) sym.col = c + 1;
}

std::string SymbolIndex::docCommentAbove(int declLine1) const {
    // Collect consecutive `///` lines directly above the declaration.
    std::vector<std::string> lines;
    for (int l = declLine1 - 2; l >= 0; --l) {
        const std::string& raw = m_lines[l];
        size_t i = 0;
        while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t')) ++i;
        if (raw.compare(i, 3, "///") != 0) break;
        std::string text = raw.substr(i + 3);
        if (!text.empty() && text[0] == ' ') text.erase(0, 1);
        lines.push_back(text);
    }
    std::string out;
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        if (!out.empty()) out += "\n";
        out += *it;
    }
    return out;
}

int SymbolIndex::findWordCol(int line0, const std::string& word, int fromCol0) const {
    if (line0 < 0 || line0 >= static_cast<int>(m_lines.size()) || word.empty()) return -1;
    const std::string& line = m_lines[line0];
    for (size_t i = static_cast<size_t>(std::max(0, fromCol0)); i + word.size() <= line.size(); ++i) {
        if (line.compare(i, word.size(), word) != 0) continue;
        bool beforeOk = (i == 0) || !isWordChar(line[i - 1]);
        bool afterOk = (i + word.size() >= line.size()) || !isWordChar(line[i + word.size()]);
        if (beforeOk && afterOk) return static_cast<int>(i);
    }
    return -1;
}

void SymbolIndex::walkFunction(const ast::FunctionDecl& fn, int scopeId, const std::string& parentClass, bool isMethod) {
    Symbol sym;
    sym.name = fn.getName();
    sym.kind = isMethod ? SymKind::Method : SymKind::Function;
    sym.type = fn.getReturnType();
    sym.signature = signatureOf(fn, isMethod);
    sym.line = static_cast<int>(fn.getLocation().line);
    sym.col = static_cast<int>(fn.getLocation().column);
    sym.scopeId = scopeId;
    sym.parent = parentClass;
    fixNameCol(sym);
    sym.docComment = docCommentAbove(sym.line);
    int endLine = findBlockEnd(sym.line - 1);
    sym.endLine = (endLine >= 0) ? endLine + 1 : sym.line;
    int symIdx = addSymbol(std::move(sym));

    int fnScope = addScope(scopeId, static_cast<int>(fn.getLocation().line), m_symbols[symIdx].endLine);
    for (const auto& param : fn.getParams()) {
        Symbol p;
        p.name = param.getName();
        p.kind = SymKind::Parameter;
        p.type = param.getType();
        p.signature = param.getName() + (param.getType().empty() ? "" : ": " + param.getType());
        // Parameters share the fn decl line; find the name on that line.
        int c = findWordCol(sym.line - 1, p.name, sym.col);
        p.line = sym.line;
        p.col = (c >= 0) ? c + 1 : sym.col;
        p.endLine = sym.line;
        p.scopeId = fnScope;
        p.parent = parentClass;
        addSymbol(std::move(p));
    }
    if (fn.hasBody() && fn.getBody()) {
        walkBlock(*fn.getBody(), fnScope);
    }
    if (fn.getShortBody()) {
        walkExpr(*fn.getShortBody(), fnScope);
    }
}

void SymbolIndex::walkDecls(const std::vector<std::unique_ptr<ast::ASTNode>>& decls, int scopeId) {
    for (const auto& d : decls) {
        if (!d) continue;
        walkNode(*d, scopeId);
    }
}

void SymbolIndex::walkNode(const ast::ASTNode& node, int scopeId) {
    switch (node.getKind()) {
        case ast::ASTNodeKind::FunctionDecl: {
            const auto& fn = static_cast<const ast::FunctionDecl&>(node);
            walkFunction(fn, scopeId, "", false);
            return;
        }
        case ast::ASTNodeKind::ClassDecl: {
            const auto& cls = static_cast<const ast::ClassDecl&>(node);
            Symbol sym;
            sym.name = cls.getName();
            sym.kind = SymKind::Class;
            sym.signature = "class " + cls.getName();
            sym.line = static_cast<int>(cls.getLocation().line);
            sym.col = static_cast<int>(cls.getLocation().column);
            sym.scopeId = scopeId;
            fixNameCol(sym);
            sym.docComment = docCommentAbove(sym.line);
            int endLine = findBlockEnd(sym.line - 1);
            sym.endLine = (endLine >= 0) ? endLine + 1 : sym.line;
            addSymbol(std::move(sym));
            for (const auto& m : cls.getMembers()) {
                if (!m) continue;
                if (m->getKind() == ast::ASTNodeKind::FunctionDecl) {
                    walkFunction(static_cast<const ast::FunctionDecl&>(*m), scopeId, cls.getName(), true);
                } else if (m->getKind() == ast::ASTNodeKind::VarDeclStmt) {
                    const auto& f = static_cast<const ast::VarDeclStmt&>(*m);
                    Symbol field;
                    field.name = f.getName();
                    field.kind = SymKind::Field;
                    field.type = f.getType();
                    if (field.type.empty() && f.hasInitializer()) {
                        field.type = inferType(f.getInitializer(), scopeId);
                    }
                    field.signature = field.name + (field.type.empty() ? "" : ": " + field.type);
                    field.line = static_cast<int>(f.getLocation().line);
                    field.col = static_cast<int>(f.getLocation().column);
                    field.endLine = field.line;
                    field.scopeId = scopeId;
                    field.parent = cls.getName();
                    fixNameCol(field);
                    addSymbol(std::move(field));
                    if (f.hasInitializer()) walkExpr(*f.getInitializer(), scopeId);
                } else if (m->getKind() == ast::ASTNodeKind::ClassDecl) {
                    walkNode(*m, scopeId);
                }
            }
            return;
        }
        case ast::ASTNodeKind::InterfaceDecl: {
            const auto& iface = static_cast<const ast::InterfaceDecl&>(node);
            Symbol sym;
            sym.name = iface.getName();
            sym.kind = SymKind::Interface;
            sym.signature = "interface " + iface.getName();
            sym.line = static_cast<int>(iface.getLocation().line);
            sym.col = static_cast<int>(iface.getLocation().column);
            sym.scopeId = scopeId;
            fixNameCol(sym);
            sym.docComment = docCommentAbove(sym.line);
            int endLine = findBlockEnd(sym.line - 1);
            sym.endLine = (endLine >= 0) ? endLine + 1 : sym.line;
            addSymbol(std::move(sym));
            for (const auto& m : iface.getMethods()) {
                if (m) walkFunction(*m, scopeId, iface.getName(), true);
            }
            return;
        }
        case ast::ASTNodeKind::StructDecl: {
            const auto& st = static_cast<const ast::StructDecl&>(node);
            Symbol sym;
            sym.name = st.getName();
            sym.kind = SymKind::Struct;
            sym.signature = "struct " + st.getName();
            sym.line = static_cast<int>(st.getLocation().line);
            sym.col = static_cast<int>(st.getLocation().column);
            sym.scopeId = scopeId;
            fixNameCol(sym);
            sym.docComment = docCommentAbove(sym.line);
            int endLine = findBlockEnd(sym.line - 1);
            sym.endLine = (endLine >= 0) ? endLine + 1 : sym.line;
            addSymbol(std::move(sym));
            for (const auto& f : st.getFields()) {
                if (!f) continue;
                Symbol field;
                field.name = f->getName();
                field.kind = SymKind::Field;
                field.type = f->getType();
                if (field.type.empty() && f->hasInitializer()) {
                    field.type = inferType(f->getInitializer(), scopeId);
                }
                field.signature = field.name + (field.type.empty() ? "" : ": " + field.type);
                field.line = static_cast<int>(f->getLocation().line);
                field.col = static_cast<int>(f->getLocation().column);
                field.endLine = field.line;
                field.scopeId = scopeId;
                field.parent = st.getName();
                fixNameCol(field);
                addSymbol(std::move(field));
                if (f->hasInitializer()) walkExpr(*f->getInitializer(), scopeId);
            }
            return;
        }
        case ast::ASTNodeKind::EnumDecl: {
            const auto& en = static_cast<const ast::EnumDecl&>(node);
            Symbol sym;
            sym.name = en.getName();
            sym.kind = SymKind::Enum;
            sym.signature = "enum " + en.getName();
            sym.line = static_cast<int>(en.getLocation().line);
            sym.col = static_cast<int>(en.getLocation().column);
            sym.scopeId = scopeId;
            fixNameCol(sym);
            sym.docComment = docCommentAbove(sym.line);
            int endLine = findBlockEnd(sym.line - 1);
            sym.endLine = (endLine >= 0) ? endLine + 1 : sym.line;
            addSymbol(std::move(sym));
            for (const auto& v : en.getVariants()) {
                Symbol var;
                var.name = v.getName();
                var.kind = SymKind::EnumVariant;
                var.signature = en.getName() + "." + v.getName();
                var.line = sym.line;
                var.col = sym.col;
                // Locate the variant name on its own line inside the enum.
                for (int l = sym.line - 1; l < sym.endLine && l < static_cast<int>(m_lines.size()); ++l) {
                    int c = findWordCol(l, v.getName(), 0);
                    if (c >= 0) {
                        var.line = l + 1;
                        var.col = c + 1;
                        break;
                    }
                }
                var.endLine = var.line;
                var.scopeId = scopeId;
                var.parent = en.getName();
                addSymbol(std::move(var));
                if (v.hasValue()) walkExpr(*v.getValue(), scopeId);
            }
            return;
        }
        default:
            // Any other top-level node (e.g. a bare statement) is walked
            // as a statement so its identifiers still resolve.
            if (const auto* stmt = dynamic_cast<const ast::StatementNode*>(&node)) {
                walkStmt(*stmt, scopeId);
            }
            return;
    }
}

void SymbolIndex::walkBlock(const ast::BlockStmt& block, int scopeId) {
    // The block's own scope was created by the caller (function scope or
    // a child scope for a nested block). Statements here share it; nested
    // blocks get their own child scope via walkStmt.
    for (const auto& s : block.getStatements()) {
        if (s) walkStmt(*s, scopeId);
    }
}

void SymbolIndex::walkStmt(const ast::StatementNode& stmt, int scopeId) {
    switch (stmt.getKind()) {
        case ast::ASTNodeKind::BlockStmt: {
            const auto& block = static_cast<const ast::BlockStmt&>(stmt);
            int startLine = static_cast<int>(block.getLocation().line);
            int endLine = findBlockEnd(startLine - 1);
            int child = addScope(scopeId, startLine, (endLine >= 0) ? endLine + 1 : startLine);
            walkBlock(block, child);
            return;
        }
        case ast::ASTNodeKind::VarDeclStmt: {
            const auto& v = static_cast<const ast::VarDeclStmt&>(stmt);
            Symbol sym;
            sym.name = v.getName();
            sym.kind = SymKind::Variable;
            sym.type = v.getType();
            if (sym.type.empty() && v.hasInitializer()) {
                sym.type = inferType(v.getInitializer(), scopeId);
            }
            sym.signature = (v.isConst() ? "const " : "let ") + sym.name +
                            (sym.type.empty() ? "" : ": " + sym.type);
            sym.line = static_cast<int>(v.getLocation().line);
            // The decl location points at `let`/`var`; find the name.
            int c = findWordCol(sym.line - 1, sym.name, sym.col);
            sym.col = (c >= 0) ? c + 1 : static_cast<int>(v.getLocation().column);
            sym.endLine = sym.line;
            sym.scopeId = scopeId;
            // NOTE: record the inlay hint BEFORE moving sym into the
            // vector — std::move pilfers its strings, so reading
            // sym.type/sym.name afterwards would always see "".
            if (v.getType().empty() && v.hasInitializer() && !sym.type.empty()) {
                LetHint hint;
                hint.line = sym.line - 1;
                hint.character = sym.col - 1 + static_cast<int>(sym.name.size());
                hint.type = sym.type;
                m_letHints.push_back(hint);
            }
            addSymbol(std::move(sym));
            if (v.hasInitializer()) walkExpr(*v.getInitializer(), scopeId);
            return;
        }
        case ast::ASTNodeKind::ExprStmt: {
            // Phase 4 slice B: ExprStmt::getExpr() is *m_expr — a null
            // expression (parser error recovery, e.g. `fn test()` leaving
            // a stray `:`) would segfault here without the guard.
            const auto& es = static_cast<const ast::ExprStmt&>(stmt);
            if (es.hasExpr()) walkExpr(es.getExpr(), scopeId);
            return;
        }
        case ast::ASTNodeKind::ReturnStmt: {
            const auto* val = static_cast<const ast::ReturnStmt&>(stmt).getValue();
            if (val) walkExpr(*val, scopeId);
            return;
        }
        case ast::ASTNodeKind::IfStmt: {
            const auto& s = static_cast<const ast::IfStmt&>(stmt);
            if (s.hasCondition()) walkExpr(s.getCondition(), scopeId);
            if (s.hasThenBranch()) walkStmt(s.getThenBranch(), scopeId);
            if (s.hasElseBranch()) walkStmt(*s.getElseBranch(), scopeId);
            return;
        }
        case ast::ASTNodeKind::WhileStmt: {
            const auto& s = static_cast<const ast::WhileStmt&>(stmt);
            if (s.hasCondition()) walkExpr(s.getCondition(), scopeId);
            if (s.hasBody()) walkStmt(s.getBody(), scopeId);
            return;
        }
        case ast::ASTNodeKind::DoWhileStmt: {
            const auto& s = static_cast<const ast::DoWhileStmt&>(stmt);
            if (s.hasBody()) walkStmt(s.getBody(), scopeId);
            if (s.hasCondition()) walkExpr(s.getCondition(), scopeId);
            return;
        }
        case ast::ASTNodeKind::ForStmt: {
            const auto& s = static_cast<const ast::ForStmt&>(stmt);
            int startLine = static_cast<int>(s.getLocation().line);
            int endLine = findBlockEnd(startLine - 1);
            int child = addScope(scopeId, startLine, (endLine >= 0) ? endLine + 1 : startLine);
            if (s.getInit()) walkStmt(*s.getInit(), child);
            if (s.getCondition()) walkExpr(*s.getCondition(), child);
            if (s.getIncrement()) walkExpr(*s.getIncrement(), child);
            if (s.hasBody()) walkStmt(s.getBody(), child);
            return;
        }
        case ast::ASTNodeKind::ForInStmt: {
            const auto& s = static_cast<const ast::ForInStmt&>(stmt);
            int startLine = static_cast<int>(s.getLocation().line);
            int endLine = findBlockEnd(startLine - 1);
            int child = addScope(scopeId, startLine, (endLine >= 0) ? endLine + 1 : startLine);
            Symbol loopVar;
            loopVar.name = s.getVarName();
            loopVar.kind = SymKind::Variable;
            loopVar.type = s.getType();
            loopVar.signature = loopVar.name + (loopVar.type.empty() ? "" : ": " + loopVar.type);
            loopVar.line = startLine;
            int c = findWordCol(startLine - 1, loopVar.name, 0);
            loopVar.col = (c >= 0) ? c + 1 : 1;
            loopVar.endLine = startLine;
            loopVar.scopeId = child;
            addSymbol(std::move(loopVar));
            if (s.hasIterable()) walkExpr(s.getIterable(), child);
            if (s.hasBody()) walkStmt(s.getBody(), child);
            return;
        }
        case ast::ASTNodeKind::SwitchStmt: {
            const auto& s = static_cast<const ast::SwitchStmt&>(stmt);
            if (s.hasSubject()) walkExpr(s.getSubject(), scopeId);
            for (const auto& kase : s.getCases()) {
                if (kase.hasValue()) walkExpr(kase.getValue(), scopeId);
                for (const auto& st : kase.getStatements()) {
                    if (st) walkStmt(*st, scopeId);
                }
            }
            for (const auto& st : s.getDefaultStatements()) {
                if (st) walkStmt(*st, scopeId);
            }
            return;
        }
        case ast::ASTNodeKind::MatchStmt: {
            const auto& s = static_cast<const ast::MatchStmt&>(stmt);
            if (s.hasSubject()) walkExpr(s.getSubject(), scopeId);
            for (const auto& arm : s.getArms()) {
                if (arm.getPattern()) walkExpr(*arm.getPattern(), scopeId);
                if (arm.hasBody()) walkNode(arm.getBody(), scopeId);
            }
            return;
        }
        case ast::ASTNodeKind::TryCatchStmt: {
            const auto& s = static_cast<const ast::TryCatchStmt&>(stmt);
            walkBlock(s.getTryBlock(), scopeId);
            for (const auto& catchClause : s.getCatches()) {
                int startLine = static_cast<int>(catchClause.getBody().getLocation().line);
                int endLine = findBlockEnd(startLine - 1);
                int child = addScope(scopeId, startLine, (endLine >= 0) ? endLine + 1 : startLine);
                Symbol caught;
                caught.name = catchClause.getName();
                caught.kind = SymKind::Variable;
                caught.type = catchClause.getType();
                caught.signature = caught.name + (caught.type.empty() ? "" : ": " + caught.type);
                caught.line = startLine;
                int c = findWordCol(startLine - 1, caught.name, 0);
                caught.col = (c >= 0) ? c + 1 : 1;
                caught.endLine = startLine;
                caught.scopeId = child;
                addSymbol(std::move(caught));
                walkBlock(catchClause.getBody(), child);
            }
            if (s.hasFinally()) walkBlock(*s.getFinallyBlock(), scopeId);
            return;
        }
        case ast::ASTNodeKind::ThrowStmt: {
            const auto& t = static_cast<const ast::ThrowStmt&>(stmt);
            if (t.hasExpr()) walkExpr(t.getExpr(), scopeId);
            return;
        }
        case ast::ASTNodeKind::FunctionDecl:
        case ast::ASTNodeKind::ClassDecl:
        case ast::ASTNodeKind::InterfaceDecl:
        case ast::ASTNodeKind::StructDecl:
        case ast::ASTNodeKind::EnumDecl:
            walkNode(stmt, scopeId);
            return;
        default:
            return;
    }
}

void SymbolIndex::walkExpr(const ast::ExpressionNode& expr, int scopeId) {
    switch (expr.getKind()) {
        case ast::ASTNodeKind::IdentifierExpr: {
            const auto& id = static_cast<const ast::IdentifierExpr&>(expr);
            Use u;
            u.name = id.getName();
            u.line = static_cast<int>(id.getLocation().line);
            u.col = static_cast<int>(id.getLocation().column);
            u.endCol = u.col + static_cast<int>(u.name.size());
            u.scopeId = scopeId;
            m_uses.push_back(std::move(u));
            return;
        }
        case ast::ASTNodeKind::MemberExpr: {
            const auto& mem = static_cast<const ast::MemberExpr&>(expr);
            walkExpr(mem.getObject(), scopeId);
            Use u;
            u.name = mem.getMember();
            u.isMember = true;
            u.receiver = &mem.getObject();
            u.scopeId = scopeId;
            // The member name follows a '.' on the object's line (or a
            // later line for chained access). Search from the object.
            int objLine = static_cast<int>(mem.getObject().getLocation().line);
            int objCol = static_cast<int>(mem.getObject().getLocation().column);
            u.line = objLine;
            u.col = objCol;
            for (int l = objLine - 1; l < static_cast<int>(m_lines.size()); ++l) {
                int from = (l == objLine - 1) ? objCol - 1 : 0;
                int c = findWordCol(l, u.name, from);
                if (c >= 0) {
                    // Must be preceded by '.' (possibly after whitespace).
                    int dot = c - 1;
                    while (dot >= 0 && m_lines[l][dot] == ' ') dot--;
                    if (dot >= 0 && m_lines[l][dot] == '.') {
                        u.line = l + 1;
                        u.col = c + 1;
                        break;
                    }
                }
                if (l > objLine) break; // only look one line ahead
            }
            u.endCol = u.col + static_cast<int>(u.name.size());
            m_uses.push_back(std::move(u));
            return;
        }
        case ast::ASTNodeKind::CallExpr: {
            const auto& call = static_cast<const ast::CallExpr&>(expr);
            walkExpr(call.getCallee(), scopeId);
            for (const auto& a : call.getArgs()) {
                if (a) walkExpr(*a, scopeId);
            }
            return;
        }
        case ast::ASTNodeKind::NewExpr: {
            const auto& nw = static_cast<const ast::NewExpr&>(expr);
            Use u;
            u.name = nw.getTypeName();
            u.line = static_cast<int>(nw.getLocation().line);
            int c = findWordCol(u.line - 1, u.name, static_cast<int>(nw.getLocation().column));
            u.col = (c >= 0) ? c + 1 : static_cast<int>(nw.getLocation().column);
            u.endCol = u.col + static_cast<int>(u.name.size());
            u.scopeId = scopeId;
            m_uses.push_back(std::move(u));
            for (const auto& a : nw.getArgs()) {
                if (a) walkExpr(*a, scopeId);
            }
            return;
        }
        case ast::ASTNodeKind::BinaryExpr: {
            const auto& b = static_cast<const ast::BinaryExpr&>(expr);
            walkExpr(b.getLeft(), scopeId);
            if (b.hasRight()) walkExpr(b.getRight(), scopeId);
            return;
        }
        case ast::ASTNodeKind::UnaryExpr: {
            const auto& u = static_cast<const ast::UnaryExpr&>(expr);
            if (u.hasOperand()) walkExpr(u.getOperand(), scopeId);
            return;
        }
        case ast::ASTNodeKind::AssignExpr: {
            const auto& a = static_cast<const ast::AssignExpr&>(expr);
            walkExpr(a.getTarget(), scopeId);
            if (a.hasValue()) walkExpr(a.getValue(), scopeId);
            return;
        }
        case ast::ASTNodeKind::TernaryExpr: {
            const auto& t = static_cast<const ast::TernaryExpr&>(expr);
            walkExpr(t.getCondition(), scopeId);
            if (t.hasThenExpr()) walkExpr(t.getThenExpr(), scopeId);
            if (t.hasElseExpr()) walkExpr(t.getElseExpr(), scopeId);
            return;
        }
        case ast::ASTNodeKind::IndexExpr: {
            const auto& ix = static_cast<const ast::IndexExpr&>(expr);
            walkExpr(ix.getObject(), scopeId);
            if (ix.hasIndex()) walkExpr(ix.getIndex(), scopeId);
            return;
        }
        case ast::ASTNodeKind::ArrayExpr:
            for (const auto& e : static_cast<const ast::ArrayExpr&>(expr).getElements()) {
                if (e) walkExpr(*e, scopeId);
            }
            return;
        case ast::ASTNodeKind::MapExpr:
            for (const auto& entry : static_cast<const ast::MapExpr&>(expr).getEntries()) {
                if (entry.first) walkExpr(*entry.first, scopeId);
                if (entry.second) walkExpr(*entry.second, scopeId);
            }
            return;
        case ast::ASTNodeKind::LambdaExpr: {
            const auto& lam = static_cast<const ast::LambdaExpr&>(expr);
            int startLine = static_cast<int>(lam.getLocation().line);
            int endLine = findBlockEnd(startLine - 1);
            int child = addScope(scopeId, startLine, (endLine >= 0) ? endLine + 1 : startLine);
            for (const auto& p : lam.getParams()) {
                Symbol ps;
                ps.name = p.name;
                ps.kind = SymKind::Parameter;
                ps.type = p.type;
                ps.signature = ps.name + (ps.type.empty() ? "" : ": " + ps.type);
                ps.line = startLine;
                int c = findWordCol(startLine - 1, ps.name, 0);
                ps.col = (c >= 0) ? c + 1 : 1;
                ps.endLine = startLine;
                ps.scopeId = child;
                addSymbol(std::move(ps));
            }
            if (lam.hasBody()) walkNode(lam.getBody(), child);
            return;
        }
        case ast::ASTNodeKind::AwaitExpr: {
            const auto& aw = static_cast<const ast::AwaitExpr&>(expr);
            if (aw.getExprPtr()) walkExpr(aw.getExpr(), scopeId);
            return;
        }
        case ast::ASTNodeKind::TryExpr: {
            const auto& te = static_cast<const ast::TryExpr&>(expr);
            if (te.getExprPtr()) walkExpr(te.getExpr(), scopeId);
            return;
        }
        case ast::ASTNodeKind::ComptimeExpr: {
            const auto& c = static_cast<const ast::ComptimeExpr&>(expr);
            if (c.hasExpr() && c.getExpr()) walkExpr(*c.getExpr(), scopeId);
            return;
        }
        default:
            return;
    }
}

const Symbol* SymbolIndex::findTopLevel(const std::string& name) const {
    for (const auto& s : m_symbols) {
        if (s.scopeId == 0 && s.name == name) return &s;
    }
    return nullptr;
}

const Symbol* SymbolIndex::findMember(const std::string& className, const std::string& member) const {
    for (const auto& s : m_symbols) {
        if (s.parent == className && s.name == member &&
            (s.kind == SymKind::Method || s.kind == SymKind::Field)) {
            return &s;
        }
    }
    return nullptr;
}

const Symbol* SymbolIndex::resolveName(const std::string& name, int scopeId, int useLine) const {
    // Walk the scope chain innermost-first. Inside a function scope a
    // use only sees declarations on or before its line (sequential
    // shadowing); top-level declarations are visible from anywhere.
    int cur = scopeId;
    while (cur >= 0) {
        const Scope& scope = m_scopes[cur];
        const Symbol* best = nullptr;
        for (const auto& s : m_symbols) {
            if (s.scopeId != cur || s.name != name) continue;
            if (cur == 0 || s.line <= useLine) {
                if (!best || s.line > best->line) best = &s;
            }
        }
        if (best) return best;
        cur = scope.parent;
    }
    return nullptr;
}

const Symbol* SymbolIndex::resolveMemberUse(const Use& use) const {
    if (!use.receiver) return nullptr;
    // 1. Sema type of the receiver expression.
    if (m_sema) {
        auto t = m_sema->getNodeType(use.receiver);
        if (t) {
            std::string typeName = t->toString();
            if (const Symbol* m = findMember(typeName, use.name)) return m;
            // `Type.member` static access: the type itself is the class.
            if (const Symbol* cls = findTopLevel(typeName)) {
                (void)cls;
                if (const Symbol* m = findMember(typeName, use.name)) return m;
            }
        }
    }
    // 2. Receiver is a plain identifier: resolve it, then use its type.
    if (use.receiver->getKind() == ast::ASTNodeKind::IdentifierExpr) {
        const auto& id = static_cast<const ast::IdentifierExpr&>(*use.receiver);
        if (const Symbol* recv = resolveName(id.getName(), use.scopeId, use.line)) {
            if (!recv->type.empty()) {
                if (const Symbol* m = findMember(recv->type, use.name)) return m;
            }
            if (recv->kind == SymKind::Class || recv->kind == SymKind::Struct ||
                recv->kind == SymKind::Interface || recv->kind == SymKind::Enum) {
                if (const Symbol* m = findMember(recv->name, use.name)) return m;
            }
        }
    }
    // 3. Last resort: the member name is unambiguous across all classes.
    const Symbol* only = nullptr;
    int count = 0;
    for (const auto& s : m_symbols) {
        if (s.name == use.name && (s.kind == SymKind::Method || s.kind == SymKind::Field)) {
            only = &s;
            ++count;
        }
    }
    return (count == 1) ? only : nullptr;
}

void SymbolIndex::resolveAllUses() {
    for (auto& u : m_uses) {
        const Symbol* target = nullptr;
        if (u.isMember) {
            target = resolveMemberUse(u);
        } else {
            target = resolveName(u.name, u.scopeId, u.line);
        }
        u.target = (target == nullptr) ? -1
                                       : static_cast<int>(target - m_symbols.data());
    }
}

const Symbol* SymbolIndex::resolveAt(int line0, int col0) const {
    if (!m_valid) return nullptr;
    int line = line0 + 1;
    int col = col0 + 1;
    // 1. Cursor on a declaration name itself.
    for (const auto& s : m_symbols) {
        if (s.line == line && col >= s.col && col < s.col + static_cast<int>(s.name.size())) {
            return &s;
        }
    }
    // 2. Cursor on a use.
    for (const auto& u : m_uses) {
        if (u.line == line && col >= u.col && col < u.endCol) {
            if (u.target >= 0) return &m_symbols[u.target];
            return nullptr;
        }
    }
    return nullptr;
}

bool SymbolIndex::rangeAt(int line0, int col0, RefPos& out) const {
    if (!m_valid) return false;
    int line = line0 + 1;
    int col = col0 + 1;
    for (const auto& s : m_symbols) {
        if (s.line == line && col >= s.col && col < s.col + static_cast<int>(s.name.size())) {
            out.line = line0;
            out.character = s.col - 1;
            out.endCharacter = s.col - 1 + static_cast<int>(s.name.size());
            return true;
        }
    }
    for (const auto& u : m_uses) {
        if (u.line == line && col >= u.col && col < u.endCol) {
            out.line = line0;
            out.character = u.col - 1;
            out.endCharacter = u.endCol - 1;
            return true;
        }
    }
    return false;
}

std::vector<RefPos> SymbolIndex::referencesOf(const Symbol* sym) const {
    std::vector<RefPos> out;
    if (!sym) return out;
    int idx = static_cast<int>(sym - m_symbols.data());
    auto push = [&](int line1, int col1, int nameLen) {
        RefPos r;
        r.line = line1 - 1;
        r.character = col1 - 1;
        r.endCharacter = col1 - 1 + nameLen;
        out.push_back(r);
    };
    push(sym->line, sym->col, static_cast<int>(sym->name.size()));
    for (const auto& u : m_uses) {
        if (u.target == idx) {
            push(u.line, u.col, static_cast<int>(u.name.size()));
        }
    }
    std::sort(out.begin(), out.end(), [](const RefPos& a, const RefPos& b) {
        return a.line != b.line ? a.line < b.line : a.character < b.character;
    });
    return out;
}

std::vector<const Symbol*> SymbolIndex::visibleAt(int line0, int col0) const {
    std::vector<const Symbol*> out;
    if (!m_valid) return out;
    int line = line0 + 1;
    (void)col0;
    // Find the deepest scope containing this line.
    int bestScope = 0;
    int bestDepth = -1;
    for (size_t i = 0; i < m_scopes.size(); ++i) {
        const Scope& sc = m_scopes[i];
        if (line < sc.startLine || line > sc.endLine) continue;
        int depth = 0;
        for (int p = sc.parent; p >= 0; p = m_scopes[p].parent) depth++;
        if (depth > bestDepth) {
            bestDepth = depth;
            bestScope = static_cast<int>(i);
        }
    }
    std::vector<std::string> seen;
    auto addUnique = [&](const Symbol* s) {
        if (std::find(seen.begin(), seen.end(), s->name) == seen.end()) {
            seen.push_back(s->name);
            out.push_back(s);
        }
    };
    int cur = bestScope;
    while (cur >= 0) {
        for (const auto& s : m_symbols) {
            // Top-level declarations are visible from anywhere (forward
            // references work); locals only from their line onward.
            if (s.scopeId == cur && (cur == 0 || s.line <= line)) addUnique(&s);
        }
        cur = m_scopes[cur].parent;
    }
    return out;
}

} // namespace sp::lsp
