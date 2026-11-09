#include <gtest/gtest.h>
#include "debugger/DapServer.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace sp::dap;

TEST(DapServerTest, InitializeRequest) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    std::string initReq = "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\",\"arguments\":{}}";
    std::string respStr = server.handleMessage(initReq);
    JsonValue resp = JsonValue::parse(respStr);

    EXPECT_EQ(resp["type"].asString(), "response");
    EXPECT_EQ(resp["command"].asString(), "initialize");
    EXPECT_EQ(resp["request_seq"].asInt(), 1);
    EXPECT_TRUE(resp["success"].asBool());

    const auto& body = resp["body"];
    EXPECT_TRUE(body["supportsConfigurationDoneRequest"].asBool());
    EXPECT_TRUE(body["supportsEvaluateForHovers"].asBool());
    EXPECT_TRUE(body["supportsSetVariable"].asBool());
}

TEST(DapServerTest, LaunchAndStopOnEntry) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    std::string sampleProgram =
        "fn main() {\n"
        "    let x = 10;\n"
        "    let y = 20;\n"
        "    let z = x + y;\n"
        "}\n";

    server.getSession().loadProgram("sample.sp", sampleProgram);

    std::string launchReq = 
        "{\"seq\":2,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{\"program\":\"sample.sp\",\"stopOnEntry\":true}}";

    std::string respStr = server.handleMessage(launchReq);
    JsonValue resp = JsonValue::parse(respStr);
    EXPECT_TRUE(resp["success"].asBool());

    EXPECT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getPauseReason(), "entry");
    // Phase 4 slice C: the first executable statement is line 2
    // (`let x = 10;`); line 1 (`fn main() {`) is a definition, not an
    // execution, so stopOnEntry pauses at line 2, not line 1.
    EXPECT_EQ(server.getSession().getCurrentLine(), 2);
}

TEST(DapServerTest, SetBreakpointsAndContinue) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    std::string sampleProgram =
        "fn main() {\n"
        "    let a = 1;\n"
        "    let b = 2;\n"
        "    let c = 3;\n"
        "}\n";

    server.getSession().loadProgram("test.sp", sampleProgram);

    std::string bpReq =
        "{\"seq\":3,\"type\":\"request\",\"command\":\"setBreakpoints\","
        "\"arguments\":{\"source\":{\"path\":\"test.sp\"},\"breakpoints\":[{\"line\":3}]}}";

    std::string respStr = server.handleMessage(bpReq);
    JsonValue resp = JsonValue::parse(respStr);
    EXPECT_TRUE(resp["success"].asBool());

    const auto& bps = resp["body"]["breakpoints"];
    ASSERT_TRUE(bps.isArray());
    ASSERT_EQ(bps.arrVal.size(), 1);
    EXPECT_TRUE(bps[0]["verified"].asBool());
    EXPECT_EQ(bps[0]["line"].asInt(), 3);

    // Launch with stopOnEntry: false -> nothing runs yet (execution
    // starts at configurationDone, the standard DAP flow).
    std::string launchReq = 
        "{\"seq\":4,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{\"program\":\"test.sp\",\"stopOnEntry\":false}}";
    server.handleMessage(launchReq);
    EXPECT_FALSE(server.getSession().isPaused());

    // configurationDone starts execution; it stops at the line 3 breakpoint.
    std::string configReq =
        "{\"seq\":5,\"type\":\"request\",\"command\":\"configurationDone\",\"arguments\":{}}";
    server.handleMessage(configReq);

    EXPECT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getPauseReason(), "breakpoint");
    EXPECT_EQ(server.getSession().getCurrentLine(), 3);
}

