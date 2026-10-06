#include "LspServer.h"
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
    }

    return "";
}

JsonValue LspServer::handleInitialize(const JsonValue&, const JsonValue&) {
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

    result["capabilities"] = caps;

    JsonValue serverInfo = JsonValue::object();
    serverInfo["name"] = "spls";
    serverInfo["version"] = "1.0.0";
    result["serverInfo"] = serverInfo;

    return result;
}

JsonValue LspServer::handleShutdown(const JsonValue&) {
    m_running = false;
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

void LspServer::handleDidChange(const JsonValue& params) {
    std::string uri = params["textDocument"]["uri"].asString();
    const auto& changes = params["contentChanges"];
    if (changes.isArray() && !changes.arrVal.empty()) {
        std::string text = changes[0]["text"].asString();
        updateDocument(uri, text);
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

    // 3. Document symbols
    if (!source.empty()) {
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

} // namespace sp::lsp
