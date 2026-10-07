#include <gtest/gtest.h>
#include "bindgen/Bindgen.h"
#include "sp/parser/Parser.h"
#include "sp/lexer/Lexer.h"
#include "sp/sema/SemanticAnalyzer.h"

using namespace sp::bindgen;

TEST(BindgenTest, ParseMacrosAndConstants) {
    const std::string header = R"c(
        #define SQLITE_OK 0
        #define SQLITE_ERROR 1
        #define BUFFER_SIZE 0x1000
        #define PI 3.14159
        #define GREETING "Hello from C"
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& macros = bg.getMacros();
    ASSERT_EQ(macros.size(), 5);

    EXPECT_EQ(macros[0].name, "SQLITE_OK");
    EXPECT_EQ(macros[0].value, "0");
    EXPECT_EQ(macros[0].spType, "int32");

    EXPECT_EQ(macros[1].name, "SQLITE_ERROR");
    EXPECT_EQ(macros[1].value, "1");

    EXPECT_EQ(macros[2].name, "BUFFER_SIZE");
    EXPECT_EQ(macros[2].value, "0x1000");

    EXPECT_EQ(macros[3].name, "PI");
    EXPECT_EQ(macros[3].spType, "double");

    EXPECT_EQ(macros[4].name, "GREETING");
    EXPECT_EQ(macros[4].spType, "string");
}

TEST(BindgenTest, ParseEnums) {
    const std::string header = R"c(
        enum Status {
            STATUS_OK = 0,
            STATUS_PENDING = 1,
            STATUS_FAIL = 2
        };

        typedef enum {
            COLOR_RED,
            COLOR_GREEN,
            COLOR_BLUE
        } Color;
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& enums = bg.getEnums();
    ASSERT_EQ(enums.size(), 2);

    EXPECT_EQ(enums[0].name, "Status");
    ASSERT_EQ(enums[0].variants.size(), 3);
    EXPECT_EQ(enums[0].variants[0].name, "STATUS_OK");
    EXPECT_EQ(enums[0].variants[0].value.value(), 0);

    EXPECT_EQ(enums[1].name, "Color");
    ASSERT_EQ(enums[1].variants.size(), 3);
    EXPECT_EQ(enums[1].variants[0].name, "COLOR_RED");
}

TEST(BindgenTest, ParseStructs) {
    const std::string header = R"c(
        struct Point {
            int x;
            int y;
        };

        typedef struct {
            double r;
            double g;
            double b;
            double a;
        } ColorRGBA;

        struct sqlite3;
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& structs = bg.getStructs();
    ASSERT_EQ(structs.size(), 3);

    EXPECT_EQ(structs[0].name, "Point");
    EXPECT_FALSE(structs[0].isOpaque);
    ASSERT_EQ(structs[0].fields.size(), 2);
    EXPECT_EQ(structs[0].fields[0].name, "x");
    EXPECT_EQ(structs[0].fields[0].type.toSPType(), "int32");

    EXPECT_EQ(structs[1].name, "ColorRGBA");
    ASSERT_EQ(structs[1].fields.size(), 4);
    EXPECT_EQ(structs[1].fields[0].name, "r");
    EXPECT_EQ(structs[1].fields[0].type.toSPType(), "double");

    EXPECT_EQ(structs[2].name, "sqlite3");
    EXPECT_TRUE(structs[2].isOpaque);
}

TEST(BindgenTest, ParseFunctionsAndPrototypes) {
    const std::string header = R"c(
        int add(int a, int b);
        void puts(const char* str);
        void* malloc(size_t size);
        void free(void* ptr);
        int sqlite3_open(const char* filename, struct sqlite3** ppDb);
        int __stdcall CustomCall(int code);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& functions = bg.getFunctions();
    ASSERT_EQ(functions.size(), 6);

    EXPECT_EQ(functions[0].name, "add");
    EXPECT_EQ(functions[0].returnType.toSPType(), "int32");
    ASSERT_EQ(functions[0].params.size(), 2);
    EXPECT_EQ(functions[0].params[0].name, "a");
    EXPECT_EQ(functions[0].params[0].type.toSPType(), "int32");

    EXPECT_EQ(functions[1].name, "puts");
    EXPECT_EQ(functions[1].returnType.toSPType(), "void");
    ASSERT_EQ(functions[1].params.size(), 1);
    EXPECT_EQ(functions[1].params[0].type.toSPType(), "ptr<char>");

    EXPECT_EQ(functions[2].name, "malloc");
    EXPECT_EQ(functions[2].returnType.toSPType(), "ptr<void>");
    EXPECT_EQ(functions[2].params[0].type.toSPType(), "uint64");

    EXPECT_EQ(functions[3].name, "free");
    EXPECT_EQ(functions[3].returnType.toSPType(), "void");
    EXPECT_EQ(functions[3].params[0].type.toSPType(), "ptr<void>");

    EXPECT_EQ(functions[4].name, "sqlite3_open");
    EXPECT_EQ(functions[4].returnType.toSPType(), "int32");
    EXPECT_EQ(functions[4].params[0].type.toSPType(), "ptr<char>");
    EXPECT_EQ(functions[4].params[1].type.toSPType(), "ptr<ptr<sqlite3>>");

    EXPECT_EQ(functions[5].name, "CustomCall");
}

TEST(BindgenTest, EndToEndCodeGenerationAndParserVerification) {
    const std::string header = R"c(
        #define SQLITE_OK 0

        struct Point {
            int x;
            int y;
        };

        int calculate(int x, int y);
    )c";

    BindgenOptions opts;
    opts.moduleName = "native";
    Bindgen bg(opts);
    ASSERT_TRUE(bg.parseHeader(header));

    std::string spSource = bg.generateSP();
    EXPECT_NE(spSource.find("module native;"), std::string::npos);
    EXPECT_NE(spSource.find("const SQLITE_OK: int32 = 0;"), std::string::npos);
    EXPECT_NE(spSource.find("#[repr(C)]"), std::string::npos);
    EXPECT_NE(spSource.find("struct Point {"), std::string::npos);
    EXPECT_NE(spSource.find("extern \"C\" {"), std::string::npos);
    EXPECT_NE(spSource.find("fn calculate(x: int32, y: int32): int32;"), std::string::npos);

    // Verify SP Parser accepts the generated bindings without errors
    sp::diagnostics::DiagnosticEngine diagEngine(std::cerr);
    sp::lexer::Lexer lexer(spSource, "test.sp", &diagEngine);
    auto tokens = lexer.tokenizeAll();
    sp::parser::Parser parser(tokens, "test.sp", &diagEngine);
    auto ast = parser.parseProgram();
    EXPECT_FALSE(diagEngine.hasErrors());
    EXPECT_NE(ast, nullptr);
}