TEST(DapServerTest, SteppingAndStackTrace) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    std::string sampleProgram =
        "fn main() {\n"
        "    let x = 100;\n"
        "    let y = 200;\n"
        "}\n";

    server.getSession().loadProgram("step.sp", sampleProgram);
    server.getSession().start(true); // stop on entry (line 2: `let x = 100;`)
    EXPECT_EQ(server.getSession().getCurrentLine(), 2);

    // Step over to line 3 (`let y = 200;`)
    std::string nextReq = "{\"seq\":5,\"type\":\"request\",\"command\":\"next\",\"arguments\":{}}";
    server.handleMessage(nextReq);
    EXPECT_EQ(server.getSession().getCurrentLine(), 3);

    // Request stackTrace
    std::string stReq = "{\"seq\":6,\"type\":\"request\",\"command\":\"stackTrace\",\"arguments\":{}}";
    std::string stRespStr = server.handleMessage(stReq);
    JsonValue stResp = JsonValue::parse(stRespStr);
    EXPECT_TRUE(stResp["success"].asBool());

    const auto& frames = stResp["body"]["stackFrames"];
    ASSERT_TRUE(frames.isArray());
    ASSERT_GE(frames.arrVal.size(), 1);
    EXPECT_EQ(frames[0]["line"].asInt(), 3);
    EXPECT_EQ(frames[0]["name"].asString(), "main");
}

TEST(DapServerTest, ScopesVariablesAndEvaluation) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    std::string sampleProgram =
        "fn main() {\n"
        "    let score = 99;\n"
        "    let name = \"Shanto\";\n"
        "}\n";

    server.getSession().loadProgram("vars.sp", sampleProgram);
    server.getSession().start(true); // line 2 (`let score = 99;`)

    // Step once: executes `let score = 99`, pauses at line 3
    // (`let name = "Shanto";`) — score is defined, name is not yet.
    server.handleMessage("{\"seq\":7,\"type\":\"request\",\"command\":\"next\",\"arguments\":{}}");
    EXPECT_EQ(server.getSession().getCurrentLine(), 3);

    // Scopes request
    std::string scopesReq = "{\"seq\":9,\"type\":\"request\",\"command\":\"scopes\",\"arguments\":{\"frameId\":1}}";
    std::string scRespStr = server.handleMessage(scopesReq);
    JsonValue scResp = JsonValue::parse(scRespStr);
    EXPECT_TRUE(scResp["success"].asBool());
    const auto& scopes = scResp["body"]["scopes"];
    ASSERT_TRUE(scopes.isArray());
    ASSERT_GE(scopes.arrVal.size(), 1);
    int varRef = static_cast<int>(scopes[0]["variablesReference"].asInt());

    // Variables request
    std::string varsReq = 
        "{\"seq\":10,\"type\":\"request\",\"command\":\"variables\",\"arguments\":{\"variablesReference\":" + std::to_string(varRef) + "}}";
    std::string varsRespStr = server.handleMessage(varsReq);
    JsonValue varsResp = JsonValue::parse(varsRespStr);
    EXPECT_TRUE(varsResp["success"].asBool());

    const auto& vars = varsResp["body"]["variables"];
    ASSERT_TRUE(vars.isArray());
    bool foundScore = false;
    bool foundName = false;
    for (const auto& v : vars.arrVal) {
        if (v["name"].asString() == "score") {
            foundScore = true;
            EXPECT_EQ(v["value"].asString(), "99");
            EXPECT_EQ(v["type"].asString(), "int");
        }
        if (v["name"].asString() == "name") foundName = true;
    }
    EXPECT_TRUE(foundScore);
    // `name` is declared on line 3, which has not executed yet.
    EXPECT_FALSE(foundName);

    // Evaluate request
    std::string evalReq =
        "{\"seq\":11,\"type\":\"request\",\"command\":\"evaluate\",\"arguments\":{\"frameId\":1,\"expression\":\"score\"}}";
    std::string evalRespStr = server.handleMessage(evalReq);
    JsonValue evalResp = JsonValue::parse(evalRespStr);
    EXPECT_TRUE(evalResp["success"].asBool());
    EXPECT_EQ(evalResp["body"]["result"].asString(), "99");

    // Phase 4 slice C: evaluate a real expression, not an echo.
    std::string evalSumReq =
        "{\"seq\":12,\"type\":\"request\",\"command\":\"evaluate\",\"arguments\":{\"frameId\":1,\"expression\":\"score + 1\"}}";
    std::string evalSumRespStr = server.handleMessage(evalSumReq);
    JsonValue evalSumResp = JsonValue::parse(evalSumRespStr);
    EXPECT_TRUE(evalSumResp["success"].asBool());
    EXPECT_EQ(evalSumResp["body"]["result"].asString(), "100");
}

