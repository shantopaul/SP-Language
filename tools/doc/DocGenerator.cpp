#include "DocGenerator.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/lexer/Token.h"
#include "sp/lexer/TokenType.h"
#include "sp/parser/Parser.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Statement.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;

namespace sp::doc {

namespace {

struct RawDocBlock {
    std::string raw;
    std::string cleanText;
    std::string brief;
    std::map<std::string, std::string> params;
    std::string returnDesc;
    int startLine = 0;
    int endLine = 0;
    bool used = false;
};

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string escapeHtml(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}

RawDocBlock parseDocBlock(const std::string& raw, int startLine, int endLine) {
    RawDocBlock block;
    block.raw = raw;
    block.startLine = startLine;
    block.endLine = endLine;

    std::istringstream stream(raw);
    std::string line;
    std::ostringstream descBuilder;

    while (std::getline(stream, line)) {
        // Strip trailing CR
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string trimmed = trim(line);
        if (trimmed.starts_with("///")) {
            line = trimmed.substr(3);
            if (!line.empty() && line[0] == ' ') line = line.substr(1);
        } else if (trimmed.starts_with("/**")) {
            line = trimmed.substr(3);
            if (!line.empty() && line[0] == ' ') line = line.substr(1);
        } else if (trimmed.starts_with("*/")) {
            continue;
        } else if (trimmed.starts_with("*")) {
            line = trimmed.substr(1);
            if (!line.empty() && line[0] == ' ') line = line.substr(1);
        }

        // Check if line ends with */
        auto endPos = line.rfind("*/");
        if (endPos != std::string::npos) {
            line = line.substr(0, endPos);
        }

        std::string lineTrimmed = trim(line);
        if (lineTrimmed.starts_with("@param")) {
            std::string rem = trim(lineTrimmed.substr(6));
            auto spacePos = rem.find_first_of(" \t");
            if (spacePos != std::string::npos) {
                std::string paramName = rem.substr(0, spacePos);
                std::string paramDesc = trim(rem.substr(spacePos));
                block.params[paramName] = paramDesc;
            } else if (!rem.empty()) {
                block.params[rem] = "";
            }
        } else if (lineTrimmed.starts_with("@return") || lineTrimmed.starts_with("@returns")) {
            size_t tagLen = lineTrimmed.starts_with("@returns") ? 8 : 7;
            block.returnDesc = trim(lineTrimmed.substr(tagLen));
        } else {
            descBuilder << line << "\n";
        }
    }

    block.cleanText = trim(descBuilder.str());
    block.brief = DocGenerator::extractBrief(block.cleanText);
    return block;
}

std::vector<RawDocBlock> collectDocBlocks(const std::string& source) {
    std::vector<RawDocBlock> blocks;
    std::ostringstream diagStream;
    diagnostics::DiagnosticEngine diagEngine(diagStream);

    lexer::Lexer lexer(source, "source.sp", &diagEngine);
    lexer.setPreserveComments(true);
    auto tokens = lexer.tokenizeAll();

    std::vector<lexer::Token> currentCommentTokens;

    auto flushComments = [&]() {
        if (currentCommentTokens.empty()) return;

        bool hasDocComment = false;
        std::ostringstream rawCombined;
        int startLine = static_cast<int>(currentCommentTokens.front().location.line);
        int endLine = static_cast<int>(currentCommentTokens.back().location.line);

        for (const auto& tok : currentCommentTokens) {
            if (tok.type == lexer::TokenType::DocComment ||
                tok.lexeme.starts_with("///") || tok.lexeme.starts_with("/**")) {
                hasDocComment = true;
            }
            rawCombined << tok.lexeme << "\n";
        }

        if (hasDocComment) {
            blocks.push_back(parseDocBlock(rawCombined.str(), startLine, endLine));
        }
        currentCommentTokens.clear();
    };

    for (const auto& tok : tokens) {
        if (tok.type == lexer::TokenType::DocComment || tok.type == lexer::TokenType::Comment) {
            if (!currentCommentTokens.empty() &&
                tok.location.line > currentCommentTokens.back().location.line + 1) {
                flushComments();
            }
            currentCommentTokens.push_back(tok);
        } else if (tok.type != lexer::TokenType::EndOfFile) {
            flushComments();
        }
    }
    flushComments();

    return blocks;
}

RawDocBlock* findDocBlockForLine(std::vector<RawDocBlock>& blocks, int line) {
    RawDocBlock* best = nullptr;
    int minDiff = 5; // Search within 4 lines above the declaration

    for (auto& b : blocks) {
        if (b.used) continue;
        if (b.endLine < line && (line - b.endLine) <= minDiff) {
            best = &b;
            minDiff = line - b.endLine;
        }
    }

    if (best) {
        best->used = true;
    }
    return best;
}

void attachDoc(DocEntry& entry, RawDocBlock* block) {
    if (!block) return;
    entry.docComment = block->cleanText;
    entry.brief = block->brief;
    if (!block->returnDesc.empty()) {
        entry.returnDescription = block->returnDesc;
    }
    for (auto& p : entry.params) {
        auto it = block->params.find(p.name);
        if (it != block->params.end()) {
            p.description = it->second;
        }
    }
}

} // namespace

DocGenerator::DocGenerator(DocOptions options)
    : m_options(std::move(options)) {}

std::string DocGenerator::cleanDocComment(const std::string& raw) {
    return parseDocBlock(raw, 0, 0).cleanText;
}

std::string DocGenerator::extractBrief(const std::string& doc) {
    if (doc.empty()) return "";
    std::istringstream stream(doc);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) {
            return trimmed;
        }
    }
    return "";
}

