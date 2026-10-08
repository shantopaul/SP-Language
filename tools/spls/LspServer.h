#pragma once

#include "Json.h"
#include "Protocol.h"
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
    JsonValue handleCompletion(const JsonValue& id, const JsonValue& params);
    JsonValue handleHover(const JsonValue& id, const JsonValue& params);
    JsonValue handleDefinition(const JsonValue& id, const JsonValue& params);
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
    std::map<std::string, std::string> m_documents; // uri -> text

    void sendNotification(const std::string& method, const JsonValue& params);
    void sendResponse(const JsonValue& id, const JsonValue& result);
    void sendError(const JsonValue& id, int code, const std::string& message);

    static std::string extractWordAtPosition(const std::string& text, int line, int character);
};

} // namespace sp::lsp
