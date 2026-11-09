#include <gtest/gtest.h>
#include "sp/codegen/ObjectEmitter.h"
#include "sp/ir/IRModule.h"
#include <filesystem>
#include <fstream>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

class ObjectEmitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Phase 4 slice D: unique temp dir per test process + test name.
        // The old fixed dir /tmp/sp_obj_emitter_test was shared by every
        // parallel ctest process; one process's SetUp/TearDown remove_all
        // deleted the directory while another process's clang was opening
        // its output file inside it:
        //   error: unable to open output file
        //   '/tmp/sp_obj_emitter_test/compute.o': 'No such file or directory'
        // (5 of 6 -j2 runs failed). Same pattern TlsTest already uses.
#if defined(_WIN32)
        const unsigned long pid = static_cast<unsigned long>(GetCurrentProcessId());
#else
        const long pid = static_cast<long>(::getpid());
#endif
        std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        m_tempDir = fs::temp_directory_path() /
                    ("sp_obj_emitter_test_" + std::to_string(pid) + "_" + testName);
        fs::create_directories(m_tempDir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    fs::path m_tempDir;
};

TEST_F(ObjectEmitterTest, DetectsHostTripleAndExtension) {
    std::string triple = sp::codegen::ObjectEmitter::getHostTriple();
    EXPECT_FALSE(triple.empty());

    std::string extGnu = sp::codegen::ObjectEmitter::getObjectFileExtension("x86_64-w64-windows-gnu");
    EXPECT_EQ(extGnu, ".o");

    std::string extMsvc = sp::codegen::ObjectEmitter::getObjectFileExtension("x86_64-pc-windows-msvc");
    EXPECT_EQ(extMsvc, ".obj");
}

TEST_F(ObjectEmitterTest, LocatesLLVMTooling) {
    std::string clang = sp::codegen::ObjectEmitter::findLLVMTool("clang");
    EXPECT_FALSE(clang.empty());
}

TEST_F(ObjectEmitterTest, EmitsNativeObjectFromIRText) {
    sp::codegen::ObjectEmitter emitter;
    std::string ir = R"(
; ModuleID = 'test_module'
target triple = "x86_64-w64-windows-gnu"

define i32 @test_add(i32 %a, i32 %b) {
entry:
  %res = add i32 %a, %b
  ret i32 %res
}
)";

    fs::path objPath = m_tempDir / "test_add.o";
    bool ok = emitter.emitObjectFromIR(ir, objPath.string());
    ASSERT_TRUE(ok) << "Error: " << emitter.getLastError();
    EXPECT_TRUE(fs::exists(objPath));
    EXPECT_GT(fs::file_size(objPath), 0u);
}

TEST_F(ObjectEmitterTest, EmitsNativeObjectDirectlyToMemory) {
    sp::codegen::ObjectEmitter emitter;
    std::string ir = R"(
; ModuleID = 'mem_module'
target triple = "x86_64-w64-windows-gnu"

define i32 @answer() {
entry:
  ret i32 42
}
)";

    std::vector<uint8_t> buffer;
    bool ok = emitter.emitObjectFromIRToMemory(ir, buffer);
    ASSERT_TRUE(ok) << "Error: " << emitter.getLastError();
    EXPECT_GT(buffer.size(), 64u);

    // Verify object file has non-zero bytes
    bool hasNonZero = false;
    for (uint8_t b : buffer) {
        if (b != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(ObjectEmitterTest, EmitsObjectFromIRModule) {
    sp::codegen::ObjectEmitter emitter;
    sp::ir::IRModule module("ir_mod");
    auto fn = module.createFunction("compute", sp::ir::IRType::getInt32(), {});
    auto bb = fn->createBasicBlock("entry");
    bb->addInstruction(std::make_shared<sp::ir::ReturnInst>(
        std::make_shared<sp::ir::ConstantInt>(sp::ir::IRType::getInt32(), 100)));

    fs::path objPath = m_tempDir / "compute.o";
    bool ok = emitter.emitObjectFile(module, objPath.string());
    ASSERT_TRUE(ok) << "Error: " << emitter.getLastError();
    EXPECT_TRUE(fs::exists(objPath));
    EXPECT_GT(fs::file_size(objPath), 0u);
}
