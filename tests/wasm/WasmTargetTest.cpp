#include <gtest/gtest.h>
#include "sp/codegen/ObjectEmitter.h"
#include "sp/codegen/LLVMGenerator.h"
#include "sp/ir/IRModule.h"

using namespace sp::codegen;

TEST(WasmTargetTest, ObjectFileExtensionForWasm) {
    EXPECT_EQ(ObjectEmitter::getObjectFileExtension("wasm32-wasi"), ".wasm");
    EXPECT_EQ(ObjectEmitter::getObjectFileExtension("wasm32-unknown-wasi"), ".wasm");
    EXPECT_EQ(ObjectEmitter::getObjectFileExtension("wasm32-unknown-unknown"), ".wasm");
}

TEST(WasmTargetTest, NativeExtensionsDifferFromWasm) {
    std::string wasmExt = ObjectEmitter::getObjectFileExtension("wasm32-wasi");
    std::string hostTriple = ObjectEmitter::getHostTriple();
    std::string hostExt = ObjectEmitter::getObjectFileExtension(hostTriple);

    EXPECT_EQ(wasmExt, ".wasm");
    EXPECT_NE(hostExt, ".wasm");
}

TEST(WasmTargetTest, WasmDataLayoutAndTripleInLLVMGenerator) {
    sp::ir::IRModule module("wasm_test_module");
    CodegenOptions opts;
    opts.targetTriple = "wasm32-unknown-wasi";

    LLVMGenerator gen(opts);
    std::string llvmIR = gen.generateLLVMIR(module);

    EXPECT_NE(llvmIR.find("target triple = \"wasm32-unknown-wasi\""), std::string::npos);
    EXPECT_NE(llvmIR.find("target datalayout = \"e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20\""), std::string::npos);
}