TEST(DapServerTest, DisconnectRequest) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    std::string discReq = "{\"seq\":12,\"type\":\"request\",\"command\":\"disconnect\",\"arguments\":{}}";
    std::string respStr = server.handleMessage(discReq);
    JsonValue resp = JsonValue::parse(respStr);
    EXPECT_TRUE(resp["success"].asBool());
    EXPECT_FALSE(server.isRunning());
}

// ---------------------------------------------------------------------------
// Phase 4 slice C: new coverage for the real interpreter.
// ---------------------------------------------------------------------------

namespace {

const char* kProbeProgram =
    "fn add(int a, int b) -> int {\n"
    "    return a + b;\n"
    "}\n"
    "\n"
    "fn main() {\n"
    "    let x = 10;\n"
    "    let y = 20;\n"
    "    let total = add(x, y);\n"
    "    let name = \"Shanto\";\n"
    "    let flag = true;\n"
    "    let pi = 3.5;\n"
    "    let sum = 0;\n"
    "    for (int i = 0; i < 3; i++) {\n"
    "        sum = sum + i;\n"
    "    }\n"
    "    println(total);\n"
    "    println(sum);\n"
    "}\n";

JsonValue sendReq(DapServer& server, int seq, const std::string& command,
                  const std::string& argsJson = "{}") {
    std::string req = "{\"seq\":" + std::to_string(seq) +
                      ",\"type\":\"request\",\"command\":\"" + command +
                      "\",\"arguments\":" + argsJson + "}";
    return JsonValue::parse(server.handleMessage(req));
}

int localsRef(DapServer& server, int seq, int frameId) {
    JsonValue resp = sendReq(server, seq, "scopes",
                             "{\"frameId\":" + std::to_string(frameId) + "}");
    const auto& scopes = resp["body"]["scopes"];
    if (!scopes.isArray()) return 0;
    for (const auto& s : scopes.arrVal) {
        if (s["name"].asString() == "Locals") {
            return static_cast<int>(s["variablesReference"].asInt());
        }
    }
    return 0;
}

std::map<std::string, std::string> variablesMap(DapServer& server, int seq, int ref) {
    JsonValue resp = sendReq(server, seq, "variables",
                             "{\"variablesReference\":" + std::to_string(ref) + "}");
    std::map<std::string, std::string> out;
    const auto& vars = resp["body"]["variables"];
    if (vars.isArray()) {
        for (const auto& v : vars.arrVal) {
            out[v["name"].asString()] = v["value"].asString();
        }
    }
    return out;
}

void launchProbe(DapServer& server, int seq) {
    server.getSession().loadProgram("probe.sp", kProbeProgram);
    JsonValue launch = sendReq(server, seq, "launch",
                               "{\"program\":\"probe.sp\",\"stopOnEntry\":false}");
    ASSERT_TRUE(launch["success"].asBool());
    EXPECT_FALSE(server.getSession().isPaused()); // launch alone runs nothing
}

// Phase 4 slice C: program output leaves the server as DAP `output`
// events on the wire — the server drains the session's pending output
// into events after every run slice — so tests read the `out` stream,
// not session.drainOutput(), which is already empty by then.
std::vector<std::string> stdoutOutputEvents(const std::string& wire) {
    std::vector<std::string> lines;
    std::size_t pos = 0;
    while (true) {
        std::size_t hdr = wire.find("Content-Length:", pos);
        if (hdr == std::string::npos) break;
        std::size_t sep = wire.find("\r\n\r\n", hdr);
        if (sep == std::string::npos) break;
        int len = std::stoi(wire.substr(hdr + 15, sep - (hdr + 15)));
        std::string json = wire.substr(sep + 4, static_cast<std::size_t>(len));
        pos = sep + 4 + static_cast<std::size_t>(len);
        JsonValue msg = JsonValue::parse(json);
        if (msg["type"].asString() != "event") continue;
        if (msg["event"].asString() != "output") continue;
        if (msg["body"]["category"].asString() != "stdout") continue;
        std::string text = msg["body"]["output"].asString();
        if (!text.empty() && text.back() == '\n') text.pop_back();
        lines.push_back(text);
    }
    return lines;
}

} // namespace

