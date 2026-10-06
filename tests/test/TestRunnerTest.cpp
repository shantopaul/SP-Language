#include <gtest/gtest.h>
#include "test/TestRunner.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace sp::testing;

class TestRunnerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tempDir = fs::temp_directory_path() / ("sp_testrunner_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
        fs::create_directories(m_tempDir / "tests", ec);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    fs::path m_tempDir;
};

TEST_F(TestRunnerTest, ExtractTestBlocks) {
    fs::path testFile = m_tempDir / "tests" / "math_test.sp";
    std::ofstream out(testFile);
    out << "fn add(a: int, b: int): int { return a + b; }\n\n"
        << "test \"addition works\" {\n"
        << "    assert(add(2, 3) == 5);\n"
        << "}\n\n"
        << "test \"subtraction works\" {\n"
        << "    assert(5 - 2 == 3);\n"
        << "}\n";
    out.close();

    TestRunner runner;
    auto testCases = runner.extractTestsFromFile(testFile.string());
    ASSERT_EQ(testCases.size(), 2);
    EXPECT_EQ(testCases[0].name, "addition works");
    EXPECT_EQ(testCases[0].functionName, "__sp_test_addition_works");
    EXPECT_EQ(testCases[1].name, "subtraction works");
    EXPECT_EQ(testCases[1].functionName, "__sp_test_subtraction_works");
}

TEST_F(TestRunnerTest, RunPassingTestFile) {
    fs::path testFile = m_tempDir / "tests" / "logic_test.sp";
    std::ofstream out(testFile);
    out << "test \"boolean truth\" {\n"
        << "    assert(true);\n"
        << "}\n\n"
        << "test \"arithmetic logic\" {\n"
        << "    let x = 10 * 2;\n"
        << "    assert(x == 20);\n"
        << "}\n";
    out.close();

    TestRunner runner;
    auto suite = runner.runFile(testFile.string());
    EXPECT_EQ(suite.total, 2);
    EXPECT_EQ(suite.passed, 2);
    EXPECT_EQ(suite.failed, 0);
    EXPECT_TRUE(suite.isSuccess());
}

TEST_F(TestRunnerTest, RunFailingTestFile) {
    fs::path testFile = m_tempDir / "tests" / "fail_test.sp";
    std::ofstream out(testFile);
    out << "test \"failing assertion\" {\n"
        << "    assert(1 == 2);\n"
        << "}\n";
    out.close();

    TestRunner runner;
    auto suite = runner.runFile(testFile.string());
    EXPECT_EQ(suite.total, 1);
    EXPECT_EQ(suite.failed, 1);
    EXPECT_FALSE(suite.isSuccess());
}

TEST_F(TestRunnerTest, DiscoverTestFiles) {
    fs::path test1 = m_tempDir / "tests" / "a_test.sp";
    fs::path test2 = m_tempDir / "tests" / "b_test.sp";
    std::ofstream(test1) << "test \"a\" { assert(true); }";
    std::ofstream(test2) << "test \"b\" { assert(true); }";

    TestRunner runner;
    auto files = runner.discoverTestFiles(m_tempDir.string());
    EXPECT_GE(files.size(), 2);
}
