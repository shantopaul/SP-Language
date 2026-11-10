// Phase 4 slice B (pro-4.0.0): protocol-level tests for the LSP depth
// work — scope-aware rename/references, locals in hover/definition,
// documentSymbol, semantic tokens, member completion, real-type inlay
// hints, and workspace indexing of unopened files.
//
// Every test drives the server through handleMessage (real JSON-RPC
// strings), the same path a client uses; the Python probe client
// (/tmp/sp-p4-lsp-probe.py) exercises the same capabilities end to end
// over stdio against the built binary.

#include <gtest/gtest.h>
#include "spls/Json.h"
#include "spls/Protocol.h"
#include "spls/LspServer.h"

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace sp::lsp;

namespace {

// Shared probe program (0-based line numbers in comments).
const char* kProbeSrc =
    "/// Adds two numbers\n"                                    // 0
    "fn add(a: int, b: int) -> int {\n"                  // 1
    "    return a + b;\n"                                 // 2
    "}\n"                                                 // 3
    "\n"                                                  // 4
    "class Greeter {\n"                                   // 5
    "    public fn greet(name: string) -> string {\n"    // 6
    "        return \"Hello, \" + name;\n"               // 7
    "    }\n"                                             // 8
    "}\n"                                                 // 9
    "\n"                                                  // 10
    "fn helper() {\n"                                     // 11
    "    let count = 0;\n"                                // 12
    "    count = count + 1;\n"                            // 13
    "}\n"                                                 // 14
    "\n"                                                  // 15
    "fn main() {\n"                                       // 16
    "    let total = add(2, 3);\n"                        // 17
    "    let g = new Greeter();\n"                        // 18
    "    let msg = g.greet(\"World\");\n"                 // 19
    "    let count = 0;\n"                                // 20
    "    count = count + 1;\n"                            // 21
    "    println(total);\n"                               // 22
    "}\n";                                                // 23

const char* kUri = "file:///tmp/lsp-depth-test/main.sp";

JsonValue call(LspServer& server, int id, const std::string& method, const std::string& paramsJson) {
    std::string req = "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(id) +
                      ",\"method\":\"" + method + "\",\"params\":" + paramsJson + "}";
    return JsonValue::parse(server.handleMessage(req));
}

std::string posJson(int line, int character) {
    return "{\"line\":" + std::to_string(line) + ",\"character\":" + std::to_string(character) + "}";
}

std::string textDocParams(int line, int character) {
    return "{\"textDocument\":{\"uri\":\"" + std::string(kUri) + "\"},\"position\":" + posJson(line, character) + "}";
}

bool hasLabel(const JsonValue& arr, const std::string& label) {
    if (!arr.isArray()) return false;
    for (const auto& item : arr.arrVal) {
        if (item["label"].asString() == label) return true;
    }
    return false;
}

struct ServerFixture {
    std::stringstream in;
    std::stringstream out;
    LspServer server;
    ServerFixture() : server(in, out) { server.openDocument(kUri, kProbeSrc); }
};

} // namespace

TEST(LspDepthTest, HoverFunctionDefShowsDocComment) {
    ServerFixture f;
    // `add` name on its declaration line (line 1, col 4).
    JsonValue resp = call(f.server, 1, "textDocument/hover", textDocParams(1, 4));
    ASSERT_TRUE(resp["result"].isObject());
    std::string contents = resp["result"]["contents"]["value"].asString();
    EXPECT_NE(contents.find("Adds two numbers"), std::string::npos);
    EXPECT_EQ(resp["result"]["range"]["start"]["line"].asInt(), 1);
    EXPECT_EQ(resp["result"]["range"]["start"]["character"].asInt(), 3);
    EXPECT_EQ(resp["result"]["range"]["end"]["character"].asInt(), 6);
}

TEST(LspDepthTest, HoverLocalVariable) {
    ServerFixture f;
    // `total` used inside println (line 22, col 13).
    JsonValue resp = call(f.server, 1, "textDocument/hover", textDocParams(22, 13));
    ASSERT_TRUE(resp["result"].isObject());
    std::string contents = resp["result"]["contents"]["value"].asString();
    EXPECT_NE(contents.find("total"), std::string::npos);
    EXPECT_NE(contents.find("int"), std::string::npos);
}

