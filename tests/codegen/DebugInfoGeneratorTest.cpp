#include <gtest/gtest.h>
#include "sp/codegen/DebugInfoGenerator.h"
#include "sp/codegen/ObjectEmitter.h"
#include <filesystem>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

class DebugInfoGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Phase 4 slice D: unique temp dir per test process + test name.
        // Same shared-dir defect as ObjectEmitterTest/LinkerDriverTest:
        // the old fixed dir was shared by every parallel ctest process
        // and one process's TearDown remove_all could delete files another
        // process was using. Fixed proactively with the same pattern.
#if defined(_WIN32)
        const unsigned long pid = static_cast<unsigned long>(GetCurrentProcessId());
#else
        const long pid = static_cast<long>(::getpid());
#endif
        std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        m_tempDir = fs::temp_directory_path() /
                    ("sp_dbg_info_test_" + std::to_string(pid) + "_" + testName);
        fs::create_directories(m_tempDir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    fs::path m_tempDir;
};

TEST_F(DebugInfoGeneratorTest, GeneratesModuleFlagsAndCompileUnit) {
    sp::codegen::DebugInfoGenerator dbg("test.sp", "src");
    std::string prolog = dbg.generateMetadataProlog();
    EXPECT_NE(prolog.find("!llvm.dbg.cu = !{!0}"), std::string::npos);
    EXPECT_NE(prolog.find("!llvm.module.flags"), std::string::npos);

    std::string epilog = dbg.generateMetadataEpilog();
    EXPECT_NE(epilog.find("!DICompileUnit"), std::string::npos);
    EXPECT_NE(epilog.find("test.sp"), std::string::npos);
}

TEST_F(DebugInfoGeneratorTest, SubprogramRegistration) {
    sp::codegen::DebugInfoGenerator dbg("user.sp", "models");
    int id = dbg.createSubprogram("calculateTotal", "calculateTotal", 42);
    EXPECT_EQ(id, 0);

    std::string epilog = dbg.generateMetadataEpilog();
    EXPECT_NE(epilog.find("!DISubprogram(name: \"calculateTotal\""), std::string::npos);
    EXPECT_NE(epilog.find("line: 42"), std::string::npos);
}

TEST_F(DebugInfoGeneratorTest, InjectsDebugInfoIntoLLVMIR) {
    sp::codegen::DebugInfoGenerator dbg("main.sp", "app");
    std::string rawIR = R"(
define i32 @main() {
entry:
  %x = add i32 10, 20
  ret i32 %x
}
)";

    std::string debugIR = dbg.injectDebugInfo(rawIR);
    EXPECT_NE(debugIR.find("!llvm.dbg.cu"), std::string::npos);
    EXPECT_NE(debugIR.find("!dbg !10"), std::string::npos);
    EXPECT_NE(debugIR.find("!DILocation"), std::string::npos);
    EXPECT_NE(debugIR.find("!DISubprogram"), std::string::npos);
}

TEST_F(DebugInfoGeneratorTest, DebugIRCompilesCleanlyToObject) {
    sp::codegen::DebugInfoGenerator dbg("calc.sp", "utils");
    std::string rawIR = R"(
target triple = "x86_64-w64-windows-gnu"

define i32 @addNums() {
entry:
  %val = add i32 15, 25
  ret i32 %val
}
)";

    std::string debugIR = dbg.injectDebugInfo(rawIR);

    sp::codegen::ObjectEmitter emitter;
    fs::path objPath = m_tempDir / "calc_debug.o";
    bool ok = emitter.emitObjectFromIR(debugIR, objPath.string());
    ASSERT_TRUE(ok) << "Debug IR compilation error: " << emitter.getLastError();
    EXPECT_TRUE(fs::exists(objPath));
    EXPECT_GT(fs::file_size(objPath), 0u);
}
