#include "LspServer.h"
#include "sp/Version.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/lexer/Token.h"
#include "sp/lexer/TokenType.h"
#include "sp/parser/Parser.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Statement.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "fmt/Formatter.h"
#include "doc/DocGenerator.h"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>

namespace sp::lsp {

namespace {

const std::vector<std::pair<std::string, std::string>> SP_KEYWORDS = {
    {"fn", "Declare a function: `fn name(params): returnType { ... }`"},
    {"let", "Declare a mutable local variable: `let x = 10;`"},
    {"const", "Declare an immutable constant: `const PI = 3.14159;`"},
    {"class", "Declare an object-oriented class: `class Name { ... }`"},
    {"interface", "Declare an interface contract: `interface Name { ... }`"},
    {"struct", "Declare a lightweight struct: `struct Name { ... }`"},
    {"enum", "Declare an enumerated type: `enum Name { ... }`"},
    {"if", "Conditional branching statement"},
    {"else", "Else branch for conditional execution"},
    {"while", "Loop construct that runs while condition is true"},
    {"for", "Loop construct: `for item in collection { ... }`"},
    {"in", "Used in for..in iterations"},
    {"return", "Return value from a function"},
    {"match", "Pattern matching control flow expression"},
    {"switch", "Switch-case branching statement"},
    {"case", "Case branch within a switch statement"},
    {"default", "Default branch in a switch statement"},
    {"break", "Exit loop or switch statement"},
    {"continue", "Jump to next loop iteration"},
    {"import", "Import external module or package"},
    {"module", "Declare file module namespace"},
    {"public", "Public visibility specifier"},
    {"private", "Private visibility specifier"},
    {"override", "Override virtual method in derived class"},
    {"static", "Declare static method or member"},
    {"async", "Asynchronous function modifier"},
    {"await", "Await asynchronous task resolution"},
    {"assert", "Test assertion: `assert(cond, [msg])`"},
    {"new", "Instantiate new class object"},
    {"try", "Begin exception handling block"},
    {"catch", "Catch exception in try block"},
    {"finally", "Finally block executed always"},
    {"throw", "Throw an error or exception"},
    {"true", "Boolean true literal"},
    {"false", "Boolean false literal"},
    {"null", "Null pointer or reference literal"}
};

const std::vector<std::string> SP_PRIMITIVE_TYPES = {
    "int", "float", "string", "bool", "char", "byte", "void", "any", "Array", "Map", "List"
};

} // namespace

LspServer::LspServer(std::istream& in, std::ostream& out)
    : m_in(in), m_out(out) {}

std::string LspServer::frameMessage(const std::string& json) {
    return "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n" + json;
}

bool LspServer::parseHeader(std::istream& in, size_t& contentLength) {
    std::string line;
    contentLength = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) {
            return contentLength > 0;
        }
        if (line.starts_with("Content-Length:") || line.starts_with("content-length:")) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                try {
                    contentLength = std::stoull(line.substr(pos + 1));
                } catch (...) {
                    contentLength = 0;
                }
            }
        }
    }
    return false;
}

void LspServer::run() {
    while (m_running && m_in.good()) {
        size_t length = 0;
        if (!parseHeader(m_in, length)) break;
        if (length == 0) continue;

        std::string body(length, '\0');
        m_in.read(&body[0], length);

        std::string resp = handleMessage(body);
        if (!resp.empty()) {
            m_out << frameMessage(resp) << std::flush;
        }
    }
}

void LspServer::sendNotification(const std::string& method, const JsonValue& params) {
    JsonValue msg = JsonValue::object();
    msg["jsonrpc"] = "2.0";
    msg["method"] = method;
    msg["params"] = params;
    std::string serialized = msg.serialize();
    m_out << frameMessage(serialized) << std::flush;
}

void LspServer::sendResponse(const JsonValue& id, const JsonValue& result) {
    JsonValue msg = JsonValue::object();
    msg["jsonrpc"] = "2.0";
    msg["id"] = id;
    msg["result"] = result;
    std::string serialized = msg.serialize();
    m_out << frameMessage(serialized) << std::flush;
}

void LspServer::sendError(const JsonValue& id, int code, const std::string& message) {
    JsonValue msg = JsonValue::object();
    msg["jsonrpc"] = "2.0";
    msg["id"] = id;
    JsonValue err = JsonValue::object();
    err["code"] = code;
    err["message"] = message;
    msg["error"] = err;
    std::string serialized = msg.serialize();
    m_out << frameMessage(serialized) << std::flush;
}

std::string LspServer::handleMessage(const std::string& rawMessage) {
    JsonValue req = JsonValue::parse(rawMessage);
    if (!req.isObject()) return "";

    std::string method = req["method"].asString("");
    JsonValue id = req["id"];
    JsonValue params = req["params"];

    // Requests (with id)
    if (!id.isNull()) {
        // Phase 4 slice B: after shutdown only `shutdown` itself is
        // answered; anything else is an invalid request until `exit`.
        if (m_shutdownRequested && method != "shutdown") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            JsonValue err = JsonValue::object();
            err["code"] = -32600;
            err["message"] = "Invalid Request: server shut down";
            resp["error"] = err;
            return resp.serialize();
        }
        if (method == "initialize") {
            JsonValue res = handleInitialize(id, params);
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = res;
            return resp.serialize();
        }
        if (method == "shutdown") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleShutdown(id);
            return resp.serialize();
        }
        if (method == "textDocument/completion") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleCompletion(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/hover") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleHover(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/definition") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleDefinition(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/formatting") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleFormatting(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/rename") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleRename(id, params);
            return resp.serialize();
        }
        if (method == "workspace/symbol") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleWorkspaceSymbol(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/inlayHint") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleInlayHint(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/references") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleReferences(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/documentSymbol") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleDocumentSymbol(id, params);
            return resp.serialize();
        }
        if (method == "textDocument/semanticTokens/full") {
            JsonValue resp = JsonValue::object();
            resp["jsonrpc"] = "2.0";
            resp["id"] = id;
            resp["result"] = handleSemanticTokens(id, params);
            return resp.serialize();
        }

        // Default empty result for unsupported requests
        JsonValue resp = JsonValue::object();
        resp["jsonrpc"] = "2.0";
        resp["id"] = id;
        resp["result"] = JsonValue{};
        return resp.serialize();
    }

    // Notifications (without id)
    if (method == "initialized") {
        m_initialized = true;
    } else if (method == "exit") {
        m_running = false;
    } else if (method == "textDocument/didOpen") {
        handleDidOpen(params);
    } else if (method == "textDocument/didChange") {
        handleDidChange(params);
    } else if (method == "textDocument/didClose") {
        handleDidClose(params);
    } else if (method == "workspace/didChangeWatchedFiles") {
        handleDidChangeWatchedFiles(params);
    }

    return "";
}