TEST(LspDepthTest, DefinitionLocalVariable) {
    ServerFixture f;
    JsonValue resp = call(f.server, 1, "textDocument/definition", textDocParams(22, 13));
    ASSERT_TRUE(resp["result"].isObject());
    EXPECT_EQ(resp["result"]["range"]["start"]["line"].asInt(), 17);
    EXPECT_EQ(resp["result"]["range"]["start"]["character"].asInt(), 8);
}

TEST(LspDepthTest, HoverParameter) {
    ServerFixture f;
    // `a` used in `return a + b;` (line 2, col 11).
    JsonValue resp = call(f.server, 1, "textDocument/hover", textDocParams(2, 11));
    ASSERT_TRUE(resp["result"].isObject());
    std::string contents = resp["result"]["contents"]["value"].asString();
    EXPECT_NE(contents.find("a"), std::string::npos);
    EXPECT_NE(contents.find("int"), std::string::npos);
}

TEST(LspDepthTest, DefinitionMethodCall) {
    ServerFixture f;
    // `greet` in `g.greet("World")` (line 19, col 17).
    JsonValue resp = call(f.server, 1, "textDocument/definition", textDocParams(19, 17));
    ASSERT_TRUE(resp["result"].isObject());
    EXPECT_EQ(resp["result"]["range"]["start"]["line"].asInt(), 6);
    EXPECT_EQ(resp["result"]["range"]["start"]["character"].asInt(), 14);
}

TEST(LspDepthTest, ReferencesLocalVariableScoped) {
    ServerFixture f;
    // `count` declared in main (line 20, col 9). helper() has its own
    // `count` (lines 12-13) — it must NOT appear.
    JsonValue resp = call(f.server, 1, "textDocument/references", textDocParams(20, 9));
    ASSERT_TRUE(resp["result"].isArray());
    ASSERT_EQ(resp["result"].arrVal.size(), 3);
    for (const auto& ref : resp["result"].arrVal) {
        int line = static_cast<int>(ref["range"]["start"]["line"].asInt());
        EXPECT_TRUE(line == 20 || line == 21) << "unexpected reference on line " << line;
    }
}

TEST(LspDepthTest, ReferencesFunction) {
    ServerFixture f;
    // `add` used in main (line 17, col 17): declaration + one use.
    JsonValue resp = call(f.server, 1, "textDocument/references", textDocParams(17, 17));
    ASSERT_TRUE(resp["result"].isArray());
    ASSERT_EQ(resp["result"].arrVal.size(), 2);
    EXPECT_EQ(resp["result"].arrVal[0]["range"]["start"]["line"].asInt(), 1);
    EXPECT_EQ(resp["result"].arrVal[1]["range"]["start"]["line"].asInt(), 17);
}

TEST(LspDepthTest, RenameLocalScoped) {
    ServerFixture f;
    std::string params = "{\"textDocument\":{\"uri\":\"" + std::string(kUri) +
                         "\"},\"position\":" + posJson(20, 9) + ",\"newName\":\"total2\"}";
    JsonValue resp = call(f.server, 1, "textDocument/rename", params);
    const auto& changes = resp["result"]["changes"][kUri];
    ASSERT_TRUE(changes.isArray());
    ASSERT_EQ(changes.arrVal.size(), 3);
    for (const auto& edit : changes.arrVal) {
        EXPECT_EQ(edit["newText"].asString(), "total2");
        int line = static_cast<int>(edit["range"]["start"]["line"].asInt());
        EXPECT_TRUE(line == 20 || line == 21) << "rename leaked to line " << line;
    }
}

TEST(LspDepthTest, DocumentSymbolOutline) {
    ServerFixture f;
    JsonValue resp = call(f.server, 1, "textDocument/documentSymbol",
                          "{\"textDocument\":{\"uri\":\"" + std::string(kUri) + "\"}}");
    ASSERT_TRUE(resp["result"].isArray());
    // Top level: add, Greeter, helper, main — no locals.
    ASSERT_EQ(resp["result"].arrVal.size(), 4);
    bool sawGreeterChildren = false;
    for (const auto& sym : resp["result"].arrVal) {
        std::string name = sym["name"].asString();
        EXPECT_TRUE(name == "add" || name == "Greeter" || name == "helper" || name == "main")
            << "unexpected top-level symbol: " << name;
        if (name == "Greeter") {
            ASSERT_TRUE(sym["children"].isArray());
            ASSERT_EQ(sym["children"].arrVal.size(), 1);
            EXPECT_EQ(sym["children"].arrVal[0]["name"].asString(), "greet");
            sawGreeterChildren = true;
        }
    }
    EXPECT_TRUE(sawGreeterChildren);
}

