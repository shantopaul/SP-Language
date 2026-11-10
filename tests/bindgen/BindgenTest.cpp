#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
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

// Phase 39 slice C: convention macros (ZEXTERN/ZEXPORT/FAR) and the
// z_const macro must not leak into types, and typedefs must resolve
// at use sites (zlib-style declarations).
TEST(BindgenTest, MacroPrefixedPrototypes) {
    const std::string header = R"c(
        typedef unsigned char Byte;
        typedef Byte FAR Bytef;
        typedef unsigned long uLong;
        typedef uLong FAR uLongf;
        typedef void FAR *voidpf;
        typedef struct z_stream_s z_stream;
        typedef z_stream FAR *z_streamp;
        ZEXTERN int ZEXPORT deflateInit(z_streamp strm, int level);
        ZEXTERN int ZEXPORT compress(Bytef *dest, uLongf *destLen, z_const Bytef *source, uLong sourceLen);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 2);

    EXPECT_EQ(fns[0].name, "deflateInit");
    ASSERT_EQ(fns[0].params.size(), 2);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "ptr<z_stream>");
    EXPECT_EQ(fns[0].params[1].type.toSPType(), "int32");

    EXPECT_EQ(fns[1].name, "compress");
    ASSERT_EQ(fns[1].params.size(), 4);
    EXPECT_EQ(fns[1].params[0].type.toSPType(), "ptr<uint8>");
    EXPECT_EQ(fns[1].params[1].type.toSPType(), "ptr<uint64>");
    EXPECT_EQ(fns[1].params[2].type.toSPType(), "ptr<uint8>");
    EXPECT_EQ(fns[1].params[3].type.toSPType(), "uint64");
}

// Phase 39 slice C: struct-returning functions were dropped entirely
// (the statement-level `struct` case pre-consumed the return type).
TEST(BindgenTest, StructReturningFunction) {
    const std::string header = R"c(
        struct MiniPoint {
            int x;
            int y;
        };
        struct MiniPoint mini_make(int x, int y);
        struct MiniPoint mini_add_point(struct MiniPoint a, struct MiniPoint b);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 2);
    EXPECT_EQ(fns[0].name, "mini_make");
    EXPECT_EQ(fns[0].returnType.toSPType(), "MiniPoint");
    EXPECT_EQ(fns[1].name, "mini_add_point");
    EXPECT_EQ(fns[1].returnType.toSPType(), "MiniPoint");
    ASSERT_EQ(fns[1].params.size(), 2);
    EXPECT_EQ(fns[1].params[0].type.toSPType(), "MiniPoint");
    EXPECT_EQ(fns[1].params[1].type.toSPType(), "MiniPoint");

    std::string spSource = bg.generateSP();
    EXPECT_NE(spSource.find("fn mini_make(x: int32, y: int32): MiniPoint;"), std::string::npos);
}

// Phase 39 slice C: `typedef struct Tag *Alias;` used to create a
// struct literally named `*`.
TEST(BindgenTest, TypedefStructPointer) {
    const std::string header = R"c(
        typedef struct z_stream_s z_stream;
        typedef z_stream FAR *z_streamp;
        int use_stream(z_streamp s);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    for (const auto& st : bg.getStructs()) {
        EXPECT_NE(st.name, "*");
    }
    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 1);
    ASSERT_EQ(fns[0].params.size(), 1);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "ptr<z_stream>");
}

// Phase 39 slice C: fn-pointer typedefs and fn-pointer struct fields
// emit as ptr<void> (SP has no function-type syntax).
TEST(BindgenTest, FunctionPointerTypedefAndField) {
    const std::string header = R"c(
        typedef void FAR *voidpf;
        typedef unsigned int uInt;
        typedef voidpf (*alloc_func)(voidpf opaque, uInt items, uInt size);
        typedef void (*free_func)(voidpf opaque, voidpf address);
        typedef struct z_stream_s {
            voidpf opaque;
            alloc_func zalloc;
            free_func zfree;
            int level;
        } z_stream;
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& structs = bg.getStructs();
    ASSERT_EQ(structs.size(), 1);
    EXPECT_EQ(structs[0].name, "z_stream");
    ASSERT_EQ(structs[0].fields.size(), 4);
    EXPECT_EQ(structs[0].fields[0].type.toSPType(), "ptr<void>");
    EXPECT_EQ(structs[0].fields[1].name, "zalloc");
    EXPECT_EQ(structs[0].fields[1].type.toSPType(), "ptr<void>");
    EXPECT_EQ(structs[0].fields[2].name, "zfree");
    EXPECT_EQ(structs[0].fields[2].type.toSPType(), "ptr<void>");
    EXPECT_EQ(structs[0].fields[3].type.toSPType(), "int32");

    std::string spSource = bg.generateSP();
    EXPECT_NE(spSource.find("zalloc: ptr<void>"), std::string::npos);
}