JsonValue LspServer::handleInitialize(const JsonValue&, const JsonValue& params) {
    // Phase 4 slice B: remember the workspace root. The VS Code client
    // already sends rootUri/workspaceFolders and watches **/*.sp, but
    // the server used to ignore both, so workspace features only ever
    // saw open documents.
    if (params.hasKey("rootUri") && !params["rootUri"].isNull()) {
        m_rootUri = params["rootUri"].asString();
    } else if (params.hasKey("workspaceFolders") && params["workspaceFolders"].isArray() &&
               !params["workspaceFolders"].arrVal.empty()) {
        m_rootUri = params["workspaceFolders"].arrVal[0]["uri"].asString();
    }
    if (!m_rootUri.empty()) {
        indexWorkspace(m_rootUri);
    }

    JsonValue result = JsonValue::object();
    JsonValue caps = JsonValue::object();

    // Full document sync
    caps["textDocumentSync"] = 1;

    // Completion provider
    JsonValue compProv = JsonValue::object();
    compProv["resolveProvider"] = false;
    JsonValue trig = JsonValue::array();
    trig.push_back(".");
    trig.push_back(":");
    trig.push_back(">");
    compProv["triggerCharacters"] = trig;
    caps["completionProvider"] = compProv;

    caps["hoverProvider"] = true;
    caps["definitionProvider"] = true;
    caps["documentFormattingProvider"] = true;
    caps["renameProvider"] = true;
    caps["workspaceSymbolProvider"] = true;
    caps["inlayHintProvider"] = true;
    // Phase 4 slice B: newly implemented providers.
    caps["referencesProvider"] = true;
    caps["documentSymbolProvider"] = true;
    {
        JsonValue semProv = JsonValue::object();
        JsonValue legend = JsonValue::object();
        JsonValue types = JsonValue::array();
        // Order is the legend: handleSemanticTokens uses these indices.
        for (const char* t : {"namespace", "type", "class", "enum", "interface", "struct",
                              "typeParameter", "parameter", "variable", "property", "enumMember",
                              "function", "method", "keyword", "comment", "string", "number",
                              "operator"}) {
            types.push_back(t);
        }
        legend["tokenTypes"] = types;
        JsonValue mods = JsonValue::array();
        for (const char* m : {"declaration", "definition", "readonly", "static", "documentation"}) {
            mods.push_back(m);
        }
        legend["tokenModifiers"] = mods;
        semProv["legend"] = legend;
        semProv["range"] = false;
        semProv["full"] = true;
        caps["semanticTokensProvider"] = semProv;
    }

    result["capabilities"] = caps;

    JsonValue serverInfo = JsonValue::object();
    serverInfo["name"] = "spls";
    serverInfo["version"] = std::string(sp::SP_VERSION_STRING);
    result["serverInfo"] = serverInfo;

    return result;
}

JsonValue LspServer::handleShutdown(const JsonValue&) {
    // Phase 4 slice B: do NOT stop here — the spec says the server
    // exits on the `exit` notification, not on `shutdown`.
    m_shutdownRequested = true;
    return JsonValue{};
}

void LspServer::openDocument(const std::string& uri, const std::string& text) {
    m_documents[uri] = text;
}

void LspServer::updateDocument(const std::string& uri, const std::string& text) {
    m_documents[uri] = text;
}

void LspServer::closeDocument(const std::string& uri) {
    m_documents.erase(uri);
}

std::string LspServer::getDocument(const std::string& uri) const {
    auto it = m_documents.find(uri);
    return it != m_documents.end() ? it->second : "";
}

void LspServer::handleDidOpen(const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    std::string text = params["textDocument"]["text"].asString();
    openDocument(uri, text);
    publishDiagnostics(uri);
}

void LspServer::applyIncrementalEdit(const std::string& uri, const Range& range, const std::string& text) {
    auto it = m_documents.find(uri);
    if (it == m_documents.end()) {
        m_documents[uri] = text;
        return;
    }

    const std::string& doc = it->second;
    auto positionToOffset = [&](const Position& pos) -> size_t {
        size_t currentLine = 0;
        size_t currentOffset = 0;
        while (currentOffset < doc.size() && currentLine < static_cast<size_t>(pos.line)) {
            if (doc[currentOffset] == '\n') {
                currentLine++;
            }
            currentOffset++;
        }
        size_t targetOffset = currentOffset + static_cast<size_t>(pos.character);
        return std::min(targetOffset, doc.size());
    };

    size_t startOffset = positionToOffset(range.start);
    size_t endOffset = positionToOffset(range.end);
    if (startOffset > endOffset) std::swap(startOffset, endOffset);

    std::string newDoc = doc.substr(0, startOffset) + text + doc.substr(endOffset);
    m_documents[uri] = std::move(newDoc);
}

