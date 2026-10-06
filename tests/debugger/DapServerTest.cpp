#include <gtest/gtest.h>
#include "debugger/DapServer.h"
#include <sstream>

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
    EXPECT_EQ(server.getSession().getCurrentLine(), 1);
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

    // Launch with stopOnEntry: false -> should stop at line 3 breakpoint
    std::string launchReq = 
        "{\"seq\":4,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{\"program\":\"test.sp\",\"stopOnEntry\":false}}";
    server.handleMessage(launchReq);

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
    server.getSession().start(true); // stop on entry (line 1)

    // Step over to line 2
    std::string nextReq = "{\"seq\":5,\"type\":\"request\",\"command\":\"next\",\"arguments\":{}}";
    server.handleMessage(nextReq);
    EXPECT_EQ(server.getSession().getCurrentLine(), 2);

    // Request stackTrace
    std::string stReq = "{\"seq\":6,\"type\":\"request\",\"command\":\"stackTrace\",\"arguments\":{}}";
    std::string stRespStr = server.handleMessage(stReq);
    JsonValue stResp = JsonValue::parse(stRespStr);
    EXPECT_TRUE(stResp["success"].asBool());

    const auto& frames = stResp["body"]["stackFrames"];
    ASSERT_TRUE(frames.isArray());
    ASSERT_GE(frames.arrVal.size(), 1);
    EXPECT_EQ(frames[0]["line"].asInt(), 2);
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
    server.getSession().start(true); // line 1

    // Step once to execute let score = 99
    server.handleMessage("{\"seq\":7,\"type\":\"request\",\"command\":\"next\",\"arguments\":{}}");
    // Step once more to execute let name = "Shanto"
    server.handleMessage("{\"seq\":8,\"type\":\"request\",\"command\":\"next\",\"arguments\":{}}");

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
    for (const auto& v : vars.arrVal) {
        if (v["name"].asString() == "score") {
            foundScore = true;
            EXPECT_EQ(v["value"].asString(), "99");
            EXPECT_EQ(v["type"].asString(), "int");
        }
    }
    EXPECT_TRUE(foundScore);

    // Evaluate request
    std::string evalReq =
        "{\"seq\":11,\"type\":\"request\",\"command\":\"evaluate\",\"arguments\":{\"frameId\":1,\"expression\":\"score\"}}";
    std::string evalRespStr = server.handleMessage(evalReq);
    JsonValue evalResp = JsonValue::parse(evalRespStr);
    EXPECT_TRUE(evalResp["success"].asBool());
    EXPECT_EQ(evalResp["body"]["result"].asString(), "99");
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
