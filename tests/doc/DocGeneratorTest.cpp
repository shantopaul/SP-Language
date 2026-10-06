#include <gtest/gtest.h>
#include "doc/DocGenerator.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;
using namespace sp::doc;

class DocGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tempDir = fs::temp_directory_path() / ("sp_doc_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
        fs::create_directories(m_tempDir, ec);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    fs::path m_tempDir;
};

TEST_F(DocGeneratorTest, CleanDocCommentAndExtractBrief) {
    std::string tripleSlash = "/// Calculates the factorial of n.\n///\n/// Uses recursion for demonstration.\n/// @param n An integer >= 0\n/// @return The factorial of n";
    std::string cleaned = DocGenerator::cleanDocComment(tripleSlash);
    EXPECT_NE(cleaned.find("Calculates the factorial of n."), std::string::npos);
    EXPECT_NE(cleaned.find("Uses recursion for demonstration."), std::string::npos);

    std::string brief = DocGenerator::extractBrief(cleaned);
    EXPECT_EQ(brief, "Calculates the factorial of n.");

    std::string blockComment = "/**\n * Represents a 2D geometric vector.\n * High precision.\n */";
    std::string cleanedBlock = DocGenerator::cleanDocComment(blockComment);
    EXPECT_NE(cleanedBlock.find("Represents a 2D geometric vector."), std::string::npos);
    EXPECT_EQ(DocGenerator::extractBrief(cleanedBlock), "Represents a 2D geometric vector.");
}

TEST_F(DocGeneratorTest, ExtractFunctionDocumentation) {
    std::string source = 
        "/// Computes power of a base.\n"
        "/// @param base The base number\n"
        "/// @param exp The exponent\n"
        "/// @return The computed result\n"
        "fn power(base: int, exp: int): int {\n"
        "    return 1;\n"
        "}\n";

    DocGenerator gen;
    DocModule mod = gen.extractModule(source, "math.sp");

    EXPECT_EQ(mod.name, "math");
    ASSERT_EQ(mod.entries.size(), 1);

    const auto& fn = mod.entries[0];
    EXPECT_EQ(fn.name, "power");
    EXPECT_EQ(fn.kind, DocEntryKind::Function);
    EXPECT_EQ(fn.brief, "Computes power of a base.");
    EXPECT_EQ(fn.returnType, "int");
    EXPECT_EQ(fn.returnDescription, "The computed result");

    ASSERT_EQ(fn.params.size(), 2);
    EXPECT_EQ(fn.params[0].name, "base");
    EXPECT_EQ(fn.params[0].type, "int");
    EXPECT_EQ(fn.params[0].description, "The base number");

    EXPECT_EQ(fn.params[1].name, "exp");
    EXPECT_EQ(fn.params[1].type, "int");
    EXPECT_EQ(fn.params[1].description, "The exponent");
}

TEST_F(DocGeneratorTest, ExtractClassAndMembersDocumentation) {
    std::string source =
        "/// A 2D point representation.\n"
        "class Point {\n"
        "    /// The X coordinate.\n"
        "    let x: float;\n"
        "\n"
        "    /// Computes distance to another point.\n"
        "    /// @param other Target point\n"
        "    /// @return Euclidean distance\n"
        "    fn distance(other: Point): float {\n"
        "        return 0.0;\n"
        "    }\n"
        "}\n";

    DocGenerator gen;
    DocModule mod = gen.extractModule(source, "geometry.sp");

    ASSERT_EQ(mod.entries.size(), 1);
    const auto& cls = mod.entries[0];
    EXPECT_EQ(cls.name, "Point");
    EXPECT_EQ(cls.kind, DocEntryKind::Class);
    EXPECT_EQ(cls.brief, "A 2D point representation.");

    ASSERT_EQ(cls.members.size(), 2);
    EXPECT_EQ(cls.members[0].name, "x");
    EXPECT_EQ(cls.members[0].kind, DocEntryKind::Field);
    EXPECT_EQ(cls.members[0].brief, "The X coordinate.");

    EXPECT_EQ(cls.members[1].name, "distance");
    EXPECT_EQ(cls.members[1].kind, DocEntryKind::Method);
    EXPECT_EQ(cls.members[1].brief, "Computes distance to another point.");
}

TEST_F(DocGeneratorTest, ExtractInterfaceAndStructDocumentation) {
    std::string source =
        "/// Printable interface.\n"
        "interface Printable {\n"
        "    /// Formats object as string.\n"
        "    fn toString(): string;\n"
        "}\n\n"
        "/// Color struct.\n"
        "struct Color {\n"
        "    r: int;\n"
        "    g: int;\n"
        "    b: int;\n"
        "}\n";

    DocGenerator gen;
    DocModule mod = gen.extractModule(source, "types.sp");

    ASSERT_EQ(mod.entries.size(), 2);
    EXPECT_EQ(mod.entries[0].name, "Printable");
    EXPECT_EQ(mod.entries[0].kind, DocEntryKind::Interface);
    EXPECT_EQ(mod.entries[0].brief, "Printable interface.");
    ASSERT_EQ(mod.entries[0].members.size(), 1);
    EXPECT_EQ(mod.entries[0].members[0].name, "toString");

    EXPECT_EQ(mod.entries[1].name, "Color");
    EXPECT_EQ(mod.entries[1].kind, DocEntryKind::Struct);
    EXPECT_EQ(mod.entries[1].brief, "Color struct.");
    EXPECT_EQ(mod.entries[1].members.size(), 3);
}

TEST_F(DocGeneratorTest, RenderMarkdownAndHtml) {
    std::string source =
        "/// Core utilities module.\n"
        "/// Provides helper tools.\n"
        "fn helper(val: int): bool {\n"
        "    return val > 0;\n"
        "}\n";

    DocGenerator gen;
    DocModule mod = gen.extractModule(source, "utils.sp");

    std::string md = gen.renderMarkdown(mod);
    EXPECT_NE(md.find("# Module `utils`"), std::string::npos);
    EXPECT_NE(md.find("## Functions"), std::string::npos);
    EXPECT_NE(md.find("helper"), std::string::npos);

    std::string html = gen.renderHtml(mod);
    EXPECT_NE(html.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(html.find("Module <code>utils</code>"), std::string::npos);
    EXPECT_NE(html.find("helper"), std::string::npos);
}

TEST_F(DocGeneratorTest, GenerateToDisk) {
    fs::path srcFile = m_tempDir / "sample.sp";
    std::ofstream out(srcFile);
    out << "/// High performance math functions\n"
        << "fn add(a: int, b: int): int {\n"
        << "    return a + b;\n"
        << "}\n";
    out.close();

    fs::path outDir = m_tempDir / "docs";

    DocOptions opts;
    opts.outputDir = outDir.string();
    opts.projectName = "Test Project";
    DocGenerator gen(opts);

    bool ok = gen.generate(srcFile.string(), outDir.string());
    EXPECT_TRUE(ok);

    EXPECT_TRUE(fs::exists(outDir / "sample.md"));
    EXPECT_TRUE(fs::exists(outDir / "sample.html"));
    EXPECT_TRUE(fs::exists(outDir / "index.html"));
    EXPECT_TRUE(fs::exists(outDir / "index.md"));

    EXPECT_GT(fs::file_size(outDir / "sample.html"), 100);
    EXPECT_GT(fs::file_size(outDir / "sample.md"), 50);
}
