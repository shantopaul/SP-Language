#include <gtest/gtest.h>
#include "spls/Json.h"
#include "spls/Protocol.h"
#include "spls/LspServer.h"

#include <sstream>

using namespace sp::lsp;

TEST(LspJsonTest, ParseAndSerializePrimitives) {
    std::string jsonStr = "{\"name\":\"spls\",\"version\":1,\"enabled\":true,\"extra\":null}";
    JsonValue v = JsonValue::parse(jsonStr);

    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v["name"].asString(), "spls");
    EXPECT_EQ(v["version"].asInt(), 1);
    EXPECT_TRUE(v["enabled"].asBool());
    EXPECT_TRUE(v["extra"].isNull());

    std::string serialized = v.serialize();
    EXPECT_NE(serialized.find("\"name\":\"spls\""), std::string::npos);
    EXPECT_NE(serialized.find("\"enabled\":true"), std::string::npos);
}

TEST(LspJsonTest, ParseAndSerializeArrays) {
    std::string jsonStr = "[\"a\",\"b\",42]";
    JsonValue v = JsonValue::parse(jsonStr);

    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v[0].asString(), "a");
    EXPECT_EQ(v[1].asString(), "b");
    EXPECT_EQ(v[2].asInt(), 42);
}

TEST(LspServerTest, MessageFraming) {
    std::string body = "{\"jsonrpc\":\"2.0\"}";
    std::string framed = LspServer::frameMessage(body);
    EXPECT_EQ(framed, "Content-Length: 17\r\n\r\n{\"jsonrpc\":\"2.0\"}");

    std::istringstream stream(framed);
    size_t length = 0;
    EXPECT_TRUE(LspServer::parseHeader(stream, length));
    EXPECT_EQ(length, 17);
}

TEST(LspServerTest, InitializeAndCapabilities) {
    std::stringstream in;
    std::stringstream out;
    LspServer server(in, out);

    std::string initReq = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";
    std::string responseStr = server.handleMessage(initReq);
    JsonValue resp = JsonValue::parse(responseStr);

    EXPECT_EQ(resp["id"].asInt(), 1);
    EXPECT_TRUE(resp["result"].isObject());

    const auto& caps = resp["result"]["capabilities"];
    EXPECT_EQ(caps["textDocumentSync"].asInt(), 1);
    EXPECT_TRUE(caps["hoverProvider"].asBool());
    EXPECT_TRUE(caps["definitionProvider"].asBool());
    EXPECT_TRUE(caps["documentFormattingProvider"].asBool());
    EXPECT_TRUE(caps["renameProvider"].asBool());
    EXPECT_TRUE(caps["completionProvider"].isObject());
}

TEST(LspServerTest, ComputeDiagnostics) {
    std::stringstream in;
    std::stringstream out;
    LspServer server(in, out);

    std::string validCode = "fn add(a: int, b: int): int {\n    return a + b;\n}\n";
    auto validDiags = server.computeDiagnostics("test://valid.sp", validCode);
    EXPECT_EQ(validDiags.size(), 0);

    std::string invalidCode = "fn broken() {\n    let x = ;\n}\n";
    auto invalidDiags = server.computeDiagnostics("test://invalid.sp", invalidCode);
    EXPECT_GT(invalidDiags.size(), 0);
}