void LspServer::handleDidChange(const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    const auto& changes = params["contentChanges"];
    if (changes.isArray() && !changes.arrVal.empty()) {
        for (const auto& ch : changes.arrVal) {
            if (ch.isObject() && ch.hasKey("range") && !ch["range"].isNull()) {
                Range r = Range::fromJson(ch["range"]);
                std::string txt = ch["text"].asString();
                applyIncrementalEdit(uri, r, txt);
            } else if (ch.isObject() && ch.hasKey("text")) {
                std::string txt = ch["text"].asString();
                updateDocument(uri, txt);
            }
        }
        publishDiagnostics(uri);
    }
}

void LspServer::handleDidClose(const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    closeDocument(uri);
}

std::vector<Diagnostic> LspServer::computeDiagnostics(const std::string& uri, const std::string& source) {
    std::vector<Diagnostic> diagnosticsList;
    std::ostringstream diagStream;
    diagnostics::DiagnosticEngine diagEngine(diagStream);

    lexer::Lexer lexer(source, uri, &diagEngine);
    auto tokens = lexer.tokenizeAll();

    if (!diagEngine.hasErrors()) {
        parser::Parser parser(tokens, uri, &diagEngine);
        auto program = parser.parseProgram();

        if (program && !diagEngine.hasErrors()) {
            sema::SemanticAnalyzer sema(diagEngine);
            sema.analyze(*program);
        }
    }

    for (const auto& d : diagEngine.getDiagnostics()) {
        Diagnostic diag;
        int l = std::max(0, static_cast<int>(d.location.line) - 1);
        int c = std::max(0, static_cast<int>(d.location.column) - 1);
        diag.range.start = Position{l, c};
        diag.range.end = Position{l, c + 1};
        diag.severity = (d.severity == diagnostics::DiagnosticSeverity::Error) ? 1 : 2;
        diag.code = d.code;
        diag.source = "spls";
        diag.message = d.message;
        if (!d.hint.empty()) {
            diag.message += " (" + d.hint + ")";
        }
        diagnosticsList.push_back(diag);
    }

    return diagnosticsList;
}

void LspServer::publishDiagnostics(const std::string& uri) {
    std::string source = getDocument(uri);
    auto diags = computeDiagnostics(uri, source);

    JsonValue params = JsonValue::object();
    params["uri"] = uri;
    JsonValue diagArr = JsonValue::array();
    for (const auto& d : diags) {
        diagArr.push_back(d.toJson());
    }
    params["diagnostics"] = diagArr;

    sendNotification("textDocument/publishDiagnostics", params);
}

JsonValue LspServer::handleCompletion(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    Position pos = Position::fromJson(params["position"]);
    std::string source = getDocument(uri);

    JsonValue items = JsonValue::array();

    // 1. Keywords
    for (const auto& [kw, doc] : SP_KEYWORDS) {
        CompletionItem item;
        item.label = kw;
        item.kind = 14; // Keyword
        item.detail = "SP keyword";
        item.documentation = doc;
        items.push_back(item.toJson());
    }

    // 2. Primitive Types
    for (const auto& typeName : SP_PRIMITIVE_TYPES) {
        CompletionItem item;
        item.label = typeName;
        item.kind = 25; // TypeParameter / Class
        item.detail = "Built-in SP type";
        items.push_back(item.toJson());
    }

    // Phase 4 slice B: index-based completions. After `.`, only the
    // receiver's members are offered; otherwise the symbols visible at
    // the cursor (locals, parameters, top-level declarations). The old
    // unfiltered DocGenerator list below runs only when the index is
    // invalid (unparseable document).
    {
        SymbolIndex index = buildIndex(uri);
        if (index.valid()) {
            // Member context? Look at the character before the cursor.
            std::string lineText;
            {
                std::istringstream ls(source);
                std::string l;
                int n = 0;
                while (std::getline(ls, l)) {
                    if (n == pos.line) { lineText = l; break; }
                    ++n;
                }
            }
            int cursor = std::min(pos.character, static_cast<int>(lineText.size()));
            int i = cursor - 1;
            while (i >= 0 && lineText[i] == ' ') --i;
            if (i >= 0 && lineText[i] == '.') {
                int recvEnd = i;
                int j = recvEnd - 1;
                while (j >= 0 && (std::isalnum(static_cast<unsigned char>(lineText[j])) || lineText[j] == '_')) --j;
                std::string receiver = lineText.substr(static_cast<size_t>(j + 1),
                                                       static_cast<size_t>(recvEnd - j - 1));
                std::string className;
                for (const Symbol* s : index.visibleAt(pos.line, pos.character)) {
                    if (s->name == receiver) {
                        if (s->kind == SymKind::Class || s->kind == SymKind::Struct ||
                            s->kind == SymKind::Interface || s->kind == SymKind::Enum) {
                            className = s->name;
                        } else {
                            className = s->type;
                        }
                        break;
                    }
                }
                if (className.empty() && index.findTopLevel(receiver)) {
                    className = receiver; // static access: `ClassName.member`
                }
                if (!className.empty()) {
                    JsonValue members = JsonValue::array();
                    for (const auto& s : index.symbols()) {
                        if (s.parent != className) continue;
                        if (s.kind != SymKind::Method && s.kind != SymKind::Field) continue;
                        CompletionItem item;
                        item.label = s.name;
                        item.kind = (s.kind == SymKind::Method) ? 2 : 5;
                        item.detail = s.signature;
                        members.push_back(item.toJson());
                    }
                    return members;
                }
                // Member context but the receiver is unknown: fall
                // through to the general list rather than guessing.
            }
            for (const Symbol* s : index.visibleAt(pos.line, pos.character)) {
                CompletionItem item;
                item.label = s->name;
                item.kind = symKindToLsp(s->kind);
                item.detail = s->signature;
                items.push_back(item.toJson());
            }
        }
    }

    // 3. Document symbols
    if (!source.empty() && !buildIndex(uri).valid()) {
        doc::DocGenerator docGen;
        doc::DocModule mod = docGen.extractModule(source, uri);
        for (const auto& entry : mod.entries) {
            CompletionItem item;
            item.label = entry.name;
            item.detail = entry.signature;
            item.documentation = entry.docComment;
            switch (entry.kind) {
                case doc::DocEntryKind::Function: item.kind = 3; break;
                case doc::DocEntryKind::Class: item.kind = 7; break;
                case doc::DocEntryKind::Interface: item.kind = 8; break;
                case doc::DocEntryKind::Struct: item.kind = 22; break;
                case doc::DocEntryKind::Enum: item.kind = 13; break;
                case doc::DocEntryKind::Variable: item.kind = 6; break;
                default: item.kind = 1; break;
            }
            items.push_back(item.toJson());

            // Member completions
            for (const auto& mem : entry.members) {
                CompletionItem mItem;
                mItem.label = mem.name;
                mItem.detail = mem.signature;
                mItem.documentation = mem.docComment;
                mItem.kind = (mem.kind == doc::DocEntryKind::Method) ? 2 : 5;
                items.push_back(mItem.toJson());
            }
        }
    }

    return items;
}