DocModule DocGenerator::extractModule(const std::string& sourceCode, const std::string& filePath) {
    DocModule mod;
    mod.filePath = filePath;
    mod.name = filePath.empty() ? "main" : fs::path(filePath).stem().string();

    auto docBlocks = collectDocBlocks(sourceCode);

    std::ostringstream diagStream;
    diagnostics::DiagnosticEngine diagEngine(diagStream);
    lexer::Lexer lexer(sourceCode, filePath.empty() ? "source.sp" : filePath, &diagEngine);
    auto tokens = lexer.tokenizeAll();

    parser::Parser parser(tokens, filePath.empty() ? "source.sp" : filePath, &diagEngine);
    auto program = parser.parseProgram();

    if (program) {
        for (const auto& stmt : program->getDeclarations()) {
            if (!stmt) continue;

            if (const auto* modDecl = dynamic_cast<const ast::ModuleDecl*>(stmt.get())) {
                mod.name = modDecl->getName();
                int line = static_cast<int>(modDecl->getLocation().line);
                auto* b = findDocBlockForLine(docBlocks, line);
                if (b) {
                    mod.docComment = b->cleanText;
                    mod.brief = b->brief;
                }
            } else if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(stmt.get())) {
                // Skip internal generated test functions from documentation
                if (fn->getName().starts_with("__sp_test_")) continue;

                DocEntry entry;
                entry.name = fn->getName();
                entry.kind = DocEntryKind::Function;
                entry.visibility = fn->getVisibility().empty() ? "public" : fn->getVisibility();
                entry.isStatic = fn->isStatic();
                entry.isAsync = fn->isAsync();
                entry.isOverride = fn->isOverride();
                entry.returnType = fn->getReturnType().empty() ? "void" : fn->getReturnType();
                entry.line = static_cast<int>(fn->getLocation().line);

                std::ostringstream sig;
                if (!entry.visibility.empty() && entry.visibility != "public") sig << entry.visibility << " ";
                if (entry.isAsync) sig << "async ";
                sig << "fn " << entry.name;
                if (fn->isGeneric()) {
                    sig << "<";
                    for (size_t i = 0; i < fn->getTypeParams().size(); ++i) {
                        if (i > 0) sig << ", ";
                        sig << fn->getTypeParams()[i];
                    }
                    sig << ">";
                }
                sig << "(";
                for (size_t i = 0; i < fn->getParams().size(); ++i) {
                    const auto& p = fn->getParams()[i];
                    if (i > 0) sig << ", ";
                    sig << p.getName() << ": " << p.getType();
                    entry.params.push_back(DocParam{p.getName(), p.getType(), "", ""});
                }
                sig << "): " << entry.returnType;
                entry.signature = sig.str();

                attachDoc(entry, findDocBlockForLine(docBlocks, entry.line));
                mod.entries.push_back(entry);
            } else if (const auto* cls = dynamic_cast<const ast::ClassDecl*>(stmt.get())) {
                DocEntry entry;
                entry.name = cls->getName();
                entry.kind = DocEntryKind::Class;
                entry.visibility = cls->getVisibility().empty() ? "public" : cls->getVisibility();
                entry.line = static_cast<int>(cls->getLocation().line);

                std::ostringstream sig;
                if (!entry.visibility.empty() && entry.visibility != "public") sig << entry.visibility << " ";
                if (cls->isAbstract()) sig << "abstract ";
                sig << "class " << entry.name;
                if (cls->isGeneric()) {
                    sig << "<";
                    for (size_t i = 0; i < cls->getTypeParams().size(); ++i) {
                        if (i > 0) sig << ", ";
                        sig << cls->getTypeParams()[i];
                    }
                    sig << ">";
                }
                if (cls->hasBaseClass()) {
                    sig << " extends " << cls->getBaseClass();
                }
                if (!cls->getInterfaces().empty()) {
                    sig << " implements ";
                    for (size_t i = 0; i < cls->getInterfaces().size(); ++i) {
                        if (i > 0) sig << ", ";
                        sig << cls->getInterfaces()[i];
                    }
                }
                entry.signature = sig.str();

                attachDoc(entry, findDocBlockForLine(docBlocks, entry.line));

                // Process class members
                for (const auto& member : cls->getMembers()) {
                    if (const auto* mFn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
                        DocEntry mEntry;
                        mEntry.name = mFn->getName();
                        mEntry.kind = DocEntryKind::Method;
                        mEntry.visibility = mFn->getVisibility().empty() ? "public" : mFn->getVisibility();
                        mEntry.isStatic = mFn->isStatic();
                        mEntry.isAsync = mFn->isAsync();
                        mEntry.isOverride = mFn->isOverride();
                        mEntry.returnType = mFn->getReturnType().empty() ? "void" : mFn->getReturnType();
                        mEntry.line = static_cast<int>(mFn->getLocation().line);

                        std::ostringstream mSig;
                        if (!mEntry.visibility.empty()) mSig << mEntry.visibility << " ";
                        if (mEntry.isStatic) mSig << "static ";
                        if (mEntry.isOverride) mSig << "override ";
                        if (mEntry.isAsync) mSig << "async ";
                        mSig << "fn " << mEntry.name << "(";
                        for (size_t i = 0; i < mFn->getParams().size(); ++i) {
                            const auto& p = mFn->getParams()[i];
                            if (i > 0) mSig << ", ";
                            mSig << p.getName() << ": " << p.getType();
                            mEntry.params.push_back(DocParam{p.getName(), p.getType(), "", ""});
                        }
                        mSig << "): " << mEntry.returnType;
                        mEntry.signature = mSig.str();

                        attachDoc(mEntry, findDocBlockForLine(docBlocks, mEntry.line));
                        entry.members.push_back(mEntry);
                    } else if (const auto* mVar = dynamic_cast<const ast::VarDeclStmt*>(member.get())) {
                        DocEntry mEntry;
                        mEntry.name = mVar->getName();
                        mEntry.kind = DocEntryKind::Field;
                        mEntry.line = static_cast<int>(mVar->getLocation().line);
                        mEntry.returnType = mVar->getType();
                        mEntry.signature = (mVar->isConst() ? "const " : "let ") + mEntry.name + ": " + mEntry.returnType;

                        attachDoc(mEntry, findDocBlockForLine(docBlocks, mEntry.line));
                        entry.members.push_back(mEntry);
                    }
                }

                mod.entries.push_back(entry);
            } else if (const auto* iface = dynamic_cast<const ast::InterfaceDecl*>(stmt.get())) {
                DocEntry entry;
                entry.name = iface->getName();
                entry.kind = DocEntryKind::Interface;
                entry.visibility = iface->getVisibility().empty() ? "public" : iface->getVisibility();
                entry.line = static_cast<int>(iface->getLocation().line);
                entry.signature = "interface " + entry.name;

                attachDoc(entry, findDocBlockForLine(docBlocks, entry.line));

                for (const auto& mFn : iface->getMethods()) {
                    DocEntry mEntry;
                    mEntry.name = mFn->getName();
                    mEntry.kind = DocEntryKind::Method;
                    mEntry.returnType = mFn->getReturnType().empty() ? "void" : mFn->getReturnType();
                    mEntry.line = static_cast<int>(mFn->getLocation().line);

                    std::ostringstream mSig;
                    mSig << "fn " << mEntry.name << "(";
                    for (size_t i = 0; i < mFn->getParams().size(); ++i) {
                        const auto& p = mFn->getParams()[i];
                        if (i > 0) mSig << ", ";
                        mSig << p.getName() << ": " << p.getType();
                        mEntry.params.push_back(DocParam{p.getName(), p.getType(), "", ""});
                    }
                    mSig << "): " << mEntry.returnType;
                    mEntry.signature = mSig.str();

                    attachDoc(mEntry, findDocBlockForLine(docBlocks, mEntry.line));
                    entry.members.push_back(mEntry);
                }

                mod.entries.push_back(entry);
            } else if (const auto* st = dynamic_cast<const ast::StructDecl*>(stmt.get())) {
                DocEntry entry;
                entry.name = st->getName();
                entry.kind = DocEntryKind::Struct;
                entry.visibility = st->getVisibility().empty() ? "public" : st->getVisibility();
                entry.line = static_cast<int>(st->getLocation().line);
                entry.signature = "struct " + entry.name;

                attachDoc(entry, findDocBlockForLine(docBlocks, entry.line));

                for (const auto& fVar : st->getFields()) {
                    DocEntry fEntry;
                    fEntry.name = fVar->getName();
                    fEntry.kind = DocEntryKind::Field;
                    fEntry.line = static_cast<int>(fVar->getLocation().line);
                    fEntry.returnType = fVar->getType();
                    fEntry.signature = fEntry.name + ": " + fEntry.returnType;

                    attachDoc(fEntry, findDocBlockForLine(docBlocks, fEntry.line));
                    entry.members.push_back(fEntry);
                }

                mod.entries.push_back(entry);
            } else if (const auto* enm = dynamic_cast<const ast::EnumDecl*>(stmt.get())) {
                DocEntry entry;
                entry.name = enm->getName();
                entry.kind = DocEntryKind::Enum;
                entry.visibility = enm->getVisibility().empty() ? "public" : enm->getVisibility();
                entry.line = static_cast<int>(enm->getLocation().line);
                entry.signature = "enum " + entry.name;

                attachDoc(entry, findDocBlockForLine(docBlocks, entry.line));

                for (const auto& v : enm->getVariants()) {
                    DocEntry vEntry;
                    vEntry.name = v.getName();
                    vEntry.kind = DocEntryKind::Field;
                    vEntry.signature = v.getName();
                    entry.members.push_back(vEntry);
                }

                mod.entries.push_back(entry);
            } else if (const auto* v = dynamic_cast<const ast::VarDeclStmt*>(stmt.get())) {
                DocEntry entry;
                entry.name = v->getName();
                entry.kind = DocEntryKind::Variable;
                entry.line = static_cast<int>(v->getLocation().line);
                entry.returnType = v->getType();
                entry.signature = (v->isConst() ? "const " : "let ") + entry.name + ": " + entry.returnType;

                attachDoc(entry, findDocBlockForLine(docBlocks, entry.line));
                mod.entries.push_back(entry);
            }
        }
    }

    // If module doc comment was not assigned via module decl, use the first unused doc block
    if (mod.docComment.empty()) {
        for (const auto& b : docBlocks) {
            if (!b.used) {
                mod.docComment = b.cleanText;
                mod.brief = b.brief;
                break;
            }
        }
    }

    return mod;
}

