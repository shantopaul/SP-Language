#include "DapServer.h"
#include <sstream>

namespace sp::dap {

DapServer::DapServer(std::istream& in, std::ostream& out)
    : m_in(in), m_out(out) {}

std::string DapServer::frameMessage(const std::string& json) {
    return "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n" + json;
}

bool DapServer::parseHeader(std::istream& in, size_t& contentLength) {
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

void DapServer::sendEvent(const std::string& eventType, const JsonValue& body) {
    JsonValue ev = JsonValue::object();
    ev["seq"] = m_seq++;
    ev["type"] = "event";
    ev["event"] = eventType;
    ev["body"] = body;
    std::string s = ev.serialize();
    m_out << frameMessage(s) << std::flush;
}

void DapServer::sendResponse(int requestSeq, const std::string& command, bool success, const JsonValue& body) {
    JsonValue resp = JsonValue::object();
    resp["seq"] = m_seq++;
    resp["type"] = "response";
    resp["request_seq"] = requestSeq;
    resp["command"] = command;
    resp["success"] = success;
    resp["body"] = body;
    std::string s = resp.serialize();
    m_out << frameMessage(s) << std::flush;
}

void DapServer::sendError(int requestSeq, const std::string& command, const std::string& message) {
    JsonValue resp = JsonValue::object();
    resp["seq"] = m_seq++;
    resp["type"] = "response";
    resp["request_seq"] = requestSeq;
    resp["command"] = command;
    resp["success"] = false;
    resp["message"] = message;
    std::string s = resp.serialize();
    m_out << frameMessage(s) << std::flush;
}

std::string DapServer::handleMessage(const std::string& rawMessage) {
    JsonValue req = JsonValue::parse(rawMessage);
    if (!req.isObject()) return "";

    std::string type = req["type"].asString("");
    if (type != "request") return "";

    int seq = static_cast<int>(req["seq"].asInt(0));
    std::string command = req["command"].asString("");
    JsonValue args = req["arguments"];

    JsonValue body = JsonValue::object();
    bool success = true;
    m_lastError.clear();

    if (command == "initialize") {
        body = handleInitialize(args);
    } else if (command == "launch") {
        body = handleLaunch(args);
    } else if (command == "setBreakpoints") {
        body = handleSetBreakpoints(args);
    } else if (command == "configurationDone") {
        body = handleConfigurationDone();
    } else if (command == "threads") {
        body = handleThreads();
    } else if (command == "stackTrace") {
        body = handleStackTrace(args);
    } else if (command == "scopes") {
        body = handleScopes(args);
    } else if (command == "variables") {
        body = handleVariables(args);
    } else if (command == "continue") {
        body = handleContinue();
    } else if (command == "next") {
        body = handleNext();
    } else if (command == "stepIn") {
        body = handleStepIn();
    } else if (command == "stepOut") {
        body = handleStepOut();
    } else if (command == "evaluate") {
        body = handleEvaluate(args);
    } else if (command == "setVariable") {
        body = handleSetVariable(args);
    } else if (command == "disconnect") {
        body = handleDisconnect();
    } else {
        // Unknown command: fail loudly instead of a silent success=true.
        m_lastError = "Unknown command '" + command + "'";
    }

    if (!m_lastError.empty()) {
        success = false;
        body = JsonValue::object();
    }

    JsonValue resp = JsonValue::object();
    resp["seq"] = m_seq++;
    resp["type"] = "response";
    resp["request_seq"] = seq;
    resp["command"] = command;
    resp["success"] = success;
    if (!success) resp["message"] = m_lastError;
    resp["body"] = body;

    return resp.serialize();
}

void DapServer::run() {
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

JsonValue DapServer::handleInitialize(const JsonValue&) {
    JsonValue caps = JsonValue::object();
    caps["supportsConfigurationDoneRequest"] = true;
    caps["supportsFunctionBreakpoints"] = false;
    caps["supportsConditionalBreakpoints"] = false;
    caps["supportsEvaluateForHovers"] = true;
    caps["supportsStepBack"] = false;
    caps["supportsSetVariable"] = true;

    // Send initialized event
    sendEvent("initialized", JsonValue::object());

    return caps;
}

JsonValue DapServer::handleLaunch(const JsonValue& args) {
    std::string program = args["program"].asString("");
    if (program.empty()) {
        m_lastError = "launch: 'program' is required";
        return JsonValue::object();
    }
    bool stopOnEntry = args["stopOnEntry"].asBool(false);
    bool noDebug = args["noDebug"].asBool(false);
    std::string cwd = args["cwd"].asString("");

    // Resolve a relative program path against `cwd` (or the process cwd).
    std::string resolved = program;
    if (!cwd.empty() && !program.empty() && program[0] != '/' && program[0] != '\\') {
        resolved = cwd;
        if (!resolved.empty() && resolved.back() != '/' && resolved.back() != '\\') resolved += "/";
        resolved += program;
    }

    // Launch prepares but does NOT run the program: execution starts at
    // configurationDone (or at the first statement when stopOnEntry).
    // A program that cannot load fails loudly here.
    if (!m_session.launch(resolved, stopOnEntry, noDebug)) {
        m_lastError = "launch failed: " + m_session.getLoadError();
        return JsonValue::object();
    }
    m_launched = true;

    if (m_session.isPaused()) {
        JsonValue stopBody = JsonValue::object();
        stopBody["reason"] = m_session.getPauseReason();
        stopBody["threadId"] = 1;
        stopBody["allThreadsStopped"] = true;
        sendEvent("stopped", stopBody);
    }

    return JsonValue::object();
}

JsonValue DapServer::handleSetBreakpoints(const JsonValue& args) {
    std::string sourcePath = args["source"]["path"].asString("");
    std::vector<int> lines;
    const auto& bps = args["breakpoints"];
    if (bps.isArray()) {
        for (const auto& bp : bps.arrVal) {
            lines.push_back(static_cast<int>(bp["line"].asInt(0)));
        }
    }

    auto verifiedBps = m_session.setBreakpoints(sourcePath, lines);

    JsonValue body = JsonValue::object();
    JsonValue bpArr = JsonValue::array();
    for (const auto& bp : verifiedBps) {
        bpArr.push_back(bp.toJson());
    }
    body["breakpoints"] = bpArr;
    return body;
}

JsonValue DapServer::handleConfigurationDone() {
    if (!m_launched) return JsonValue::object(); // tolerant: nothing to run yet
    if (!m_session.startExecution()) {
        m_lastError = m_session.getLastError();
        return JsonValue::object();
    }
    drainOutputEvents();
    checkSessionState();
    return JsonValue::object();
}

JsonValue DapServer::handleThreads() {
    JsonValue body = JsonValue::object();
    JsonValue thArr = JsonValue::array();
    for (const auto& t : m_session.getThreads()) {
        thArr.push_back(t.toJson());
    }
    body["threads"] = thArr;
    return body;
}

JsonValue DapServer::handleStackTrace(const JsonValue&) {
    JsonValue body = JsonValue::object();
    JsonValue frames = JsonValue::array();
    auto stFrames = m_session.getStackFrames();
    for (const auto& f : stFrames) {
        frames.push_back(f.toJson());
    }
    body["stackFrames"] = frames;
    body["totalFrames"] = static_cast<int>(stFrames.size());
    return body;
}

JsonValue DapServer::handleScopes(const JsonValue& args) {
    int frameId = static_cast<int>(args["frameId"].asInt(1));
    JsonValue body = JsonValue::object();
    JsonValue scArr = JsonValue::array();
    for (const auto& sc : m_session.getScopes(frameId)) {
        scArr.push_back(sc.toJson());
    }
    body["scopes"] = scArr;
    return body;
}

JsonValue DapServer::handleVariables(const JsonValue& args) {
    int ref = static_cast<int>(args["variablesReference"].asInt(0));
    JsonValue body = JsonValue::object();
    JsonValue varArr = JsonValue::array();
    for (const auto& v : m_session.getVariables(ref)) {
        varArr.push_back(v.toJson());
    }
    body["variables"] = varArr;
    return body;
}

void DapServer::drainOutputEvents() {
    for (const auto& line : m_session.drainOutput()) {
        JsonValue body = JsonValue::object();
        body["category"] = "stdout";
        body["output"] = line + "\n";
        sendEvent("output", body);
    }
}

void DapServer::checkSessionState() {
    if (m_session.isPaused()) {
        JsonValue stopBody = JsonValue::object();
        stopBody["reason"] = m_session.getPauseReason();
        stopBody["threadId"] = 1;
        stopBody["allThreadsStopped"] = true;
        sendEvent("stopped", stopBody);
    } else if (m_session.isTerminated()) {
        std::string error = m_session.getLastError();
        int exitCode = error.empty() ? 0 : 1;
        if (!error.empty()) {
            JsonValue body = JsonValue::object();
            body["category"] = "stderr";
            body["output"] = "Runtime error: " + error + "\n";
            sendEvent("output", body);
        }
        sendEvent("terminated", JsonValue::object());
        JsonValue exitBody = JsonValue::object();
        exitBody["exitCode"] = exitCode;
        sendEvent("exited", exitBody);
    }
}

JsonValue DapServer::handleContinue() {
    if (!m_session.continueExecution()) {
        m_lastError = m_session.getLastError();
        return JsonValue::object();
    }
    drainOutputEvents();
    checkSessionState();
    return JsonValue::object();
}

JsonValue DapServer::handleNext() {
    if (!m_session.stepOver()) {
        m_lastError = m_session.getLastError();
        return JsonValue::object();
    }
    drainOutputEvents();
    checkSessionState();
    return JsonValue::object();
}

JsonValue DapServer::handleStepIn() {
    if (!m_session.stepIn()) {
        m_lastError = m_session.getLastError();
        return JsonValue::object();
    }
    drainOutputEvents();
    checkSessionState();
    return JsonValue::object();
}

JsonValue DapServer::handleStepOut() {
    if (!m_session.stepOut()) {
        m_lastError = m_session.getLastError();
        return JsonValue::object();
    }
    drainOutputEvents();
    checkSessionState();
    return JsonValue::object();
}

JsonValue DapServer::handleEvaluate(const JsonValue& args) {
    int frameId = static_cast<int>(args["frameId"].asInt(1));
    std::string expr = args["expression"].asString("");
    Variable v;
    std::string error;
    if (!m_session.evaluate(frameId, expr, v, error)) {
        m_lastError = "evaluate failed: " + error;
        return JsonValue::object();
    }

    JsonValue body = JsonValue::object();
    body["result"] = v.value;
    body["type"] = v.type;
    body["variablesReference"] = v.variablesReference;
    return body;
}

JsonValue DapServer::handleSetVariable(const JsonValue& args) {
    int ref = static_cast<int>(args["variablesReference"].asInt(0));
    std::string name = args["name"].asString("");
    std::string value = args["value"].asString("");
    if (!m_session.setVariable(ref, name, value)) {
        m_lastError = "setVariable failed: no such variable '" + name +
                      "' in this scope, or the value is not a literal";
        return JsonValue::object();
    }
    JsonValue body = JsonValue::object();
    body["value"] = value;
    body["type"] = "";
    body["variablesReference"] = 0;
    return body;
}

JsonValue DapServer::handleDisconnect() {
    m_running = false;
    return JsonValue::object();
}

} // namespace sp::dap
