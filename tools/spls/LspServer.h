#pragma once

#include "Json.h"
#include "Protocol.h"
#include "SymbolIndex.h"
#include <set>
#include <string>
#include <map>
#include <vector>
#include <iostream>

namespace sp::lsp {

class LspServer {
public:
    explicit LspServer(std::istream& in = std::cin, std::ostream& out = std::cout);

    void run();

    // Process a single JSON-RPC string message and return response (or empty string if notification)
    std::string handleMessage(const std::string& rawMessage);

    // Document store
    void openDocument(const std::string& uri, const std::string& text);
    void updateDocument(const std::string& uri, const std::string& text);
    void closeDocument(const std::string& uri);
    std::string getDocument(const std::string& uri) const;

    // LSP Handlers
    JsonValue handleInitialize(const JsonValue& id, const JsonValue& params);
    JsonValue handleShutdown(const JsonValue& id);
    void handleDidOpen(const JsonValue& params);
    void handleDidChange(const JsonValue& params);
    void handleDidClose(const JsonValue& params);
    void handleDidChangeWatchedFiles(const JsonValue& params);
    JsonValue handleCompletion(const JsonValue& id, const JsonValue& params);
    JsonValue handleHover(const JsonValue& id, const JsonValue& params);
    JsonValue handleDefinition(const JsonValue& id, const JsonValue& params);
    JsonValue handleReferences(const JsonValue& id, const JsonValue& params);
    JsonValue handleDocumentSymbol(const JsonValue& id, const JsonValue& params);
    JsonValue handleSemanticTokens(const JsonValue& id, const JsonValue& params);
    JsonValue handleFormatting(const JsonValue& id, const JsonValue& params);
    JsonValue handleRename(const JsonValue& id, const JsonValue& params);
    JsonValue handleWorkspaceSymbol(const JsonValue& id, const JsonValue& params);
    JsonValue handleInlayHint(const JsonValue& id, const JsonValue& params);

    // Incremental document synchronization helper
    void applyIncrementalEdit(const std::string& uri, const Range& range, const std::string& text);

    // Diagnostics helper
    std::vector<Diagnostic> computeDiagnostics(const std::string& uri, const std::string& source);
    void publishDiagnostics(const std::string& uri);

    // Framing helper
    static std::string frameMessage(const std::string& json);
    static bool parseHeader(std::istream& in, size_t& contentLength);

    [[nodiscard]] bool isRunning() const { return m_running; }
    [[nodiscard]] bool isInitialized() const { return m_initialized; }
    void stop() { m_running = false; }

private:
    std::istream& m_in;
    std::ostream& m_out;
    bool m_running{true};
    [[maybe_unused]] bool m_initialized{false};
    // Phase 4 slice B: per the LSP spec the server must stay alive
    // after `shutdown` until it receives the `exit` notification. It
    // used to exit on shutdown itself, so a well-behaved client sending
    // `exit` afterwards got a broken pipe.
    bool m_shutdownRequested{false};
    std::map<std::string, std::string> m_documents; // uri -> text

    // Phase 4 slice B: workspace state. The VS Code client already
    // watches **/*.sp and sends workspace/didChangeWatchedFiles, but the
    // server used to ignore both it and initialize's rootUri, so
    // workspace-wide features only ever saw open documents.
    std::string m_rootUri;
    std::map<std::string, std::string> m_workspaceFiles; // uri -> text (on-disk, not open)

    void sendNotification(const std::string& method, const JsonValue& params);
    void sendResponse(const JsonValue& id, const JsonValue& result);
    void sendError(const JsonValue& id, int code, const std::string& message);

    static std::string extractWordAtPosition(const std::string& text, int line, int character);
    static std::string uriToPath(const std::string& uri);
    static std::string readFileText(const std::string& path);
    void indexWorkspace(const std::string& rootUri);
    void searchTextForSymbols(const std::string& uri, const std::string& text, const std::string& queryLower, JsonValue& out, std::set<std::string>& seen) const;

    // Phase 4 slice B: AST-based symbol index for one document, built
    // fresh per request (parse + sema). Falls back to the old
    // DocGenerator/token behaviour when the document does not parse.
    SymbolIndex buildIndex(const std::string& uri) const;
    static std::string symKindLabel(SymKind kind);
    static int symKindToLsp(SymKind kind);
    static std::string hoverTextFor(const Symbol& sym);
};

} // namespace sp::lsp