TEST(DapServerTest, LaunchMissingProgramFails) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    JsonValue resp = sendReq(server, 1, "launch",
                             "{\"program\":\"/tmp/does-not-exist-12345.sp\",\"stopOnEntry\":false}");
    EXPECT_FALSE(resp["success"].asBool());
    EXPECT_FALSE(resp["message"].asString().empty());
}

TEST(DapServerTest, LoadInvalidProgramFails) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    EXPECT_FALSE(server.getSession().loadProgram(
        "bad.sp", "fn main() {\n    let x = ;\n}\n"));
    EXPECT_FALSE(server.getSession().getLoadError().empty());
}

TEST(DapServerTest, BreakpointVerification) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);
    server.getSession().loadProgram("probe.sp", kProbeProgram);

    // Blank line 4 moves to the next executable line (6); the closing
    // brace (18) and past-EOF (99) can never execute: verified=false.
    JsonValue resp = sendReq(server, 1, "setBreakpoints",
                             "{\"source\":{\"path\":\"probe.sp\"},"
                             "\"breakpoints\":[{\"line\":4},{\"line\":18},{\"line\":99}]}");
    ASSERT_TRUE(resp["success"].asBool());
    const auto& bps = resp["body"]["breakpoints"];
    ASSERT_TRUE(bps.isArray());
    ASSERT_EQ(bps.arrVal.size(), 3);
    EXPECT_TRUE(bps[0]["verified"].asBool());
    EXPECT_EQ(bps[0]["line"].asInt(), 6);
    EXPECT_FALSE(bps[1]["verified"].asBool());
    EXPECT_EQ(bps[1]["line"].asInt(), 18);
    EXPECT_FALSE(bps[2]["verified"].asBool());
    EXPECT_EQ(bps[2]["line"].asInt(), 99);

    JsonValue ok = sendReq(server, 2, "setBreakpoints",
                           "{\"source\":{\"path\":\"probe.sp\"},"
                           "\"breakpoints\":[{\"line\":8}]}");
    ASSERT_TRUE(ok["success"].asBool());
    EXPECT_TRUE(ok["body"]["breakpoints"].arrVal[0]["verified"].asBool());
    EXPECT_EQ(ok["body"]["breakpoints"].arrVal[0]["line"].asInt(), 8);
}

TEST(DapServerTest, StandardFlowStopsAtBreakpoint) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);
    launchProbe(server, 1);

    JsonValue bp = sendReq(server, 2, "setBreakpoints",
                           "{\"source\":{\"path\":\"probe.sp\"},"
                           "\"breakpoints\":[{\"line\":8}]}");
    ASSERT_TRUE(bp["success"].asBool());

    JsonValue config = sendReq(server, 3, "configurationDone");
    ASSERT_TRUE(config["success"].asBool());
    ASSERT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getPauseReason(), "breakpoint");
    EXPECT_EQ(server.getSession().getCurrentLine(), 8);

    // Line 8 (`let total = add(x, y);`) has not executed yet: x and y are
    // real values, total does not exist yet.
    auto vars = variablesMap(server, 4, localsRef(server, 5, 1));
    EXPECT_EQ(vars["x"], "10");
    EXPECT_EQ(vars["y"], "20");
    EXPECT_EQ(vars.count("total"), 0u);

    // stepIn enters add() at its first statement (line 2), with real
    // parameters bound.
    JsonValue stepIn = sendReq(server, 6, "stepIn");
    ASSERT_TRUE(stepIn["success"].asBool());
    ASSERT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getCurrentLine(), 2);
    JsonValue st = sendReq(server, 7, "stackTrace", "{\"threadId\":1}");
    const auto& frames = st["body"]["stackFrames"];
    ASSERT_TRUE(frames.isArray());
    ASSERT_EQ(frames.arrVal.size(), 2);
    EXPECT_EQ(frames[0]["name"].asString(), "add");
    EXPECT_EQ(frames[0]["line"].asInt(), 2);
    EXPECT_EQ(frames[1]["name"].asString(), "main");
    EXPECT_EQ(frames[1]["line"].asInt(), 8);
    int addFrameId = static_cast<int>(frames[0]["id"].asInt());
    auto addVars = variablesMap(server, 8, localsRef(server, 9, addFrameId));
    EXPECT_EQ(addVars["a"], "10");
    EXPECT_EQ(addVars["b"], "20");

    // stepOut returns to the caller line after the call, with total = 30.
    JsonValue stepOut = sendReq(server, 10, "stepOut");
    ASSERT_TRUE(stepOut["success"].asBool());
    ASSERT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getCurrentLine(), 9);
    auto afterVars = variablesMap(server, 11, localsRef(server, 12, 1));
    EXPECT_EQ(afterVars["total"], "30");

    // Continue to completion; println output is captured, not lost.
    JsonValue cont = sendReq(server, 13, "continue");
    ASSERT_TRUE(cont["success"].asBool());
    EXPECT_TRUE(server.getSession().isTerminated());
    std::vector<std::string> output = stdoutOutputEvents(out.str());
    ASSERT_EQ(output.size(), 2u);
    EXPECT_EQ(output[0], "30");
    EXPECT_EQ(output[1], "3");
}