DocModule DocGenerator::extractModuleFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return DocModule{};
    std::ostringstream ss;
    ss << file.rdbuf();
    return extractModule(ss.str(), filePath);
}

std::vector<DocModule> DocGenerator::extractModulesFromDirectory(const std::string& dirPath) {
    std::vector<DocModule> modules;
    std::error_code ec;

    for (const auto& entry : fs::recursive_directory_iterator(dirPath, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sp") {
            // Skip hidden folders and build folders
            std::string pStr = entry.path().string();
            if (pStr.find(".git") != std::string::npos ||
                pStr.find("build") != std::string::npos ||
                pStr.find(".spm") != std::string::npos) {
                continue;
            }
            modules.push_back(extractModuleFromFile(pStr));
        }
    }

    std::sort(modules.begin(), modules.end(), [](const DocModule& a, const DocModule& b) {
        return a.name < b.name;
    });

    return modules;
}

std::string DocGenerator::renderMarkdown(const DocModule& mod) const {
    std::ostringstream md;

    md << "# Module `" << mod.name << "`\n\n";
    if (!mod.docComment.empty()) {
        md << mod.docComment << "\n\n";
    }

    // Split entries by kind
    std::vector<DocEntry> classes;
    std::vector<DocEntry> interfaces;
    std::vector<DocEntry> structs;
    std::vector<DocEntry> enums;
    std::vector<DocEntry> functions;
    std::vector<DocEntry> variables;

    for (const auto& e : mod.entries) {
        switch (e.kind) {
            case DocEntryKind::Class: classes.push_back(e); break;
            case DocEntryKind::Interface: interfaces.push_back(e); break;
            case DocEntryKind::Struct: structs.push_back(e); break;
            case DocEntryKind::Enum: enums.push_back(e); break;
            case DocEntryKind::Function: functions.push_back(e); break;
            case DocEntryKind::Variable: variables.push_back(e); break;
            default: break;
        }
    }

    md << "## Table of Contents\n\n";
    if (!classes.empty()) {
        md << "- [Classes](#classes)\n";
        for (const auto& c : classes) md << "  - [`" << c.name << "`](#class-" << c.name << ")\n";
    }
    if (!interfaces.empty()) {
        md << "- [Interfaces](#interfaces)\n";
        for (const auto& i : interfaces) md << "  - [`" << i.name << "`](#interface-" << i.name << ")\n";
    }
    if (!structs.empty()) {
        md << "- [Structs](#structs)\n";
        for (const auto& s : structs) md << "  - [`" << s.name << "`](#struct-" << s.name << ")\n";
    }
    if (!enums.empty()) {
        md << "- [Enums](#enums)\n";
        for (const auto& en : enums) md << "  - [`" << en.name << "`](#enum-" << en.name << ")\n";
    }
    if (!functions.empty()) {
        md << "- [Functions](#functions)\n";
        for (const auto& f : functions) md << "  - [`" << f.name << "`](#fn-" << f.name << ")\n";
    }
    if (!variables.empty()) {
        md << "- [Variables & Constants](#variables--constants)\n";
    }
    md << "\n---\n\n";

    // Functions
    if (!functions.empty()) {
        md << "## Functions\n\n";
        for (const auto& f : functions) {
            md << "### `fn " << f.name << "`\n\n";
            md << "```sp\n" << f.signature << "\n```\n\n";
            if (!f.docComment.empty()) {
                md << f.docComment << "\n\n";
            }
            if (!f.params.empty()) {
                md << "**Parameters:**\n\n";
                for (const auto& p : f.params) {
                    md << "- `" << p.name << ": " << p.type << "`";
                    if (!p.description.empty()) md << " - " << p.description;
                    md << "\n";
                }
                md << "\n";
            }
            if (!f.returnType.empty() && f.returnType != "void") {
                md << "**Returns:** `" << f.returnType << "`";
                if (!f.returnDescription.empty()) md << " - " << f.returnDescription;
                md << "\n\n";
            }
            md << "---\n\n";
        }
    }

    // Classes
    if (!classes.empty()) {
        md << "## Classes\n\n";
        for (const auto& c : classes) {
            md << "### `class " << c.name << "`\n\n";
            md << "```sp\n" << c.signature << "\n```\n\n";
            if (!c.docComment.empty()) {
                md << c.docComment << "\n\n";
            }
            if (!c.members.empty()) {
                md << "#### Members\n\n";
                for (const auto& m : c.members) {
                    md << "- `" << m.signature << "`";
                    if (!m.brief.empty()) md << " - " << m.brief;
                    md << "\n";
                }
                md << "\n";
            }
            md << "---\n\n";
        }
    }

    // Interfaces
    if (!interfaces.empty()) {
        md << "## Interfaces\n\n";
        for (const auto& i : interfaces) {
            md << "### `interface " << i.name << "`\n\n";
            md << "```sp\n" << i.signature << "\n```\n\n";
            if (!i.docComment.empty()) md << i.docComment << "\n\n";
            if (!i.members.empty()) {
                md << "#### Methods\n\n";
                for (const auto& m : i.members) {
                    md << "- `" << m.signature << "`";
                    if (!m.brief.empty()) md << " - " << m.brief;
                    md << "\n";
                }
                md << "\n";
            }
            md << "---\n\n";
        }
    }

    // Structs
    if (!structs.empty()) {
        md << "## Structs\n\n";
        for (const auto& s : structs) {
            md << "### `struct " << s.name << "`\n\n";
            md << "```sp\n" << s.signature << "\n```\n\n";
            if (!s.docComment.empty()) md << s.docComment << "\n\n";
            if (!s.members.empty()) {
                md << "#### Fields\n\n";
                for (const auto& f : s.members) {
                    md << "- `" << f.signature << "`";
                    if (!f.brief.empty()) md << " - " << f.brief;
                    md << "\n";
                }
                md << "\n";
            }
            md << "---\n\n";
        }
    }

    // Enums
    if (!enums.empty()) {
        md << "## Enums\n\n";
        for (const auto& en : enums) {
            md << "### `enum " << en.name << "`\n\n";
            md << "```sp\n" << en.signature << "\n```\n\n";
            if (!en.docComment.empty()) md << en.docComment << "\n\n";
            if (!en.members.empty()) {
                md << "#### Variants\n\n";
                for (const auto& v : en.members) {
                    md << "- `" << v.name << "`\n";
                }
                md << "\n";
            }
            md << "---\n\n";
        }
    }

    // Variables & Constants
    if (!variables.empty()) {
        md << "## Variables & Constants\n\n";
        for (const auto& v : variables) {
            md << "- `" << v.signature << "`";
            if (!v.brief.empty()) md << " - " << v.brief;
            md << "\n";
        }
        md << "\n";
    }

    return md.str();
}