// Phase 39 slice C: anonymous struct/union members — unnamed ones
// flatten into the outer struct; named ones become synthetic structs.
TEST(BindgenTest, AnonymousStructFields) {
    const std::string header = R"c(
        struct Outer {
            int a;
            struct {
                int b;
                int c;
            };
            struct {
                int d;
            } inner;
        };
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& structs = bg.getStructs();
    ASSERT_EQ(structs.size(), 2);
    const CStruct* outer = nullptr;
    const CStruct* synth = nullptr;
    for (const auto& st : structs) {
        if (st.name == "Outer") outer = &st;
        if (st.name == "Outer_inner") synth = &st;
    }
    ASSERT_NE(outer, nullptr);
    ASSERT_EQ(outer->fields.size(), 4);
    EXPECT_EQ(outer->fields[0].name, "a");
    EXPECT_EQ(outer->fields[1].name, "b");
    EXPECT_EQ(outer->fields[2].name, "c");
    EXPECT_EQ(outer->fields[3].name, "inner");
    EXPECT_EQ(outer->fields[3].type.toSPType(), "Outer_inner");
    ASSERT_NE(synth, nullptr);
    ASSERT_EQ(synth->fields.size(), 1);
    EXPECT_EQ(synth->fields[0].name, "d");
}

// Phase 39 slice C: #include following — typedefs from an included
// header must resolve at use sites in the including header.
TEST(BindgenTest, IncludeFollowing) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "bindgen_inc_test";
    fs::create_directories(dir);
    {
        std::ofstream base(dir / "inc_base.h");
        base << "typedef unsigned int uInt;\n"
             << "typedef uInt FAR uIntf;\n";
    }
    {
        std::ofstream main(dir / "inc_main.h");
        main << "#include \"inc_base.h\"\n"
             << "int use_it(uIntf x);\n";
    }

    BindgenOptions opts;
    opts.includeDirs = {dir.string()};
    Bindgen bg(opts);
    ASSERT_TRUE(bg.parseFile((dir / "inc_main.h").string()));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 1);
    ASSERT_EQ(fns[0].params.size(), 1);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "uint32");

    fs::remove_all(dir);
}

// Phase 39 slice C: variadic declarations emit `...` and the SP
// parser accepts the generated declaration.
TEST(BindgenTest, VariadicEmission) {
    const std::string header = R"c(
        int printf_like(const char *fmt, ...);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 1);
    EXPECT_TRUE(fns[0].isVariadic);
    ASSERT_EQ(fns[0].params.size(), 1);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "ptr<char>");

    std::string spSource = bg.generateSP();
    EXPECT_NE(spSource.find("fn printf_like(fmt: ptr<char>, ...): int32;"), std::string::npos);

    sp::diagnostics::DiagnosticEngine diagEngine(std::cerr);
    sp::lexer::Lexer lexer(spSource, "test.sp", &diagEngine);
    auto tokens = lexer.tokenizeAll();
    sp::parser::Parser parser(tokens, "test.sp", &diagEngine);
    auto ast = parser.parseProgram();
    EXPECT_FALSE(diagEngine.hasErrors());
    EXPECT_NE(ast, nullptr);
}

// Phase 39 slice C: the `extern "C" {` opener from `#ifdef
// __cplusplus` guards must not swallow the first declaration after
// it (zlib lost `alloc_func` this way), and the closing `}` must not
// swallow anything either.
TEST(BindgenTest, ExternCGuardDoesNotSwallowNextDeclaration) {
    const std::string header = R"c(
        typedef void FAR *voidpf;
        typedef unsigned int uInt;
        #ifdef __cplusplus
        extern "C" {
        #endif
        typedef voidpf (*alloc_func)(voidpf opaque, uInt items, uInt size);
        typedef void (*free_func)(voidpf opaque, voidpf address);
        typedef struct S {
            alloc_func zalloc;
            free_func zfree;
        } S;
        int use_it(alloc_func cb);
        #ifdef __cplusplus
        }
        #endif
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& structs = bg.getStructs();
    ASSERT_EQ(structs.size(), 1);
    ASSERT_EQ(structs[0].fields.size(), 2);
    EXPECT_EQ(structs[0].fields[0].type.toSPType(), "ptr<void>");
    EXPECT_EQ(structs[0].fields[1].type.toSPType(), "ptr<void>");

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 1);
    ASSERT_EQ(fns[0].params.size(), 1);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "ptr<void>");
}

// Phase 39 slice C: #ifdef/#else alternative definitions of one
// macro (zlib's MAX_MEM_LEVEL 8/9) must not emit duplicate consts;
// C's last-#define-wins rule keeps the later value.
TEST(BindgenTest, DuplicateMacroKeepsLastValue) {
    const std::string header = R"c(
        #define MAX_MEM_LEVEL 8
        #define MAX_MEM_LEVEL 9
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& macros = bg.getMacros();
    ASSERT_EQ(macros.size(), 1);
    EXPECT_EQ(macros[0].name, "MAX_MEM_LEVEL");
    EXPECT_EQ(macros[0].value, "9");
}

