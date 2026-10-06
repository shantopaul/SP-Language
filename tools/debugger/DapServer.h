#pragma once

#include "DapProtocol.h"
#include "DebugSession.h"
#include <iostream>
#include <string>

namespace sp::dap {

class DapServer {
public:
    explicit DapServer(std::istream& in = std::cin, std::ostream& out = std::cout);

    void run();

    // Process a single JSON-RPC string message and return response (or empty string if event/notification)
    std::string handleMessage(const std::string& rawMessage);

    // DAP Event dispatchers
    void sendEvent(const std::string& eventType, const JsonValue& body = JsonValue::object());
    void sendResponse(int requestSeq, const std::string& command, bool success = true, const JsonValue& body = JsonValue::object());
    void sendError(int requestSeq, const std::string& command, const std::string& message);

    DebugSession& getSession() { return m_session; }
    [[nodiscard]] bool isRunning() const { return m_running; }
    void stop() { m_running = false; }

    static std::string frameMessage(const std::string& json);
    static bool parseHeader(std::istream& in, size_t& contentLength);

private:
    std::istream& m_in;
    std::ostream& m_out;
    bool m_running{true};
    int m_seq{1};

    DebugSession m_session;

    JsonValue handleInitialize(const JsonValue& args);
    JsonValue handleLaunch(const JsonValue& args);
    JsonValue handleSetBreakpoints(const JsonValue& args);
    JsonValue handleConfigurationDone();
    JsonValue handleThreads();
    JsonValue handleStackTrace(const JsonValue& args);
    JsonValue handleScopes(const JsonValue& args);
    JsonValue handleVariables(const JsonValue& args);
    JsonValue handleContinue();
    JsonValue handleNext();
    JsonValue handleStepIn();
    JsonValue handleStepOut();
    JsonValue handleEvaluate(const JsonValue& args);
    JsonValue handleDisconnect();

    void checkSessionState();
};

} // namespace sp::dap
