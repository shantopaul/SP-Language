#include <gtest/gtest.h>
#include "sp/codegen/LinkerDriver.h"
#include "sp/codegen/ObjectEmitter.h"
#include <filesystem>
#include <cstdlib>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

class LinkerDriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Phase 4 slice D: unique temp dir per test process + test name.
        // The old fixed dir was shared by every parallel ctest process;
        // one process's TearDown remove_all deleted the directory while
        // another process was linking/running inside it (same defect as
        // ObjectEmitterTest -- see that fixture). Failed in full -j2 runs,
        // passed alone.
#if defined(_WIN32)
        const unsigned long pid = static_cast<unsigned long>(GetCurrentProcessId());
#else
        const long pid = static_cast<long>(::getpid());
#endif
        std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        m_tempDir = fs::temp_directory_path() /
                    ("sp_linker_driver_test_" + std::to_string(pid) + "_" + testName);
        fs::create_directories(m_tempDir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    fs::path m_tempDir;
};

TEST_F(LinkerDriverTest, LocatesLinker) {
    std::string linker = sp::codegen::LinkerDriver::findLinker();
    EXPECT_FALSE(linker.empty());
}

TEST_F(LinkerDriverTest, LocatesPrecompiledRuntimeLib) {
    std::string rtLib = sp::codegen::LinkerDriver::findPrecompiledRuntimeLib();
    EXPECT_FALSE(rtLib.empty());
    EXPECT_TRUE(fs::exists(rtLib)) << "Runtime library not found at: " << rtLib;
}

TEST_F(LinkerDriverTest, FailsCleanlyWhenNoInputObjects) {
    sp::codegen::LinkerDriver driver;
    bool ok = driver.link({}, (m_tempDir / "out.exe").string());
    EXPECT_FALSE(ok);
    EXPECT_FALSE(driver.getLastError().empty());
}

TEST_F(LinkerDriverTest, LinksObjectFileToExecutableAndRuns) {
    // 1. Emit a native object file returning 42
    std::string ir = R"(
; ModuleID = 'linker_test'
target triple = "x86_64-w64-windows-gnu"

define i32 @main() {
entry:
  ret i32 42
}
)";

    sp::codegen::ObjectEmitter emitter;
    fs::path objPath = m_tempDir / "test_main.o";
    ASSERT_TRUE(emitter.emitObjectFromIR(ir, objPath.string())) << emitter.getLastError();

    // 2. Link object file into standalone executable
    sp::codegen::LinkerDriver driver;
    fs::path exePath = m_tempDir / "test_main.exe";
    bool ok = driver.linkSingle(objPath.string(), exePath.string());
    ASSERT_TRUE(ok) << "Link error: " << driver.getLastError();
    EXPECT_TRUE(fs::exists(exePath));

    // 3. Execute the resulting binary and verify exit code 42
#if defined(_WIN32)
    std::string runCmd = "\"" + exePath.string() + "\"";
    int code = std::system(runCmd.c_str());
#else
    std::string runCmd = exePath.string();
    int code = std::system(runCmd.c_str());
    code = WEXITSTATUS(code);
#endif
    EXPECT_EQ(code, 42);
}