std::string DocGenerator::renderHtml(const DocModule& mod) const {
    std::ostringstream html;

    html << "<!DOCTYPE html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "  <meta charset=\"UTF-8\">\n"
         << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "  <title>Module " << escapeHtml(mod.name) << " - " << escapeHtml(m_options.projectName) << "</title>\n"
         << "  <style>\n"
         << "    :root {\n"
         << "      --bg-primary: #0f172a;\n"
         << "      --bg-secondary: #1e293b;\n"
         << "      --bg-code: #0b1120;\n"
         << "      --text-main: #f1f5f9;\n"
         << "      --text-muted: #94a3b8;\n"
         << "      --accent: #38bdf8;\n"
         << "      --accent-hover: #0ea5e9;\n"
         << "      --border: #334155;\n"
         << "      --badge-fn: #10b981;\n"
         << "      --badge-class: #6366f1;\n"
         << "      --badge-type: #f59e0b;\n"
         << "    }\n"
         << "    * { box-sizing: border-box; margin: 0; padding: 0; }\n"
         << "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: var(--bg-primary); color: var(--text-main); line-height: 1.6; display: flex; }\n"
         << "    .sidebar { width: 280px; height: 100vh; position: sticky; top: 0; background: var(--bg-secondary); border-right: 1px solid var(--border); padding: 1.5rem; overflow-y: auto; }\n"
         << "    .sidebar h2 { font-size: 1.1rem; color: var(--accent); margin-bottom: 1rem; text-transform: uppercase; letter-spacing: 0.05em; }\n"
         << "    .sidebar a { display: block; color: var(--text-muted); text-decoration: none; padding: 0.35rem 0.5rem; border-radius: 4px; font-size: 0.9rem; transition: background 0.15s, color 0.15s; }\n"
         << "    .sidebar a:hover { background: rgba(56, 189, 248, 0.1); color: var(--accent); }\n"
         << "    .content { flex: 1; max-width: 900px; padding: 2.5rem 3rem; }\n"
         << "    h1 { font-size: 2.25rem; font-weight: 700; color: #fff; margin-bottom: 0.5rem; }\n"
         << "    h2 { font-size: 1.5rem; font-weight: 600; color: #e2e8f0; margin: 2.5rem 0 1rem; border-bottom: 1px solid var(--border); padding-bottom: 0.5rem; }\n"
         << "    h3 { font-size: 1.2rem; font-weight: 600; color: var(--accent); margin-top: 1.5rem; }\n"
         << "    .doc-lead { font-size: 1.1rem; color: var(--text-muted); margin-bottom: 2rem; }\n"
         << "    .entry-card { background: var(--bg-secondary); border: 1px solid var(--border); border-radius: 8px; padding: 1.5rem; margin-bottom: 1.5rem; }\n"
         << "    pre { background: var(--bg-code); border: 1px solid var(--border); border-radius: 6px; padding: 1rem; overflow-x: auto; font-family: 'JetBrains Mono', Consolas, monospace; font-size: 0.9rem; color: #38bdf8; margin: 0.75rem 0; }\n"
         << "    .desc { color: #cbd5e1; margin: 0.75rem 0; }\n"
         << "    .badge { display: inline-block; padding: 0.2rem 0.5rem; border-radius: 9999px; font-size: 0.75rem; font-weight: 600; text-transform: uppercase; margin-right: 0.5rem; }\n"
         << "    .badge-fn { background: rgba(16, 185, 129, 0.2); color: var(--badge-fn); }\n"
         << "    .badge-class { background: rgba(99, 102, 241, 0.2); color: var(--badge-class); }\n"
         << "    .badge-type { background: rgba(245, 158, 11, 0.2); color: var(--badge-type); }\n"
         << "    table { width: 100%; border-collapse: collapse; margin-top: 0.75rem; font-size: 0.9rem; }\n"
         << "    th, td { text-align: left; padding: 0.5rem 0.75rem; border-bottom: 1px solid var(--border); }\n"
         << "    th { color: var(--text-muted); font-weight: 600; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "  <aside class=\"sidebar\">\n"
         << "    <h2>" << escapeHtml(m_options.projectName) << "</h2>\n"
         << "    <a href=\"index.html\">&larr; Module Index</a>\n"
         << "    <hr style=\"border: 0; border-top: 1px solid var(--border); margin: 1rem 0;\">\n"
         << "    <h2>On This Page</h2>\n";

    for (const auto& e : mod.entries) {
        html << "    <a href=\"#" << escapeHtml(e.name) << "\">" << escapeHtml(e.name) << "</a>\n";
    }

    html << "  </aside>\n"
         << "  <main class=\"content\">\n"
         << "    <h1>Module <code>" << escapeHtml(mod.name) << "</code></h1>\n";

    if (!mod.docComment.empty()) {
        html << "    <p class=\"doc-lead\">" << escapeHtml(mod.docComment) << "</p>\n";
    }

    // Render items
    for (const auto& e : mod.entries) {
        std::string badgeClass = (e.kind == DocEntryKind::Function || e.kind == DocEntryKind::Method) ? "badge-fn"
            : (e.kind == DocEntryKind::Class) ? "badge-class" : "badge-type";

        html << "    <div class=\"entry-card\" id=\"" << escapeHtml(e.name) << "\">\n"
             << "      <div>\n"
             << "        <span class=\"badge " << badgeClass << "\">" << docEntryKindToString(e.kind) << "</span>\n"
             << "        <h3 style=\"display: inline;\">" << escapeHtml(e.name) << "</h3>\n"
             << "      </div>\n"
             << "      <pre><code>" << escapeHtml(e.signature) << "</code></pre>\n";

        if (!e.docComment.empty()) {
            html << "      <p class=\"desc\">" << escapeHtml(e.docComment) << "</p>\n";
        }

        if (!e.params.empty()) {
            html << "      <table>\n"
                 << "        <thead><tr><th>Parameter</th><th>Type</th><th>Description</th></tr></thead>\n"
                 << "        <tbody>\n";
            for (const auto& p : e.params) {
                html << "          <tr><td><code>" << escapeHtml(p.name) << "</code></td><td><code>"
                     << escapeHtml(p.type) << "</code></td><td>" << escapeHtml(p.description) << "</td></tr>\n";
            }
            html << "        </tbody>\n      </table>\n";
        }

        if (!e.returnType.empty() && e.returnType != "void") {
            html << "      <p style=\"margin-top: 0.75rem;\"><strong>Returns:</strong> <code>"
                 << escapeHtml(e.returnType) << "</code>"
                 << (!e.returnDescription.empty() ? (" &mdash; " + escapeHtml(e.returnDescription)) : "")
                 << "</p>\n";
        }

        if (!e.members.empty()) {
            html << "      <h4 style=\"margin-top: 1rem; color: var(--text-muted); font-size: 0.9rem; text-transform: uppercase;\">Members</h4>\n"
                 << "      <ul style=\"list-style: none; margin-top: 0.5rem;\">\n";
            for (const auto& m : e.members) {
                html << "        <li style=\"margin-bottom: 0.4rem;\"><code>" << escapeHtml(m.signature) << "</code>";
                if (!m.brief.empty()) html << " &mdash; <span style=\"color: var(--text-muted);\">" << escapeHtml(m.brief) << "</span>";
                html << "</li>\n";
            }
            html << "      </ul>\n";
        }

        html << "    </div>\n";
    }

    html << "  </main>\n"
         << "</body>\n"
         << "</html>\n";

    return html.str();
}