TEST(DapServerTest, LoopIterationValues) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);
    launchProbe(server, 1);

    JsonValue bp = sendReq(server, 2, "setBreakpoints",
                           "{\"source\":{\"path\":\"probe.sp\"},"
                           "\"breakpoints\":[{\"line\":14}]}");
    ASSERT_TRUE(bp["success"].asBool());

    sendReq(server, 3, "configurationDone");
    // Three stops at the loop body, one per iteration, with real values.
    const std::pair<const char*, const char*> expected[3] = {
        {"0", "0"}, {"1", "0"}, {"2", "1"}};
    for (int iter = 0; iter < 3; ++iter) {
        ASSERT_TRUE(server.getSession().isPaused()) << "iteration " << iter;
        EXPECT_EQ(server.getSession().getCurrentLine(), 14);
        auto vars = variablesMap(server, 10 + iter, localsRef(server, 20 + iter, 1));
        EXPECT_EQ(vars["i"], expected[iter].first) << "iteration " << iter;
        EXPECT_EQ(vars["sum"], expected[iter].second) << "iteration " << iter;
        sendReq(server, 30 + iter, "continue");
    }
    EXPECT_TRUE(server.getSession().isTerminated());
    std::vector<std::string> output = stdoutOutputEvents(out.str());
    ASSERT_EQ(output.size(), 2u);
    EXPECT_EQ(output[0], "30");
    EXPECT_EQ(output[1], "3"); // sum after the loop really ran 3 times
}

TEST(DapServerTest, EvaluateRealExpressions) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);
    launchProbe(server, 1);
    sendReq(server, 2, "setBreakpoints",
            "{\"source\":{\"path\":\"probe.sp\"},\"breakpoints\":[{\"line\":9}]}");
    sendReq(server, 3, "configurationDone");
    ASSERT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getCurrentLine(), 9);

    JsonValue sum = sendReq(server, 4, "evaluate",
                            "{\"frameId\":1,\"expression\":\"x + y\"}");
    ASSERT_TRUE(sum["success"].asBool());
    EXPECT_EQ(sum["body"]["result"].asString(), "30");

    JsonValue total = sendReq(server, 5, "evaluate",
                              "{\"frameId\":1,\"expression\":\"total\"}");
    ASSERT_TRUE(total["success"].asBool());
    EXPECT_EQ(total["body"]["result"].asString(), "30");

    // An unknown name fails loudly instead of echoing itself back.
    JsonValue bad = sendReq(server, 6, "evaluate",
                            "{\"frameId\":1,\"expression\":\"nosuchvar\"}");
    EXPECT_FALSE(bad["success"].asBool());
}