TEST(LspDepthTest, SemanticTokensBasic) {
    ServerFixture f;
    JsonValue resp = call(f.server, 1, "textDocument/semanticTokens/full",
                          "{\"textDocument\":{\"uri\":\"" + std::string(kUri) + "\"}}");
    ASSERT_TRUE(resp["result"].isObject());
    const auto& data = resp["result"]["data"];
    ASSERT_TRUE(data.isArray());
    ASSERT_GT(data.arrVal.size(), 0);
    ASSERT_EQ(data.arrVal.size() % 5, 0u);

    // Decode the delta stream and check a few known tokens.
    int line = 0, col = 0;
    bool sawFnKeyword = false, sawAddFunction = false, sawString = false, sawGreeterClass = false;
    for (size_t i = 0; i + 4 < data.arrVal.size(); i += 5) {
        int dLine = static_cast<int>(data.arrVal[i].asInt());
        int dCol = static_cast<int>(data.arrVal[i + 1].asInt());
        int len = static_cast<int>(data.arrVal[i + 2].asInt());
        int type = static_cast<int>(data.arrVal[i + 3].asInt());
        int mods = static_cast<int>(data.arrVal[i + 4].asInt());
        line += dLine;
        col = (dLine == 0) ? col + dCol : dCol;
        if (line == 1 && col == 0 && len == 2 && type == 13) sawFnKeyword = true;      // `fn`
        if (line == 1 && col == 3 && len == 3 && type == 11 && (mods & 1)) sawAddFunction = true; // `add` decl
        if (line == 7 && col == 15 && len == 9 && type == 15) sawString = true;         // "Hello, "
        if (line == 18 && col == 16 && len == 7 && type == 2) sawGreeterClass = true;    // Greeter
    }
    EXPECT_TRUE(sawFnKeyword);
    EXPECT_TRUE(sawAddFunction);
    EXPECT_TRUE(sawString);
    EXPECT_TRUE(sawGreeterClass);
}

// Phase 4 slice B: the lexer returns string lexemes decoded (quotes
// stripped, escapes processed), so a semantic token's length must be
// measured from the source text — `"a\nb"` spans 6 source chars, not
// the 3 decoded ones — or editors highlight the wrong range.
TEST(LspDepthTest, SemanticTokensStringLengthCountsSourceSpan) {
    std::stringstream in, out;
    LspServer server(in, out);
    std::string uri = "file:///tmp/lsp-depth-test/escapes.sp";
    server.openDocument(uri, "fn main() {\n    let s = \"a\\nb\";\n}\n");
    JsonValue resp = call(server, 1, "textDocument/semanticTokens/full",
                          "{\"textDocument\":{\"uri\":\"" + uri + "\"}}");
    ASSERT_TRUE(resp["result"].isObject());
    const auto& data = resp["result"]["data"];
    ASSERT_TRUE(data.isArray());
    int line = 0, col = 0;
    bool sawEscapedString = false;
    for (size_t i = 0; i + 4 < data.arrVal.size(); i += 5) {
        int dLine = static_cast<int>(data.arrVal[i].asInt());
        int dCol = static_cast<int>(data.arrVal[i + 1].asInt());
        int len = static_cast<int>(data.arrVal[i + 2].asInt());
        int type = static_cast<int>(data.arrVal[i + 3].asInt());
        line += dLine;
        col = (dLine == 0) ? col + dCol : dCol;
        if (line == 1 && col == 12 && len == 6 && type == 15) sawEscapedString = true;
    }
    EXPECT_TRUE(sawEscapedString) << "escaped string token missing or wrong length";
}

TEST(LspDepthTest, MemberCompletionAfterDot) {
    ServerFixture f;
    // Cursor just after `g.` in `g.greet(...)` (line 19, col 16).
    JsonValue resp = call(f.server, 1, "textDocument/completion", textDocParams(19, 16));
    ASSERT_TRUE(resp["result"].isArray());
    EXPECT_TRUE(hasLabel(resp["result"], "greet"));
    EXPECT_FALSE(hasLabel(resp["result"], "add")) << "member completion leaked a top-level function";
    EXPECT_FALSE(hasLabel(resp["result"], "main")) << "member completion leaked a top-level function";
}