std::string LspServer::extractWordAtPosition(const std::string& text, int line, int character) {
    std::istringstream stream(text);
    std::string lineStr;
    int curLine = 0;
    while (std::getline(stream, lineStr)) {
        if (curLine == line) break;
        curLine++;
    }

    if (curLine != line || lineStr.empty()) return "";
    if (character >= static_cast<int>(lineStr.size())) {
        character = static_cast<int>(lineStr.size()) - 1;
    }
    if (character < 0) return "";

    auto isWordChar = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    };

    if (!isWordChar(lineStr[character])) {
        // Try one character before cursor
        if (character > 0 && isWordChar(lineStr[character - 1])) {
            character--;
        } else {
            return "";
        }
    }

    int start = character;
    while (start > 0 && isWordChar(lineStr[start - 1])) start--;

    int end = character;
    while (end + 1 < static_cast<int>(lineStr.size()) && isWordChar(lineStr[end + 1])) end++;

    return lineStr.substr(start, end - start + 1);
}

JsonValue LspServer::handleHover(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    Position pos = Position::fromJson(params["position"]);
    std::string source = getDocument(uri);

    std::string word = extractWordAtPosition(source, pos.line, pos.character);
    if (word.empty()) return JsonValue{};

    // Phase 4 slice B: AST index first. This is what resolves locals,
    // parameters and members, which the DocGenerator path below cannot
    // see (it only knows top-level declarations).
    {
        SymbolIndex index = buildIndex(uri);
        if (index.valid()) {
            if (const Symbol* sym = index.resolveAt(pos.line, pos.character)) {
                Hover h;
                h.contents = hoverTextFor(*sym);
                // The range must cover the hovered word (the use site),
                // not the declaration.
                RefPos r;
                if (index.rangeAt(pos.line, pos.character, r)) {
                    h.range.start = Position{r.line, r.character};
                    h.range.end = Position{r.line, r.endCharacter};
                    h.hasRange = true;
                }
                return h.toJson();
            }
        }
    }

    // 1. Check Keywords
    for (const auto& [kw, doc] : SP_KEYWORDS) {
        if (kw == word) {
            Hover h;
            h.contents = "**Keyword** `" + kw + "`\n\n" + doc;
            return h.toJson();
        }
    }

    // 2. Check Primitive Types
    for (const auto& typeName : SP_PRIMITIVE_TYPES) {
        if (typeName == word) {
            Hover h;
            h.contents = "**Type** `" + typeName + "`\n\nBuilt-in primitive type in SP.";
            return h.toJson();
        }
    }

    // 3. Check Document Symbols
    doc::DocGenerator docGen;
    doc::DocModule mod = docGen.extractModule(source, uri);
    for (const auto& entry : mod.entries) {
        if (entry.name == word) {
            Hover h;
            std::ostringstream md;
            md << "```sp\n" << entry.signature << "\n```\n";
            if (!entry.docComment.empty()) {
                md << "\n" << entry.docComment << "\n";
            }
            h.contents = md.str();
            return h.toJson();
        }
        for (const auto& mem : entry.members) {
            if (mem.name == word) {
                Hover h;
                std::ostringstream md;
                md << "```sp\n" << mem.signature << "\n```\n";
                if (!mem.docComment.empty()) {
                    md << "\n" << mem.docComment << "\n";
                }
                h.contents = md.str();
                return h.toJson();
            }
        }
    }

    return JsonValue{};
}

JsonValue LspServer::handleDefinition(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    Position pos = Position::fromJson(params["position"]);
    std::string source = getDocument(uri);

    std::string word = extractWordAtPosition(source, pos.line, pos.character);
    if (word.empty()) return JsonValue{};

    // Phase 4 slice B: AST index first (locals/params/members).
    {
        SymbolIndex index = buildIndex(uri);
        if (index.valid()) {
            if (const Symbol* sym = index.resolveAt(pos.line, pos.character)) {
                Location loc;
                loc.uri = uri;
                loc.range.start = Position{sym->line - 1, sym->col - 1};
                loc.range.end = Position{sym->line - 1, sym->col - 1 + static_cast<int>(sym->name.size())};
                return loc.toJson();
            }
        }
    }

    doc::DocGenerator docGen;
    doc::DocModule mod = docGen.extractModule(source, uri);
    for (const auto& entry : mod.entries) {
        if (entry.name == word) {
            Location loc;
            loc.uri = uri;
            int l = std::max(0, entry.line - 1);
            loc.range.start = Position{l, 0};
            loc.range.end = Position{l, static_cast<int>(word.size())};
            return loc.toJson();
        }
        for (const auto& mem : entry.members) {
            if (mem.name == word) {
                Location loc;
                loc.uri = uri;
                int l = std::max(0, mem.line - 1);
                loc.range.start = Position{l, 0};
                loc.range.end = Position{l, static_cast<int>(word.size())};
                return loc.toJson();
            }
        }
    }

    return JsonValue{};
}