TEST(DapServerTest, RuntimeErrorIsLoud) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    server.getSession().loadProgram(
        "divzero.sp",
        "fn main() {\n"
        "    let d = 0;\n"
        "    let q = 10 / d;\n"
        "    println(q);\n"
        "}\n");
    sendReq(server, 1, "launch",
            "{\"program\":\"divzero.sp\",\"stopOnEntry\":false}");
    sendReq(server, 2, "configurationDone");
    EXPECT_TRUE(server.getSession().isTerminated());
    std::string error = server.getSession().getLastError();
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("Division by zero"), std::string::npos);
}

TEST(DapServerTest, SetVariableWorks) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);
    launchProbe(server, 1);
    sendReq(server, 2, "setBreakpoints",
            "{\"source\":{\"path\":\"probe.sp\"},\"breakpoints\":[{\"line\":8}]}");
    sendReq(server, 3, "configurationDone");
    ASSERT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getCurrentLine(), 8);

    int ref = localsRef(server, 4, 1);
    JsonValue set = sendReq(server, 5, "setVariable",
                            "{\"variablesReference\":" + std::to_string(ref) +
                                ",\"name\":\"x\",\"value\":\"99\"}");
    ASSERT_TRUE(set["success"].asBool());
    auto vars = variablesMap(server, 6, ref);
    EXPECT_EQ(vars["x"], "99");

    // The override survives the resume (replay re-applies it): total is
    // computed from the new x.
    sendReq(server, 7, "continue");
    EXPECT_TRUE(server.getSession().isTerminated());
    std::vector<std::string> output = stdoutOutputEvents(out.str());
    ASSERT_EQ(output.size(), 2u);
    EXPECT_EQ(output[0], "119"); // add(99, 20)
    EXPECT_EQ(output[1], "3");
}

TEST(DapServerTest, UnknownCommandFails) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    JsonValue resp = sendReq(server, 1, "frobnicate");
    EXPECT_FALSE(resp["success"].asBool());
}

TEST(DapServerTest, TryCatchCatchesThrow) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    // The first catch wins regardless of its type name (the payload is
    // stringified; same rule as the compiler's lowering, Phase 3 slice B).
    server.getSession().loadProgram(
        "trycatch.sp",
        "fn main() {\n"
        "    let r = 0;\n"
        "    try {\n"
        "        throw \"boom\";\n"
        "    } catch (Exception e) {\n"
        "        r = 7;\n"
        "    }\n"
        "    println(r);\n"
        "}\n");
    sendReq(server, 1, "launch",
            "{\"program\":\"trycatch.sp\",\"stopOnEntry\":false}");
    sendReq(server, 2, "configurationDone");
    EXPECT_TRUE(server.getSession().isTerminated());
    EXPECT_TRUE(server.getSession().getLastError().empty());
    std::vector<std::string> output = stdoutOutputEvents(out.str());
    ASSERT_EQ(output.size(), 1u);
    EXPECT_EQ(output[0], "7");
}

TEST(DapServerTest, SecondLaunchInSameSession) {
    std::stringstream in;
    std::stringstream out;
    DapServer server(in, out);

    // First program terminates (division by zero).
    server.getSession().loadProgram(
        "divzero.sp",
        "fn main() {\n"
        "    let d = 0;\n"
        "    let q = 10 / d;\n"
        "    println(q);\n"
        "}\n");
    sendReq(server, 1, "launch",
            "{\"program\":\"divzero.sp\",\"stopOnEntry\":false}");
    sendReq(server, 2, "configurationDone");
    EXPECT_TRUE(server.getSession().isTerminated());

    // A second launch in the same session must start fresh, not return
    // the first program's terminal state at once.
    launchProbe(server, 3);
    sendReq(server, 4, "setBreakpoints",
            "{\"source\":{\"path\":\"probe.sp\"},\"breakpoints\":[{\"line\":8}]}");
    sendReq(server, 5, "configurationDone");
    ASSERT_TRUE(server.getSession().isPaused());
    EXPECT_EQ(server.getSession().getCurrentLine(), 8);
    sendReq(server, 6, "continue");
    EXPECT_TRUE(server.getSession().isTerminated());
    std::vector<std::string> output = stdoutOutputEvents(out.str());
    ASSERT_EQ(output.size(), 2u);
    EXPECT_EQ(output[0], "30");
    EXPECT_EQ(output[1], "3");
}
