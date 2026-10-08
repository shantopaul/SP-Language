#include <gtest/gtest.h>
#include "sp/sema/ModuleLoader.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/util/PathUtils.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace sp::diagnostics;
using namespace sp::lexer;
using namespace sp::parser;
using namespace sp::sema;

class ModuleLoaderTest : public ::testing::Test {
protected:
    fs::path tempDir;

    void SetUp() override {
        std::error_code ec;
        tempDir = fs::temp_directory_path() / ("sp_module_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(tempDir, ec);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(tempDir, ec);
    }

    void writeSourceFile(const fs::path& relPath, const std::string& content) {
        fs::path fullPath = tempDir / relPath;
        std::error_code ec;
        fs::create_directories(fullPath.parent_path(), ec);
        std::ofstream out(fullPath);
        out << content;
    }
};

TEST_F(ModuleLoaderTest, ResolveStdlibPath) {
    DiagnosticEngine diag(std::cerr);
    ModuleLoader loader(diag);

    fs::path stdlibDir = sp::util::getStdlibDir();
    loader.setStdlibPath(stdlibDir);

    fs::path httpPath = loader.resolveModulePath("std.http", tempDir / "main.sp");
    EXPECT_FALSE(httpPath.empty());
    EXPECT_TRUE(fs::exists(httpPath));
    EXPECT_EQ(httpPath.filename().string(), "http.sp");

    fs::path jsonPath = loader.resolveModulePath("std.json", tempDir / "main.sp");
    EXPECT_FALSE(jsonPath.empty());
    EXPECT_TRUE(fs::exists(jsonPath));
    EXPECT_EQ(jsonPath.filename().string(), "json.sp");

    fs::path cryptoPath = loader.resolveModulePath("std.crypto", tempDir / "main.sp");
    EXPECT_FALSE(cryptoPath.empty());
    EXPECT_TRUE(fs::exists(cryptoPath));

    fs::path netPath = loader.resolveModulePath("std.net", tempDir / "main.sp");
    EXPECT_FALSE(netPath.empty());
    EXPECT_TRUE(fs::exists(netPath));
}

TEST_F(ModuleLoaderTest, ResolveRelativePath) {
    DiagnosticEngine diag(std::cerr);
    ModuleLoader loader(diag);

    writeSourceFile("routes/users.sp", "public fn getUserName(): string { return \"Alice\"; }");
    writeSourceFile("models/user.sp", "struct User { string name; }");

    fs::path rootFile = tempDir / "main.sp";

    fs::path usersPath = loader.resolveModulePath("routes.users", rootFile);
    EXPECT_FALSE(usersPath.empty());
    EXPECT_TRUE(fs::exists(usersPath));

    fs::path userModelPath = loader.resolveModulePath("models.user", rootFile);
    EXPECT_FALSE(userModelPath.empty());
    EXPECT_TRUE(fs::exists(userModelPath));
}

TEST_F(ModuleLoaderTest, MultiFileCompilationMergingAndTypeChecking) {
    writeSourceFile("math/calc.sp", R"(
        public fn add(int a, int b) -> int {
            return a + b;
        }

        struct Vector2 {
            int x;
            int y;
        }
    )");

    writeSourceFile("main.sp", R"(
        import math.calc;

        fn main() {
            var sum = add(10, 20);
            Vector2 v;
            v.x = sum;
            v.y = 42;
        }
    )");

    DiagnosticEngine diag(std::cerr);
    fs::path rootFile = tempDir / "main.sp";
    std::ifstream file(rootFile);
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    Lexer lexer(source, rootFile.string(), &diag);
    auto tokens = lexer.tokenizeAll();
    ASSERT_FALSE(diag.hasErrors());

    Parser parser(tokens, rootFile.string(), &diag);
    auto program = parser.parseProgram();
    ASSERT_TRUE(program != nullptr);
    ASSERT_FALSE(diag.hasErrors());

    ModuleLoader loader(diag);
    loader.addSearchPath(tempDir);
    bool mergeSuccess = loader.loadAndMergeAll(*program, rootFile);
    EXPECT_TRUE(mergeSuccess);
    EXPECT_FALSE(diag.hasErrors());

    // Verify Semantic Analysis passes with symbols from math/calc.sp resolved
    SemanticAnalyzer sema(diag);
    bool semaSuccess = sema.analyze(*program);
    EXPECT_TRUE(semaSuccess);
    EXPECT_FALSE(diag.hasErrors());
}

TEST_F(ModuleLoaderTest, TransitiveImportsResolution) {
    // Module C: deep dependency
    writeSourceFile("deep/c.sp", R"(
        public fn deepValue() -> int {
            return 999;
        }
    )");

    // Module B: imports C
    writeSourceFile("mid/b.sp", R"(
        import deep.c;

        public fn midCompute() -> int {
            return deepValue() + 1;
        }
    )");

    // Module A (main): imports B
    writeSourceFile("main.sp", R"(
        import mid.b;

        fn main() {
            var res = midCompute();
            println(res);
        }
    )");

    DiagnosticEngine diag(std::cerr);
    fs::path rootFile = tempDir / "main.sp";
    std::ifstream file(rootFile);
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    Lexer lexer(source, rootFile.string(), &diag);
    auto tokens = lexer.tokenizeAll();
    Parser parser(tokens, rootFile.string(), &diag);
    auto program = parser.parseProgram();
    ASSERT_TRUE(program != nullptr);

    ModuleLoader loader(diag);
    loader.addSearchPath(tempDir);
    bool mergeSuccess = loader.loadAndMergeAll(*program, rootFile);
    EXPECT_TRUE(mergeSuccess);
    EXPECT_FALSE(diag.hasErrors());

    SemanticAnalyzer sema(diag);
    EXPECT_TRUE(sema.analyze(*program));
    EXPECT_FALSE(diag.hasErrors());
}

TEST_F(ModuleLoaderTest, CircularDependencyDetectionReportsE1010) {
    // Module A imports Module B
    writeSourceFile("a.sp", R"(
        import b;
        public fn foo(): int { return 1; }
    )");

    // Module B imports Module A (cycle)
    writeSourceFile("b.sp", R"(
        import a;
        public fn bar(): int { return 2; }
    )");

    writeSourceFile("main.sp", R"(
        import a;
        fn main() {
            foo();
        }
    )");

    DiagnosticEngine diag(std::cerr);
    fs::path rootFile = tempDir / "main.sp";
    std::ifstream file(rootFile);
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    Lexer lexer(source, rootFile.string(), &diag);
    auto tokens = lexer.tokenizeAll();
    Parser parser(tokens, rootFile.string(), &diag);
    auto program = parser.parseProgram();
    ASSERT_TRUE(program != nullptr);

    ModuleLoader loader(diag);
    loader.addSearchPath(tempDir);
    bool mergeSuccess = loader.loadAndMergeAll(*program, rootFile);

    // Must detect circular import error E1010
    EXPECT_FALSE(mergeSuccess);
    EXPECT_TRUE(diag.hasErrors());
}

TEST_F(ModuleLoaderTest, NonExistentModuleReportsE1009) {
    writeSourceFile("main.sp", R"(
        import non_existent_pkg.missing_mod;
        fn main() {}
    )");

    DiagnosticEngine diag(std::cerr);
    fs::path rootFile = tempDir / "main.sp";
    std::ifstream file(rootFile);
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    Lexer lexer(source, rootFile.string(), &diag);
    auto tokens = lexer.tokenizeAll();
    Parser parser(tokens, rootFile.string(), &diag);
    auto program = parser.parseProgram();
    ASSERT_TRUE(program != nullptr);

    ModuleLoader loader(diag);
    loader.addSearchPath(tempDir);
    bool mergeSuccess = loader.loadAndMergeAll(*program, rootFile);

    EXPECT_FALSE(mergeSuccess);
    EXPECT_TRUE(diag.hasErrors());
}