std::string DocGenerator::renderIndexMarkdown(const std::vector<DocModule>& modules) const {
    std::ostringstream md;
    md << "# " << m_options.projectName << " Documentation\n\n";
    md << "Welcome to the SP API documentation.\n\n";
    md << "## Modules\n\n";
    for (const auto& mod : modules) {
        md << "### [`" << mod.name << "`](" << mod.name << ".md)\n\n";
        if (!mod.brief.empty()) {
            md << mod.brief << "\n\n";
        }
    }
    return md.str();
}

std::string DocGenerator::renderIndexHtml(const std::vector<DocModule>& modules) const {
    std::ostringstream html;

    html << "<!DOCTYPE html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "  <meta charset=\"UTF-8\">\n"
         << "  <title>" << escapeHtml(m_options.projectName) << " - API Reference</title>\n"
         << "  <style>\n"
         << "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0f172a; color: #f1f5f9; padding: 3rem; max-width: 900px; margin: 0 auto; }\n"
         << "    h1 { font-size: 2.5rem; color: #38bdf8; margin-bottom: 1rem; }\n"
         << "    .card { background: #1e293b; border: 1px solid #334155; border-radius: 8px; padding: 1.5rem; margin-bottom: 1rem; text-decoration: none; display: block; color: inherit; transition: border-color 0.15s; }\n"
         << "    .card:hover { border-color: #38bdf8; }\n"
         << "    .card h3 { color: #38bdf8; margin-bottom: 0.5rem; }\n"
         << "    .card p { color: #94a3b8; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "  <h1>" << escapeHtml(m_options.projectName) << "</h1>\n"
         << "  <p style=\"color: #94a3b8; margin-bottom: 2rem;\">Generated API Documentation for SP Language</p>\n";

    for (const auto& mod : modules) {
        html << "  <a class=\"card\" href=\"" << escapeHtml(mod.name) << ".html\">\n"
             << "    <h3>Module " << escapeHtml(mod.name) << "</h3>\n"
             << "    <p>" << (mod.brief.empty() ? "No description provided." : escapeHtml(mod.brief)) << "</p>\n"
             << "  </a>\n";
    }

    html << "</body>\n</html>\n";
    return html.str();
}