JsonValue LspServer::handleFormatting(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    std::string source = getDocument(uri);
    if (source.empty()) return JsonValue::array();

    fmt::FormatOptions opts;
    fmt::Formatter formatter(opts);
    std::string formatted = formatter.format(source);

    // Calculate document end line and character
    int lineCount = 0;
    int lastLineLen = 0;
    for (char c : source) {
        if (c == '\n') {
            lineCount++;
            lastLineLen = 0;
        } else {
            lastLineLen++;
        }
    }

    TextEdit edit;
    edit.range.start = Position{0, 0};
    edit.range.end = Position{lineCount, lastLineLen};
    edit.newText = formatted;

    JsonValue arr = JsonValue::array();
    arr.push_back(edit.toJson());
    return arr;
}

JsonValue LspServer::handleRename(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    Position pos = Position::fromJson(params["position"]);
    std::string newName = params["newName"].asString();
    std::string source = getDocument(uri);

    std::string word = extractWordAtPosition(source, pos.line, pos.character);
    if (word.empty() || newName.empty()) return JsonValue{};

    // Phase 4 slice B: scope-aware rename via the symbol index. The
    // old token scan below renamed every same-named identifier in the
    // file, including variables in OTHER functions.
    {
        SymbolIndex index = buildIndex(uri);
        if (index.valid()) {
            if (const Symbol* sym = index.resolveAt(pos.line, pos.character)) {
                JsonValue edits = JsonValue::array();
                for (const RefPos& r : index.referencesOf(sym)) {
                    TextEdit edit;
                    edit.range.start = Position{r.line, r.character};
                    edit.range.end = Position{r.line, r.endCharacter};
                    edit.newText = newName;
                    edits.push_back(edit.toJson());
                }
                JsonValue workspaceEdit = JsonValue::object();
                JsonValue changes = JsonValue::object();
                changes[uri] = edits;
                workspaceEdit["changes"] = changes;
                return workspaceEdit;
            }
            // Unresolved name: fall through to the token scan.
        }
    }

    // Tokenize document and find all identifier tokens matching word
    std::ostringstream diagStream;
    diagnostics::DiagnosticEngine diagEngine(diagStream);
    lexer::Lexer lexer(source, uri, &diagEngine);
    lexer.setPreserveComments(false);
    auto tokens = lexer.tokenizeAll();

    JsonValue edits = JsonValue::array();
    for (const auto& tok : tokens) {
        if (tok.type == lexer::TokenType::Identifier && tok.lexeme == word) {
            TextEdit edit;
            int l = std::max(0, static_cast<int>(tok.location.line) - 1);
            int c = std::max(0, static_cast<int>(tok.location.column) - 1);
            edit.range.start = Position{l, c};
            edit.range.end = Position{l, c + static_cast<int>(word.size())};
            edit.newText = newName;
            edits.push_back(edit.toJson());
        }
    }

    JsonValue workspaceEdit = JsonValue::object();
    JsonValue changes = JsonValue::object();
    changes[uri] = edits;
    workspaceEdit["changes"] = changes;

    return workspaceEdit;
}

JsonValue LspServer::handleWorkspaceSymbol(const JsonValue&, const JsonValue& params) {
    std::string query = params["query"].asString("");
    std::string qLower = query;
    std::transform(qLower.begin(), qLower.end(), qLower.begin(), [](unsigned char c) { return std::tolower(c); });

    JsonValue symbols = JsonValue::array();
    std::set<std::string> seen; // uri|name|line — dedupe open doc vs cache
    auto addSymbol = [&](const std::string& name, int kind, const std::string& uri,
                         int line, int c0, int c1, const std::string& container) {
        std::string sLower = name;
        std::transform(sLower.begin(), sLower.end(), sLower.begin(), [](unsigned char c) { return std::tolower(c); });
        if (!qLower.empty() && sLower.find(qLower) == std::string::npos) return;
        std::string key = uri + "|" + name + "|" + std::to_string(line);
        if (!seen.insert(key).second) return;
        SymbolInformation sym;
        sym.name = name;
        sym.kind = kind;
        sym.location.uri = uri;
        sym.location.range.start = Position{line, c0};
        sym.location.range.end = Position{line, c1};
        sym.containerName = container;
        symbols.push_back(sym.toJson());
    };

    // Open documents: AST index (accurate) with the regex scan as
    // fallback for unparseable files.
    for (const auto& [uri, text] : m_documents) {
        SymbolIndex index = buildIndex(uri);
        if (index.valid()) {
            for (const auto& sym : index.symbols()) {
                addSymbol(sym.name, symKindToLsp(sym.kind), uri, sym.line - 1, sym.col - 1,
                          sym.col - 1 + static_cast<int>(sym.name.size()), sym.parent);
            }
        } else {
            searchTextForSymbols(uri, text, qLower, symbols, seen);
        }
    }

    // Phase 4 slice B: workspace files on disk (indexed at initialize,
    // kept fresh by didChangeWatchedFiles). Open documents win on
    // overlap; the cache is what makes workspaceSymbol find symbols in
    // files the user has never opened.
    for (const auto& [uri, text] : m_workspaceFiles) {
        if (m_documents.count(uri)) continue;
        SymbolIndex index;
        if (index.build(text, uri)) {
            for (const auto& sym : index.symbols()) {
                addSymbol(sym.name, symKindToLsp(sym.kind), uri, sym.line - 1, sym.col - 1,
                          sym.col - 1 + static_cast<int>(sym.name.size()), sym.parent);
            }
        } else {
            searchTextForSymbols(uri, text, qLower, symbols, seen);
        }
    }

    return symbols;
}