// Phase 39 slice C: `typedef Byte const *voidpc;` (zconf.h) — the
// pre-scan must not mistake the TYPE for a macro prefix just because
// a qualifier (`const`) follows it; only a real type after the
// candidate macro justifies skipping it.
TEST(BindgenTest, ConstQualifiedPointerTypedef) {
    const std::string header = R"c(
        typedef unsigned char Byte;
        typedef Byte const *voidpc;
        typedef Byte FAR *voidpf;
        int use_it(voidpc buf, voidpf opaque);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 1);
    ASSERT_EQ(fns[0].params.size(), 2);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "ptr<uint8>");
    EXPECT_EQ(fns[0].params[1].type.toSPType(), "ptr<uint8>");
}

// Phase 39 slice C: `#define z_off64_t off64_t` (zconf.h) is a macro
// aliasing a type name, and `va_list` (<stdarg.h>) may not be on the
// include path at all.
TEST(BindgenTest, DefineTypeAliasAndVaList) {
    const std::string header = R"c(
        typedef long off64_t;
        #define z_off64_t off64_t
        int use_off(z_off64_t pos);
        int vprintf_like(const char *fmt, va_list ap);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 2);
    ASSERT_EQ(fns[0].params.size(), 1);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "int64");
    ASSERT_EQ(fns[1].params.size(), 2);
    EXPECT_EQ(fns[1].params[1].type.toSPType(), "ptr<void>");
}

// Phase 39 slice C: a `#define NAME <type>` in a later file must not
// clobber a real typedef from an earlier file (zlib's inactive-#ifdef
// Z_PREFIX renames: Byte -> z_Byte, voidpf -> z_voidpf). Real
// typedefs always win at resolve time.
TEST(BindgenTest, DefineAliasDoesNotClobberRealTypedef) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "bindgen_clobber_test";
    fs::create_directories(dir);
    {
        std::ofstream base(dir / "clobber_base.h");
        base << "typedef void FAR *voidpf;\n"
             << "typedef unsigned char Byte;\n"
             << "typedef Byte FAR Bytef;\n";
    }
    {
        std::ofstream main(dir / "clobber_main.h");
        main << "#include \"clobber_base.h\"\n"
             << "#define voidpf z_voidpf\n"
             << "#define Bytef z_Bytef\n"
             << "int use_it(voidpf p, Bytef b);\n";
    }

    Bindgen bg;
    ASSERT_TRUE(bg.parseFile((dir / "clobber_main.h").string()));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 1);
    ASSERT_EQ(fns[0].params.size(), 2);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "ptr<void>");
    EXPECT_EQ(fns[0].params[1].type.toSPType(), "uint8");

    fs::remove_all(dir);
}

// Phase 39 slice C: a known struct name must not be renamed through
// an inactive-#ifdef `#define` (zlib's `#define gz_header
// z_gz_header` under Z_PREFIX).
TEST(BindgenTest, DefineAliasDoesNotRenameStruct) {
    const std::string header = R"c(
        #define gz_header z_gz_header
        typedef struct gz_header_s {
            int text;
        } gz_header;
        typedef gz_header FAR *gz_headerp;
        int use_it(gz_headerp head);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& structs = bg.getStructs();
    ASSERT_EQ(structs.size(), 1);
    EXPECT_EQ(structs[0].name, "gz_header");

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 1);
    ASSERT_EQ(fns[0].params.size(), 1);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "ptr<gz_header>");
}

// Phase 39 slice C: `ZEXTERN uLong ZEXPORT adler32(...)` — a
// convention macro before a TYPEDEF-named return type. The macro must
// be skipped (it is, before a primitive type); otherwise the parser
// takes the macro as the return type, the typedef as the function
// name, and swallows the whole declaration.
TEST(BindgenTest, MacroBeforeTypedefReturnType) {
    const std::string header = R"c(
        typedef unsigned long uLong;
        ZEXTERN uLong ZEXPORT adler32(uLong adler, const unsigned char *buf, unsigned len);
        ZEXTERN int ZEXPORT plain(int x);
    )c";

    Bindgen bg;
    ASSERT_TRUE(bg.parseHeader(header));

    const auto& fns = bg.getFunctions();
    ASSERT_EQ(fns.size(), 2);
    EXPECT_EQ(fns[0].name, "adler32");
    EXPECT_EQ(fns[0].returnType.toSPType(), "uint64");
    ASSERT_EQ(fns[0].params.size(), 3);
    EXPECT_EQ(fns[0].params[0].type.toSPType(), "uint64");
    EXPECT_EQ(fns[0].params[1].type.toSPType(), "ptr<uint8>");
    EXPECT_EQ(fns[0].params[2].type.toSPType(), "uint32");
    EXPECT_EQ(fns[1].name, "plain");
}
