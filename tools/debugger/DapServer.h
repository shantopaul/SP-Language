#pragma once

// Phase 4 slice C (pro-4.0.0): DapServer changes.
//   - launch does NOT run the program. Execution starts at
//     configurationDone (the standard DAP flow: launch -> setBreakpoints
//     -> configurationDone -> continue). Before this slice, launch ran
//     the program to completion immediately, so breakpoints set afterwards
//     could never fire.
//   - launch fails loudly (success=false) when the program cannot be
//     loaded (missing file, syntax/sema error). Before, a missing file
//     "succeeded" and terminated at once.
//   - launch honours `cwd` (relative program paths) and `noDebug` (run
//     to completion, breakpoints ignored).
//   - Program output (println/print) is sent as DAP `output` events
//     (stdout); a runtime error is sent as an `output` event (stderr)
//     and the session exits with code 1.
//   - evaluate fails loudly (success=false) when the expression cannot
//     be evaluated. Before, it echoed the expression back as its value.
//   - Unknown commands fail loudly instead of returning success=true
//     with an empty body.
//   - setVariable is now handled (it was advertised in initialize but
//     had no handler at all).

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
    bool m_launched{false};

    DebugSession m_session;

    // Set by a handler when its request must fail; handleMessage turns
    // it into success=false + message. Cleared on every request.
    std::string m_lastError;

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
    JsonValue handleSetVariable(const JsonValue& args);
    JsonValue handleDisconnect();

    void checkSessionState();
    void drainOutputEvents();
};

} // namespace sp::dap