JsonValue LspServer::handleInlayHint(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    auto docIt = m_documents.find(uri);
    if (docIt == m_documents.end()) {
        return JsonValue::array();
    }

    // Phase 4 slice B: real sema-inferred types from the index first.
    // The regex fallback below only ever saw literals ("any" for every
    // call result, constructor, or method call).
    {
        SymbolIndex index = buildIndex(uri);
        if (index.valid()) {
            JsonValue hints = JsonValue::array();
            int startLine = 0;
            int endLine = 1 << 20;
            if (params.hasKey("range") && !params["range"].isNull()) {
                startLine = static_cast<int>(params["range"]["start"]["line"].asInt(0));
                endLine = static_cast<int>(params["range"]["end"]["line"].asInt(0));
            }
            for (const LetHint& hint : index.letHints()) {
                if (hint.line < startLine || hint.line > endLine) continue;
                InlayHint h;
                h.position.line = hint.line;
                h.position.character = hint.character;
                h.label = ": " + hint.type;
                h.kind = 1; // Type
                h.paddingLeft = false;
                h.paddingRight = true;
                hints.push_back(h.toJson());
            }
            return hints;
        }
    }

    JsonValue hints = JsonValue::array();
    const std::string& text = docIt->second;
    std::istringstream stream(text);
    std::string line;
    int lineNum = 0;

    while (std::getline(stream, line)) {
        // Look for `let <name> = <val>;` without colon `: <type>`
        size_t letPos = line.find("let ");
        if (letPos != std::string::npos && (letPos == 0 || std::isspace(static_cast<unsigned char>(line[letPos - 1])))) {
            size_t nameStart = letPos + 4;
            while (nameStart < line.size() && std::isspace(static_cast<unsigned char>(line[nameStart]))) {
                nameStart++;
            }
            size_t nameEnd = nameStart;
            while (nameEnd < line.size() && (std::isalnum(static_cast<unsigned char>(line[nameEnd])) || line[nameEnd] == '_')) {
                nameEnd++;
            }
            if (nameEnd > nameStart) {
                // Check if already has explicit type annotation e.g. `let x: int = ...`
                size_t afterName = nameEnd;
                while (afterName < line.size() && std::isspace(static_cast<unsigned char>(line[afterName]))) {
                    afterName++;
                }
                if (afterName < line.size() && line[afterName] == '=') {
                    // Type was omitted! Infer from initializer
                    size_t eqPos = afterName + 1;
                    while (eqPos < line.size() && std::isspace(static_cast<unsigned char>(line[eqPos]))) {
                        eqPos++;
                    }
                    std::string inferred = "any";
                    if (eqPos < line.size()) {
                        char firstChar = line[eqPos];
                        if (firstChar == '"' || firstChar == '\'') {
                            inferred = "string";
                        } else if (line.compare(eqPos, 4, "true") == 0 || line.compare(eqPos, 5, "false") == 0) {
                            inferred = "bool";
                        } else if (std::isdigit(static_cast<unsigned char>(firstChar)) || firstChar == '-') {
                            size_t dotPos = line.find('.', eqPos);
                            size_t semiPos = line.find(';', eqPos);
                            if (dotPos != std::string::npos && (semiPos == std::string::npos || dotPos < semiPos)) {
                                inferred = "float";
                            } else {
                                inferred = "int";
                            }
                        } else if (firstChar == '[') {
                            inferred = "Array";
                        } else if (firstChar == '{') {
                            inferred = "Map";
                        }
                    }

                    InlayHint hint;
                    hint.position.line = lineNum;
                    hint.position.character = static_cast<int>(nameEnd);
                    hint.label = ": " + inferred;
                    hint.kind = 1; // Type
                    hint.paddingLeft = false;
                    hint.paddingRight = true;
                    hints.push_back(hint.toJson());
                }
            }
        }
        lineNum++;
    }

    return hints;
}

// ---------------------------------------------------------------------
// Phase 4 slice B helpers and new handlers
// ---------------------------------------------------------------------

SymbolIndex LspServer::buildIndex(const std::string& uri) const {
    SymbolIndex index;
    std::string source = getDocument(uri);
    if (!source.empty()) {
        index.build(source, uri);
    }
    return index;
}

std::string LspServer::symKindLabel(SymKind kind) {
    switch (kind) {
        case SymKind::Function: return "function";
        case SymKind::Method: return "method";
        case SymKind::Class: return "class";
        case SymKind::Interface: return "interface";
        case SymKind::Struct: return "struct";
        case SymKind::Enum: return "enum";
        case SymKind::EnumVariant: return "enum variant";
        case SymKind::Variable: return "variable";
        case SymKind::Parameter: return "parameter";
        case SymKind::Field: return "field";
    }
    return "symbol";
}

int LspServer::symKindToLsp(SymKind kind) {
    // LSP SymbolKind: 5 Class, 6 Method, 8 Field, 10 Enum, 11 Interface,
    // 12 Function, 13 Variable, 22 EnumMember, 23 Struct.
    switch (kind) {
        case SymKind::Function: return 12;
        case SymKind::Method: return 6;
        case SymKind::Class: return 5;
        case SymKind::Interface: return 11;
        case SymKind::Struct: return 23;
        case SymKind::Enum: return 10;
        case SymKind::EnumVariant: return 22;
        case SymKind::Variable: return 13;
        case SymKind::Parameter: return 13;
        case SymKind::Field: return 8;
    }
    return 13;
}

std::string LspServer::hoverTextFor(const Symbol& sym) {
    std::ostringstream md;
    md << "```sp\n" << sym.signature << "\n```\n";
    md << "\n" << symKindLabel(sym.kind) << " `" << sym.name << "`";
    if (!sym.parent.empty()) {
        md << " of `" << sym.parent << "`";
    }
    md << "\n";
    if (!sym.docComment.empty()) {
        md << "\n" << sym.docComment << "\n";
    }
    return md.str();
}

std::string LspServer::uriToPath(const std::string& uri) {
    if (uri.rfind("file://", 0) != 0) return "";
    std::string path = uri.substr(7);
    std::string out;
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '%' && i + 2 < path.size() &&
            std::isxdigit(static_cast<unsigned char>(path[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(path[i + 2]))) {
            int v = std::stoi(path.substr(i + 1, 2), nullptr, 16);
            out.push_back(static_cast<char>(v));
            i += 2;
        } else {
            out.push_back(path[i]);
        }
    }
    return out;
}