TEST(LspDepthTest, CompletionIncludesLocals) {
    ServerFixture f;
    // Inside println(total) (line 22, col 13): locals must be offered.
    JsonValue resp = call(f.server, 1, "textDocument/completion", textDocParams(22, 13));
    ASSERT_TRUE(resp["result"].isArray());
    EXPECT_TRUE(hasLabel(resp["result"], "total"));
    EXPECT_TRUE(hasLabel(resp["result"], "g"));
    EXPECT_TRUE(hasLabel(resp["result"], "msg"));
    EXPECT_TRUE(hasLabel(resp["result"], "add"));
}

TEST(LspDepthTest, InlayHintRealTypes) {
    ServerFixture f;
    std::string params = "{\"textDocument\":{\"uri\":\"" + std::string(kUri) +
                         "\"},\"range\":{\"start\":{\"line\":0,\"character\":0},"
                         "\"end\":{\"line\":23,\"character\":0}}}";
    JsonValue resp = call(f.server, 1, "textDocument/inlayHint", params);
    ASSERT_TRUE(resp["result"].isArray());
    bool sawInt = false, sawGreeter = false, sawString = false, sawAny = false;
    for (const auto& hint : resp["result"].arrVal) {
        std::string label = hint["label"].asString();
        int line = static_cast<int>(hint["position"]["line"].asInt());
        if (label == ": any") sawAny = true;
        if (line == 17 && label == ": int") sawInt = true;
        if (line == 18 && label == ": Greeter") sawGreeter = true;
        if (line == 19 && label == ": string") sawString = true;
    }
    EXPECT_TRUE(sawInt) << "missing ': int' for add(2, 3)";
    EXPECT_TRUE(sawGreeter) << "missing ': Greeter' for new Greeter()";
    EXPECT_TRUE(sawString) << "missing ': string' for g.greet(...)";
    EXPECT_FALSE(sawAny) << "a hint still fell back to ': any'";
}

TEST(LspDepthTest, InitializeAdvertisesNewProviders) {
    std::stringstream in, out;
    LspServer server(in, out);
    JsonValue resp = call(server, 1, "initialize", "{}");
    const auto& caps = resp["result"]["capabilities"];
    EXPECT_TRUE(caps["referencesProvider"].asBool());
    EXPECT_TRUE(caps["documentSymbolProvider"].asBool());
    ASSERT_TRUE(caps["semanticTokensProvider"].isObject());
    bool sawFunction = false;
    for (const auto& t : caps["semanticTokensProvider"]["legend"]["tokenTypes"].arrVal) {
        if (t.asString() == "function") sawFunction = true;
    }
    EXPECT_TRUE(sawFunction);
}

TEST(LspDepthTest, WorkspaceSymbolFindsUnopenedFile) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "spls-depth-test-ws";
    fs::create_directories(dir);
    {
        std::ofstream f(dir / "helper.sp");
        f << "fn helperFn() {\n    return 1;\n}\n";
    }
    std::string rootUri = "file://" + dir.string();
    std::string helperUri = "file://" + (dir / "helper.sp").string();

    std::stringstream in, out;
    LspServer server(in, out);
    JsonValue init = call(server, 1, "initialize", "{\"rootUri\":\"" + rootUri + "\"}");
    EXPECT_TRUE(init["result"].isObject());

    JsonValue resp = call(server, 2, "workspace/symbol", "{\"query\":\"helperFn\"}");
    ASSERT_TRUE(resp["result"].isArray());
    ASSERT_EQ(resp["result"].arrVal.size(), 1);
    EXPECT_EQ(resp["result"].arrVal[0]["name"].asString(), "helperFn");
    EXPECT_EQ(resp["result"].arrVal[0]["location"]["uri"].asString(), helperUri);

    // Deleting the file via didChangeWatchedFiles removes it from results.
    std::string notif = "{\"jsonrpc\":\"2.0\",\"method\":\"workspace/didChangeWatchedFiles\","
                        "\"params\":{\"changes\":[{\"uri\":\"" + helperUri + "\",\"type\":3}]}}";
    EXPECT_EQ(server.handleMessage(notif), "");
    JsonValue resp2 = call(server, 3, "workspace/symbol", "{\"query\":\"helperFn\"}");
    ASSERT_TRUE(resp2["result"].isArray());
    EXPECT_EQ(resp2["result"].arrVal.size(), 0u);

    fs::remove_all(dir);
}