TEST(LspServerTest, CompletionProvidesKeywordsAndSymbols) {
    std::stringstream in;
    std::stringstream out;
    LspServer server(in, out);

    std::string uri = "test://comp.sp";
    std::string source = 
        "/// Calculates total\n"
        "fn calculate(x: int): int {\n"
        "    return x * 2;\n"
        "}\n";

    server.openDocument(uri, source);

    std::string compReq = 
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"textDocument/completion\","
        "\"params\":{\"textDocument\":{\"uri\":\"" + uri + "\"},\"position\":{\"line\":1,\"character\":4}}}";

    std::string respStr = server.handleMessage(compReq);
    JsonValue resp = JsonValue::parse(respStr);
    EXPECT_EQ(resp["id"].asInt(), 2);
    EXPECT_TRUE(resp["result"].isArray());

    bool foundFnKw = false;
    bool foundIntType = false;
    bool foundCalculate = false;

    for (const auto& item : resp["result"].arrVal) {
        std::string label = item["label"].asString();
        if (label == "fn") foundFnKw = true;
        if (label == "int") foundIntType = true;
        if (label == "calculate") foundCalculate = true;
    }

    EXPECT_TRUE(foundFnKw);
    EXPECT_TRUE(foundIntType);
    EXPECT_TRUE(foundCalculate);
}

TEST(LspServerTest, HoverDocumentation) {
    std::stringstream in;
    std::stringstream out;
    LspServer server(in, out);

    std::string uri = "test://hover.sp";
    std::string source = 
        "/// Multiplies value by 10\n"
        "fn scale(val: int): int {\n"
        "    return val * 10;\n"
        "}\n";

    server.openDocument(uri, source);

    std::string hoverReq = 
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"textDocument/hover\","
        "\"params\":{\"textDocument\":{\"uri\":\"" + uri + "\"},\"position\":{\"line\":1,\"character\":5}}}";

    std::string respStr = server.handleMessage(hoverReq);
    JsonValue resp = JsonValue::parse(respStr);
    EXPECT_EQ(resp["id"].asInt(), 3);
    EXPECT_TRUE(resp["result"].isObject());

    std::string contents = resp["result"]["contents"]["value"].asString();
    EXPECT_NE(contents.find("scale"), std::string::npos);
    EXPECT_NE(contents.find("Multiplies value by 10"), std::string::npos);
}

TEST(LspServerTest, DocumentFormatting) {
    std::stringstream in;
    std::stringstream out;
    LspServer server(in, out);

    std::string uri = "test://format.sp";
    std::string unformatted = "fn foo(){\nlet x=1+2;\nreturn x;\n}\n";
    server.openDocument(uri, unformatted);

    std::string fmtReq = 
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"textDocument/formatting\","
        "\"params\":{\"textDocument\":{\"uri\":\"" + uri + "\"},\"options\":{\"tabSize\":4}}}";

    std::string respStr = server.handleMessage(fmtReq);
    JsonValue resp = JsonValue::parse(respStr);
    EXPECT_EQ(resp["id"].asInt(), 4);
    EXPECT_TRUE(resp["result"].isArray());
    ASSERT_EQ(resp["result"].arrVal.size(), 1);

    std::string newText = resp["result"][0]["newText"].asString();
    EXPECT_NE(newText.find("fn foo() {"), std::string::npos);
    EXPECT_NE(newText.find("    let x = 1 + 2;"), std::string::npos);
}

TEST(LspServerTest, RenameSymbol) {
    std::stringstream in;
    std::stringstream out;
    LspServer server(in, out);

    std::string uri = "test://rename.sp";
    std::string source = "fn test() {\n    let count = 0;\n    count = count + 1;\n}\n";
    server.openDocument(uri, source);

    std::string renameReq = 
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"textDocument/rename\","
        "\"params\":{\"textDocument\":{\"uri\":\"" + uri + "\"},\"position\":{\"line\":1,\"character\":10},\"newName\":\"total\"}}";

    std::string respStr = server.handleMessage(renameReq);
    JsonValue resp = JsonValue::parse(respStr);
    EXPECT_EQ(resp["id"].asInt(), 5);

    const auto& changes = resp["result"]["changes"][uri];
    EXPECT_TRUE(changes.isArray());
    EXPECT_EQ(changes.arrVal.size(), 3); // 3 occurrences of 'count' renamed to 'total'
    for (const auto& edit : changes.arrVal) {
        EXPECT_EQ(edit["newText"].asString(), "total");
    }
}