std::string LspServer::readFileText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void LspServer::indexWorkspace(const std::string& rootUri) {
    m_workspaceFiles.clear();
    std::string root = uriToPath(rootUri);
    if (root.empty()) return;
    std::error_code ec;
    std::filesystem::path rootPath(root);
    size_t count = 0;
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(rootPath,
             std::filesystem::directory_options::skip_permission_denied, ec)) {
        if (ec) break;
        if (count >= 500) break; // bounded: a workspace scan must stay cheap
        if (!entry.is_regular_file(ec)) continue;
        if (entry.path().extension() != ".sp") continue;
        std::filesystem::path rel = entry.path().lexically_relative(rootPath);
        bool skip = false;
        for (const auto& part : rel) {
            std::string p = part.string();
            if (p == ".git" || p == "node_modules" || p == "target" || p == "build" ||
                (!p.empty() && p[0] == '.')) {
                skip = true;
                break;
            }
        }
        if (skip) continue;
        std::string text = readFileText(entry.path().string());
        if (text.empty()) continue;
        m_workspaceFiles["file://" + entry.path().string()] = text;
        ++count;
    }
}

void LspServer::handleDidChangeWatchedFiles(const JsonValue& params) {
    const auto& changes = params["changes"];
    if (!changes.isArray()) return;
    for (const auto& ch : changes.arrVal) {
        std::string uri = ch["uri"].asString();
        int type = static_cast<int>(ch["type"].asInt(1));
        if (type == 3) { // Deleted
            m_workspaceFiles.erase(uri);
        } else { // Created (1) or Changed (2): re-read from disk
            std::string text = readFileText(uriToPath(uri));
            if (!text.empty()) {
                m_workspaceFiles[uri] = text;
            } else {
                m_workspaceFiles.erase(uri);
            }
        }
    }
}

void LspServer::searchTextForSymbols(const std::string& uri, const std::string& text,
                                     const std::string& qLower, JsonValue& out,
                                     std::set<std::string>& seen) const {
    // The pre-existing regex line scan, kept as the fallback for files
    // that do not parse (the AST index is preferred wherever it works).
    std::istringstream stream(text);
    std::string line;
    int lineNum = 0;
    auto findSymbol = [&](const std::string& keyword, int kind) {
        size_t pos = line.find(keyword);
        if (pos != std::string::npos && (pos == 0 || std::isspace(static_cast<unsigned char>(line[pos - 1])))) {
            size_t nameStart = pos + keyword.size();
            while (nameStart < line.size() && std::isspace(static_cast<unsigned char>(line[nameStart]))) {
                nameStart++;
            }
            size_t nameEnd = nameStart;
            while (nameEnd < line.size() && (std::isalnum(static_cast<unsigned char>(line[nameEnd])) || line[nameEnd] == '_')) {
                nameEnd++;
            }
            if (nameEnd > nameStart) {
                std::string symName = line.substr(nameStart, nameEnd - nameStart);
                std::string sLower = symName;
                std::transform(sLower.begin(), sLower.end(), sLower.begin(), [](unsigned char c) { return std::tolower(c); });
                if (qLower.empty() || sLower.find(qLower) != std::string::npos) {
                    std::string key = uri + "|" + symName + "|" + std::to_string(lineNum);
                    if (!seen.insert(key).second) return;
                    SymbolInformation sym;
                    sym.name = symName;
                    sym.kind = kind;
                    sym.location.uri = uri;
                    sym.location.range.start.line = lineNum;
                    sym.location.range.start.character = static_cast<int>(nameStart);
                    sym.location.range.end.line = lineNum;
                    sym.location.range.end.character = static_cast<int>(nameEnd);
                    out.push_back(sym.toJson());
                }
            }
        }
    };
    while (std::getline(stream, line)) {
        findSymbol("fn", 12);
        findSymbol("class", 5);
        findSymbol("struct", 23);
        findSymbol("interface", 11);
        findSymbol("enum", 10);
        findSymbol("let", 13);
        findSymbol("const", 14);
        lineNum++;
    }
}

JsonValue LspServer::handleReferences(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    Position pos = Position::fromJson(params["position"]);
    JsonValue refs = JsonValue::array();
    SymbolIndex index = buildIndex(uri);
    if (index.valid()) {
        if (const Symbol* sym = index.resolveAt(pos.line, pos.character)) {
            // The declaration itself is included (LSP default).
            for (const RefPos& r : index.referencesOf(sym)) {
                Location loc;
                loc.uri = uri;
                loc.range.start = Position{r.line, r.character};
                loc.range.end = Position{r.line, r.endCharacter};
                refs.push_back(loc.toJson());
            }
        }
    }
    return refs;
}

JsonValue LspServer::handleDocumentSymbol(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    JsonValue out = JsonValue::array();
    SymbolIndex index = buildIndex(uri);
    if (!index.valid()) return out;
    for (const auto& s : index.symbols()) {
        // Top-level only: class members carry a non-empty parent and
        // are nested as children below; locals are excluded entirely.
        if (s.scopeId != 0 || !s.parent.empty()) continue;
        DocumentSymbol ds;
        ds.name = s.name;
        ds.detail = s.signature;
        ds.kind = symKindToLsp(s.kind);
        ds.range.start = Position{s.line - 1, 0};
        ds.range.end = Position{s.endLine - 1, 0};
        ds.selectionRange.start = Position{s.line - 1, s.col - 1};
        ds.selectionRange.end = Position{s.line - 1, s.col - 1 + static_cast<int>(s.name.size())};
        if (s.kind == SymKind::Class || s.kind == SymKind::Interface ||
            s.kind == SymKind::Struct || s.kind == SymKind::Enum) {
            for (const auto& m : index.symbols()) {
                if (m.parent != s.name) continue;
                // Only real members nest here: method parameters live
                // in the method's scope but carry the class as parent.
                if (m.kind != SymKind::Method && m.kind != SymKind::Field &&
                    m.kind != SymKind::EnumVariant) {
                    continue;
                }
                DocumentSymbol child;
                child.name = m.name;
                child.detail = m.signature;
                child.kind = symKindToLsp(m.kind);
                child.range.start = Position{m.line - 1, m.col - 1};
                child.range.end = Position{m.line - 1, m.col - 1 + static_cast<int>(m.name.size())};
                child.selectionRange = child.range;
                ds.children.push_back(child);
            }
        }
        out.push_back(ds.toJson());
    }
    return out;
}