// Phase 4 slice B (segfault fix): `fn test()` is a keyword used as a
// function name. The parser's error recovery then builds a program with
// ExprStmts whose expression is null, and the index used to dereference
// it unconditionally (SIGSEGV in walkExpr). The index must survive and
// still produce real-type hints for the lets inside the stray block.
TEST(LspDepthTest, InlayHintOnKeywordNamedFunction) {
    std::stringstream in, out;
    LspServer server(in, out);
    std::string uri = "file:///tmp/lsp-depth-test/types.sp";
    server.openDocument(uri,
        "fn test(): void {\n"
        "    let count = 100;\n"
        "    let message = \"hello\";\n"
        "    let active = true;\n"
        "    let ratio = 3.14;\n"
        "}\n");
    std::string params = "{\"textDocument\":{\"uri\":\"" + uri +
                         "\"},\"range\":{\"start\":{\"line\":0,\"character\":0},"
                         "\"end\":{\"line\":5,\"character\":0}}}";
    JsonValue resp = call(server, 1, "textDocument/inlayHint", params);
    ASSERT_TRUE(resp["result"].isArray());
    ASSERT_EQ(resp["result"].arrVal.size(), 4u);
    bool sawInt = false, sawString = false, sawBool = false, sawFloat = false;
    for (const auto& hint : resp["result"].arrVal) {
        std::string label = hint["label"].asString();
        EXPECT_EQ(label.find("type-error"), std::string::npos)
            << "sema's error type leaked into a hint: " << label;
        if (label == ": int") sawInt = true;
        if (label == ": string") sawString = true;
        if (label == ": bool") sawBool = true;
        if (label == ": float") sawFloat = true;
    }
    EXPECT_TRUE(sawInt);
    EXPECT_TRUE(sawString);
    EXPECT_TRUE(sawBool);
    EXPECT_TRUE(sawFloat);
}

// Phase 4 slice B (segfault fix): parseExpression() returns nullptr for
// any invalid prefix, so error recovery can leave null children all over
// the AST (null if/while condition, null binary RHS, null unary operand,
// null ternary branch, null assign value, null index, null lambda body,
// null await operand, null throw expression, null switch-case value,
// null match-arm body, null for-in iterable). The index must skip them
// instead of crashing — each program below must answer, not segfault.
TEST(LspDepthTest, MalformedProgramsDoNotCrashIndex) {
    const char* kBroken[] = {
        "fn main() { if ; }\n",
        "fn main() { while ; }\n",
        "fn main() { let x = 1 + ; }\n",
        "fn main() { let x = -; }\n",
        "fn main() { let x = true ? 1 : ; }\n",
        "fn main() { let x = arr[;]; }\n",
        "fn main() { let x = await ; }\n",
        "fn main() { throw; }\n",
        "fn main() { let x = 1; x = ; }\n",
        "fn main() { let x = 1; switch x { case : break; } }\n",
        "fn main() { let x = 1; match x { 1 => ; } }\n",
        "fn main() { for i in ; {} }\n",
        "fn main() { let f = x => ; }\n",
        "fn main() { let x = foo(1, ;); }\n",
        "fn main() { let x = [1, ;]; }\n",
    };
    for (const char* src : kBroken) {
        std::stringstream in, out;
        LspServer server(in, out);
        std::string uri = "file:///tmp/lsp-depth-test/broken.sp";
        server.openDocument(uri, src);
        std::string hintParams = "{\"textDocument\":{\"uri\":\"" + uri +
                                 "\"},\"range\":{\"start\":{\"line\":0,\"character\":0},"
                                 "\"end\":{\"line\":0,\"character\":0}}}";
        JsonValue hints = call(server, 1, "textDocument/inlayHint", hintParams);
        EXPECT_TRUE(hints["result"].isArray()) << "inlayHint failed for: " << src;
        JsonValue symbols = call(server, 2, "textDocument/documentSymbol",
                                 "{\"textDocument\":{\"uri\":\"" + uri + "\"}}");
        EXPECT_TRUE(symbols["result"].isArray()) << "documentSymbol failed for: " << src;
    }
}