bool DocGenerator::generate(const std::string& inputPath, const std::string& outputDir) {
    std::string out = outputDir.empty() ? m_options.outputDir : outputDir;
    fs::path outPath(out);
    std::error_code ec;
    fs::create_directories(outPath, ec);

    std::vector<DocModule> modules;
    if (fs::is_directory(inputPath)) {
        modules = extractModulesFromDirectory(inputPath);
    } else if (fs::is_regular_file(inputPath)) {
        modules.push_back(extractModuleFromFile(inputPath));
    } else {
        std::cerr << "error: invalid doc input path " << inputPath << "\n";
        return false;
    }

    if (modules.empty()) {
        std::cout << "No SP source files found to document in " << inputPath << "\n";
        return true;
    }

    for (const auto& mod : modules) {
        if (m_options.generateMarkdown) {
            std::ofstream mdOut(outPath / (mod.name + ".md"));
            if (mdOut.is_open()) {
                mdOut << renderMarkdown(mod);
            }
        }
        if (m_options.generateHtml) {
            std::ofstream htmlOut(outPath / (mod.name + ".html"));
            if (htmlOut.is_open()) {
                htmlOut << renderHtml(mod);
            }
        }
    }

    // Index files
    if (m_options.generateMarkdown) {
        std::ofstream idxMd(outPath / "index.md");
        if (idxMd.is_open()) idxMd << renderIndexMarkdown(modules);
    }
    if (m_options.generateHtml) {
        std::ofstream idxHtml(outPath / "index.html");
        if (idxHtml.is_open()) idxHtml << renderIndexHtml(modules);
    }

    std::cout << "Successfully generated documentation for " << modules.size()
              << " module(s) in " << outPath.string() << "\n";
    return true;
}

} // namespace sp::doc
