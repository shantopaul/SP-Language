#pragma once

// Phase 4 slice B (pro-4.0.0): AST-based symbol index for one SP document.
// The pre-existing LSP handlers resolved names only through DocGenerator
// (top-level declarations) or raw token text, so hover/definition returned
// null for every local variable and parameter, and rename renamed
// same-named variables in OTHER functions (no scope awareness). This index
// walks the real AST with a scope tree, resolves each identifier use to
// its declaration (sequential shadowing inside a scope, innermost scope
// wins), and carries sema-inferred types for inlay hints and completion.
//
// The index is built fresh per request from the document text (parse +
// sema). If parsing fails the index is invalid and callers fall back to
// the old DocGenerator/token behaviour. It never invents language
// semantics: unresolved names stay unresolved.

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace sp::ast {
class ASTNode;
class BlockStmt;
class ExpressionNode;
class FunctionDecl;
class Program;
class StatementNode;
} // namespace sp::ast

namespace sp::diagnostics {
class DiagnosticEngine;
} // namespace sp::diagnostics

namespace sp::sema {
class SemanticAnalyzer;
} // namespace sp::sema

namespace sp::lsp {

enum class SymKind {
    Function,
    Method,
    Class,
    Interface,
    Struct,
    Enum,
    EnumVariant,
    Variable,
    Parameter,
    Field
};

struct Symbol {
    std::string name;
    SymKind kind = SymKind::Variable;
    std::string type;       // declared or sema-inferred type ("" = unknown)
    std::string signature;  // display string, e.g. "fn add(a: int) -> int"
    std::string docComment; // `///` lines directly above the declaration
    int line = 1;           // 1-based, name start
    int col = 1;            // 1-based, name start
    int endLine = 1;        // 1-based, inclusive (block end for fns/classes)
    int scopeId = 0;        // owning scope
    std::string parent;     // enclosing class name for members
};

struct Use {
    std::string name;
    int line = 1;           // 1-based
    int col = 1;            // 1-based, name start
    int endCol = 1;         // 1-based, exclusive
    int scopeId = 0;
    bool isMember = false;  // `obj.name` — resolved via the receiver's type
    const sp::ast::ExpressionNode* receiver = nullptr;
    int target = -1;        // index into SymbolIndex::symbols(), -1 = unresolved
};

struct Scope {
    int parent = -1;
    int startLine = 1;      // 1-based
    int endLine = 1;        // 1-based, inclusive
};

struct LetHint {
    int line = 0;           // 0-based (LSP)
    int character = 0;      // 0-based, just past the variable name
    std::string type;       // inferred type for a `let` without annotation
};

struct RefPos {
    int line = 0;           // 0-based (LSP)
    int character = 0;      // 0-based
    int endCharacter = 0;   // 0-based, exclusive
};

class SymbolIndex {
public:
    // Build from one document's source. Returns false when the source
    // does not parse (index invalid; callers must fall back).
    bool build(const std::string& source, const std::string& uri);

    [[nodiscard]] bool valid() const noexcept { return m_valid; }

    // Resolve the declaration for the identifier at a 0-based LSP
    // position (use site or the declaration name itself). nullptr when
    // nothing is declared there or the name is unresolved.
    [[nodiscard]] const Symbol* resolveAt(int line0, int col0) const;

    // Range (0-based) of the declaration name or the use at a position,
    // for hover ranges (which must cover the hovered word, not the
    // declaration). False when nothing is declared there.
    [[nodiscard]] bool rangeAt(int line0, int col0, RefPos& out) const;

    // All positions (0-based) of the symbol: its declaration plus every
    // use that resolves to it. Empty when sym is null.
    [[nodiscard]] std::vector<RefPos> referencesOf(const Symbol* sym) const;

    // Symbols visible at a position (innermost scope outward, then
    // top-level), for completion. Deduplicated by name, innermost first.
    [[nodiscard]] std::vector<const Symbol*> visibleAt(int line0, int col0) const;

    // Inlay hints for `let` bindings without an explicit type annotation.
    [[nodiscard]] const std::vector<LetHint>& letHints() const noexcept { return m_letHints; }

    [[nodiscard]] const Symbol* findMember(const std::string& className, const std::string& member) const;
    [[nodiscard]] const Symbol* findTopLevel(const std::string& name) const;
    [[nodiscard]] const std::vector<Symbol>& symbols() const noexcept { return m_symbols; }
    [[nodiscard]] const std::vector<Use>& uses() const noexcept { return m_uses; }

private:
    bool m_valid = false;
    std::string m_uri;
    std::vector<std::string> m_lines; // source split by '\n'
    std::vector<Symbol> m_symbols;
    std::vector<Use> m_uses;
    std::vector<Scope> m_scopes;
    std::vector<LetHint> m_letHints;

    // Owned analysis state (kept alive: sema node-type lookups key on
    // AST node pointers).
    std::ostringstream m_diagStream;
    std::unique_ptr<diagnostics::DiagnosticEngine> m_diagEngine;
    std::unique_ptr<ast::Program> m_program;
    std::unique_ptr<sema::SemanticAnalyzer> m_sema;

    int addScope(int parent, int startLine, int endLine);
    int addSymbol(Symbol sym);
    void resolveAllUses();

    // Walking
    void walkDecls(const std::vector<std::unique_ptr<ast::ASTNode>>& decls, int scopeId);
    void walkNode(const ast::ASTNode& node, int scopeId);
    void walkStmt(const ast::StatementNode& stmt, int scopeId);
    void walkBlock(const ast::BlockStmt& block, int scopeId);
    void walkExpr(const ast::ExpressionNode& expr, int scopeId);
    void walkFunction(const ast::FunctionDecl& fn, int scopeId, const std::string& parentClass, bool isMethod);

    // Helpers
    std::string inferType(const ast::ExpressionNode* expr, int scopeId) const;
    std::string signatureOf(const ast::FunctionDecl& fn, bool isMethod) const;
    int findBlockEnd(int startLine0) const; // 0-based line of matching '}', -1
    const Symbol* resolveName(const std::string& name, int scopeId, int useLine) const;
    const Symbol* resolveMemberUse(const Use& use) const;
    int findWordCol(int line0, const std::string& word, int fromCol0) const;
    // Declaration locations point at the keyword (`fn`, `class`, ...),
    // not the name: move sym.col onto the name on its line.
    void fixNameCol(Symbol& sym) const;
    // `///` doc-comment lines directly above a declaration line.
    std::string docCommentAbove(int declLine1) const;
};

} // namespace sp::lsp
