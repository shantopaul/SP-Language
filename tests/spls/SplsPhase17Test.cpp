#include <gtest/gtest.h>
#include "spls/LspServer.h"
#include "spls/Json.h"
#include "spls/Protocol.h"
#include <sstream>

using namespace sp::lsp;

class SplsPhase17Test : public ::testing::Test {
protected:
    std::stringstream in;
    std::stringstream out;
};

// 1. Incremental Document Synchronization Test
TEST_F(SplsPhase17Test, IncrementalDocumentSyncUpdatesContent) {
    LspServer server(in, out);
    std::string uri = "test://main.sp";

    // Initial document: 3 lines
    std::string initialText = 
        "fn main(): void {\n"
        "    let x = 10;\n"
        "}\n";

    server.openDocument(uri, initialText);
    EXPECT_EQ(server.getDocument(uri), initialText);

    // Incremental change: replace `let x = 10;` with `let x = 42; let y = 99;`
    // Range: line 1, char 4 to line 1, char 14
    std::string changeReq = 
        "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{"
        "  \"textDocument\":{\"uri\":\"" + uri + "\"},"
        "  \"contentChanges\":[{"
        "    \"range\":{\"start\":{\"line\":1,\"character\":4},\"end\":{\"line\":1,\"character\":14}},"
        "    \"text\":\"let x = 42; let y = 99;\""
        "  }]"
        "}}";

    server.handleMessage(changeReq);
    std::string updated = server.getDocument(uri);

    EXPECT_NE(updated.find("let x = 42; let y = 99;"), std::string::npos);
    EXPECT_EQ(updated.find("let x = 10;"), std::string::npos);
}

// 2. Workspace Symbol Indexing Test
TEST_F(SplsPhase17Test, WorkspaceSymbolIndexingFindsDeclarations) {
    LspServer server(in, out);
    std::string uri1 = "test://service.sp";
    std::string code1 = 
        "class UserService {\n"
        "    fn fetchUser(id: int): string {\n"
        "        return \"user\";\n"
        "    }\n"
        "}\n";

    std::string uri2 = "test://util.sp";
    std::string code2 = 
        "fn calculateHash(data: string): int {\n"
        "    return 42;\n"
        "}\n"
        "struct UserProfile {\n"
        "    let id: int;\n"
        "}\n";

    server.openDocument(uri1, code1);
    server.openDocument(uri2, code2);

    // Query for "User"
    std::string symReq = 
        "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"workspace/symbol\",\"params\":{\"query\":\"User\"}}";
    std::string respStr = server.handleMessage(symReq);
    JsonValue resp = JsonValue::parse(respStr);

    EXPECT_EQ(resp["id"].asInt(), 10);
    EXPECT_TRUE(resp["result"].isArray());

    const auto& symbols = resp["result"].arrVal;
    EXPECT_GE(symbols.size(), 2);

    bool foundClass = false;
    bool foundFn = false;
    for (const auto& s : symbols) {
        std::string n = s["name"].asString();
        if (n == "UserService") foundClass = true;
        if (n == "fetchUser") foundFn = true;
    }
    EXPECT_TRUE(foundClass);
    EXPECT_TRUE(foundFn);
}

// 3. Inlay Type Hints Test
TEST_F(SplsPhase17Test, InlayTypeHintsInfersOmittedTypes) {
    LspServer server(in, out);
    std::string uri = "test://types.sp";
    std::string code = 
        "fn test(): void {\n"
        "    let count = 100;\n"
        "    let message = \"hello\";\n"
        "    let active = true;\n"
        "    let ratio = 3.14;\n"
        "}\n";

    server.openDocument(uri, code);

    std::string hintReq = 
        "{\"jsonrpc\":\"2.0\",\"id\":20,\"method\":\"textDocument/inlayHint\",\"params\":{"
        "  \"textDocument\":{\"uri\":\"" + uri + "\"},"
        "  \"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":5,\"character\":0}}"
        "}}";

    std::string respStr = server.handleMessage(hintReq);
    JsonValue resp = JsonValue::parse(respStr);

    EXPECT_EQ(resp["id"].asInt(), 20);
    EXPECT_TRUE(resp["result"].isArray());

    const auto& hints = resp["result"].arrVal;
    EXPECT_GE(hints.size(), 4);

    bool foundInt = false;
    bool foundString = false;
    bool foundBool = false;
    bool foundFloat = false;

    for (const auto& h : hints) {
        std::string lbl = h["label"].asString();
        if (lbl == ": int") foundInt = true;
        if (lbl == ": string") foundString = true;
        if (lbl == ": bool") foundBool = true;
        if (lbl == ": float") foundFloat = true;
    }

    EXPECT_TRUE(foundInt);
    EXPECT_TRUE(foundString);
    EXPECT_TRUE(foundBool);
    EXPECT_TRUE(foundFloat);
}
