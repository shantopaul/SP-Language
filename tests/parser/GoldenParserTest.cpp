#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/ast/ASTPrinter.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#ifndef SP_PROJECT_ROOT
#define SP_PROJECT_ROOT "."
#endif

namespace sp::parser::test {

namespace {

std::filesystem::path resolveTestFilePath(const std::string& relPath) {
    std::filesystem::path fromRoot = std::filesystem::path(SP_PROJECT_ROOT) / relPath;
    if (std::filesystem::exists(fromRoot)) return fromRoot;

    std::filesystem::path fromCwd = std::filesystem::current_path() / relPath;
    if (std::filesystem::exists(fromCwd)) return fromCwd;

    std::filesystem::path oneUp = std::filesystem::current_path() / ".." / relPath;
    if (std::filesystem::exists(oneUp)) return oneUp;

    std::filesystem::path twoUp = std::filesystem::current_path() / ".." / ".." / relPath;
    if (std::filesystem::exists(twoUp)) return twoUp;

    return fromRoot;
}

std::string readFileContent(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void assertFileParsesCleanly(const std::string& relPath) {
    std::filesystem::path filePath = resolveTestFilePath(relPath);
    ASSERT_TRUE(std::filesystem::exists(filePath)) << "File does not exist: " << filePath;

    std::string content = readFileContent(filePath);
    ASSERT_FALSE(content.empty()) << "File is empty: " << filePath;

    std::ostringstream errStream;
    diagnostics::DiagnosticEngine diag(errStream);

    lexer::Lexer lexer(content, filePath.filename().string(), &diag);
    std::vector<lexer::Token> tokens = lexer.tokenizeAll();
    EXPECT_FALSE(diag.hasErrors()) << "Lexer error in " << filePath << ": " << errStream.str();

    Parser parser(tokens, filePath.filename().string(), &diag);
    std::unique_ptr<ast::Program> program = parser.parseProgram();

    EXPECT_FALSE(diag.hasErrors()) << "Parser error in " << filePath << ": " << errStream.str();
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(program->getDeclarations().empty());

    // Verify ASTPrinter can serialize the resulting tree without crash
    ast::ASTPrinter printer;
    std::string serialized = printer.print(*program);
    EXPECT_FALSE(serialized.empty());
}

} // namespace

TEST(GoldenParserTest, ParseHelloProgram) {
    assertFileParsesCleanly("examples/hello.sp");
}

TEST(GoldenParserTest, ParseFibonacciProgram) {
    assertFileParsesCleanly("examples/fibonacci.sp");
}

TEST(GoldenParserTest, ParseClassesProgram) {
    assertFileParsesCleanly("examples/classes.sp");
}

TEST(GoldenParserTest, ParseStdlibIO) {
    assertFileParsesCleanly("stdlib/io/io.sp");
}

TEST(GoldenParserTest, ParseStdlibMath) {
    assertFileParsesCleanly("stdlib/math/math.sp");
}

TEST(GoldenParserTest, ParseStdlibString) {
    assertFileParsesCleanly("stdlib/string/string.sp");
}

TEST(GoldenParserTest, ParseStdlibCollections) {
    assertFileParsesCleanly("stdlib/collections/collections.sp");
}

TEST(GoldenParserTest, ParseStdlibFS) {
    assertFileParsesCleanly("stdlib/fs/fs.sp");
}

TEST(GoldenParserTest, ParseStdlibTime) {
    assertFileParsesCleanly("stdlib/time/time.sp");
}

TEST(GoldenParserTest, ParseStdlibOS) {
    assertFileParsesCleanly("stdlib/os/os.sp");
}

TEST(GoldenParserTest, ParseStdlibProcess) {
    assertFileParsesCleanly("stdlib/process/process.sp");
}

TEST(GoldenParserTest, ParseStdlibMem) {
    assertFileParsesCleanly("stdlib/mem/mem.sp");
}

TEST(GoldenParserTest, ParseStdlibSync) {
    assertFileParsesCleanly("stdlib/sync/sync.sp");
}

TEST(GoldenParserTest, ParseStdlibNet) {
    assertFileParsesCleanly("stdlib/net/net.sp");
}

TEST(GoldenParserTest, ParseStdlibCrypto) {
    assertFileParsesCleanly("stdlib/crypto/crypto.sp");
}

TEST(GoldenParserTest, ParseStdlibJson) {
    assertFileParsesCleanly("stdlib/json/json.sp");
}

TEST(GoldenParserTest, ParseStdlibHttp) {
    assertFileParsesCleanly("stdlib/http/http.sp");
}

TEST(GoldenParserTest, ParseStdlibPassword) {
    assertFileParsesCleanly("stdlib/crypto/password.sp");
}

TEST(GoldenParserTest, ParseStdlibJwt) {
    assertFileParsesCleanly("stdlib/security/jwt.sp");
}

TEST(GoldenParserTest, ParseStdlibSession) {
    assertFileParsesCleanly("stdlib/security/session.sp");
}

TEST(GoldenParserTest, ParseStdlibSecurityMiddleware) {
    assertFileParsesCleanly("stdlib/security/middleware.sp");
}

TEST(GoldenParserTest, ParseStdlibSecurity) {
    assertFileParsesCleanly("stdlib/security/security.sp");
}

} // namespace sp::parser::test