JsonValue LspServer::handleSemanticTokens(const JsonValue&, const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    std::string source = getDocument(uri);
    JsonValue result = JsonValue::object();
    JsonValue data = JsonValue::array();
    if (!source.empty()) {
        // Legend indices (must match handleInitialize):
        // 0 namespace, 1 type, 2 class, 3 enum, 4 interface, 5 struct,
        // 6 typeParameter, 7 parameter, 8 variable, 9 property,
        // 10 enumMember, 11 function, 12 method, 13 keyword, 14 comment,
        // 15 string, 16 number, 17 operator.
        // Modifier bits: 1 declaration, 16 documentation.
        auto isKeywordLexeme = [](const std::string& w) {
            for (const auto& [k, d] : SP_KEYWORDS) {
                if (k == w) return true;
            }
            return false;
        };
        auto isPrimitiveLexeme = [](const std::string& w) {
            for (const auto& t : SP_PRIMITIVE_TYPES) {
                if (t == w) return true;
            }
            return false;
        };
        std::ostringstream diagStream;
        diagnostics::DiagnosticEngine diagEngine(diagStream);
        lexer::Lexer lexer(source, uri, &diagEngine);
        lexer.setPreserveComments(true);
        auto tokens = lexer.tokenizeAll();
        // Phase 4 slice B: the lexer returns string/char lexemes DECODED
        // (quotes stripped, escapes processed), so lexeme.size() is not
        // the source span (`"Hello, "` is 9 source chars, 7 decoded).
        // Measure those tokens from the source text instead, so editors
        // highlight the whole literal including its quotes.
        std::vector<size_t> lineStarts{0};
        for (size_t i = 0; i < source.size(); ++i) {
            if (source[i] == '\n') lineStarts.push_back(i + 1);
        }
        auto sourceLenAt = [&](int line, int col) -> int {
            if (line < 0 || line >= static_cast<int>(lineStarts.size())) return 0;
            size_t pos = lineStarts[static_cast<size_t>(line)] + static_cast<size_t>(col);
            if (pos >= source.size()) return 0;
            char quote = source[pos];
            if (quote != '"' && quote != '\'') return 0;
            size_t i = pos + 1;
            while (i < source.size() && source[i] != '\n') {
                if (source[i] == '\\') { i += 2; continue; }
                if (source[i] == quote) return static_cast<int>(i - pos + 1);
                ++i;
            }
            return 0; // unterminated: caller falls back to lexeme length
        };
        SymbolIndex index = buildIndex(uri);
        struct Tok {
            int line, col, len, type, mods;
        };
        std::vector<Tok> toks;
        for (const auto& tok : tokens) {
            if (tok.type == lexer::TokenType::EndOfFile || tok.type == lexer::TokenType::Invalid) continue;
            int line = static_cast<int>(tok.location.line) - 1;
            int col = static_cast<int>(tok.location.column) - 1;
            int len = static_cast<int>(tok.lexeme.size());
            if (len <= 0) continue;
            int type = 17;
            int mods = 0;
            switch (tok.type) {
                case lexer::TokenType::Comment:
                    type = 14;
                    break;
                case lexer::TokenType::DocComment:
                    type = 14;
                    mods = 16; // documentation
                    break;
                case lexer::TokenType::StringLiteral:
                case lexer::TokenType::CharLiteral: {
                    type = 15;
                    int srcLen = sourceLenAt(line, col);
                    if (srcLen > 0) len = srcLen;
                    break;
                }
                case lexer::TokenType::IntegerLiteral:
                case lexer::TokenType::FloatLiteral:
                case lexer::TokenType::BooleanLiteral:
                    type = 16;
                    break;
                case lexer::TokenType::Identifier: {
                    type = 8; // variable, refined below via the index
                    if (index.valid()) {
                        if (const Symbol* s = index.resolveAt(line, col)) {
                            switch (s->kind) {
                                case SymKind::Function: type = 11; break;
                                case SymKind::Method: type = 12; break;
                                case SymKind::Class: type = 2; break;
                                case SymKind::Interface: type = 4; break;
                                case SymKind::Struct: type = 5; break;
                                case SymKind::Enum: type = 3; break;
                                case SymKind::EnumVariant: type = 10; break;
                                case SymKind::Parameter: type = 7; break;
                                case SymKind::Field: type = 9; break;
                                default: type = 8; break;
                            }
                            if (s->line - 1 == line && s->col - 1 == col) {
                                mods |= 1; // declaration
                            }
                        }
                    }
                    break;
                }
                default:
                    if (isPrimitiveLexeme(tok.lexeme)) {
                        type = 1;
                    } else if (isKeywordLexeme(tok.lexeme)) {
                        type = 13;
                    } else {
                        type = 17; // operators and punctuation
                    }
                    break;
            }
            toks.push_back(Tok{line, col, len, type, mods});
        }
        std::sort(toks.begin(), toks.end(), [](const Tok& a, const Tok& b) {
            return a.line != b.line ? a.line < b.line : a.col < b.col;
        });
        int prevLine = 0;
        int prevCol = 0;
        for (const auto& t : toks) {
            int dLine = t.line - prevLine;
            int dCol = (dLine == 0) ? t.col - prevCol : t.col;
            data.push_back(dLine);
            data.push_back(dCol);
            data.push_back(t.len);
            data.push_back(t.type);
            data.push_back(t.mods);
            prevLine = t.line;
            prevCol = t.col;
        }
    }
    result["data"] = data;
    return result;
}

} // namespace sp::lsp
