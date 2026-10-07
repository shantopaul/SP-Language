#include <gtest/gtest.h>
#include "sp/codegen/LLVMOptimizer.h"
#include <string>
#include <algorithm>

TEST(LLVMOptimizerTest, OptLevelParsingAndStrings) {
    using namespace sp::codegen;
    EXPECT_EQ(LLVMOptimizer::optLevelToString(OptLevel::O0), "-O0");
    EXPECT_EQ(LLVMOptimizer::optLevelToString(OptLevel::O1), "-O1");
    EXPECT_EQ(LLVMOptimizer::optLevelToString(OptLevel::O2), "-O2");
    EXPECT_EQ(LLVMOptimizer::optLevelToString(OptLevel::O3), "-O3");
    EXPECT_EQ(LLVMOptimizer::optLevelToString(OptLevel::Os), "-Os");
    EXPECT_EQ(LLVMOptimizer::optLevelToString(OptLevel::Oz), "-Oz");

    EXPECT_EQ(LLVMOptimizer::parseOptLevel("0"), OptLevel::O0);
    EXPECT_EQ(LLVMOptimizer::parseOptLevel("-O1"), OptLevel::O1);
    EXPECT_EQ(LLVMOptimizer::parseOptLevel("O2"), OptLevel::O2);
    EXPECT_EQ(LLVMOptimizer::parseOptLevel("3"), OptLevel::O3);
    EXPECT_EQ(LLVMOptimizer::parseOptLevel("s"), OptLevel::Os);
    EXPECT_EQ(LLVMOptimizer::parseOptLevel("-Oz"), OptLevel::Oz);
}

TEST(LLVMOptimizerTest, PipelineDescriptions) {
    using namespace sp::codegen;
    auto o2Passes = LLVMOptimizer::getPassPipeline(OptLevel::O2);
    EXPECT_FALSE(o2Passes.empty());
    EXPECT_NE(std::find(o2Passes.begin(), o2Passes.end(), "mem2reg"), o2Passes.end());
    EXPECT_NE(std::find(o2Passes.begin(), o2Passes.end(), "dce"), o2Passes.end());
    EXPECT_NE(std::find(o2Passes.begin(), o2Passes.end(), "sroa"), o2Passes.end());

    auto o3Passes = LLVMOptimizer::getPassPipeline(OptLevel::O3);
    EXPECT_NE(std::find(o3Passes.begin(), o3Passes.end(), "loop-vectorize"), o3Passes.end());
    EXPECT_NE(std::find(o3Passes.begin(), o3Passes.end(), "slp-vectorizer"), o3Passes.end());
}

TEST(LLVMOptimizerTest, InProcessConstantFoldingPass) {
    using namespace sp::codegen;
    LLVMOptimizer optimizer;

    std::string rawIR = R"(
define i32 @calc() {
entry:
  %x = add i32 10, 20
  %y = mul i32 6, 7
  ret i32 %y
}
)";

    std::string folded = optimizer.runInProcessPasses(rawIR);
    EXPECT_NE(folded.find("const-folded (add 10, 20)"), std::string::npos);
    EXPECT_NE(folded.find("const-folded (mul 6, 7)"), std::string::npos);
    EXPECT_NE(folded.find("42"), std::string::npos);
}

TEST(LLVMOptimizerTest, O0PassThrough) {
    using namespace sp::codegen;
    OptimizerOptions opts;
    opts.level = OptLevel::O0;
    LLVMOptimizer optimizer(opts);

    std::string rawIR = "define i32 @noop() { ret i32 0 }\n";
    std::string outputIR;
    bool ok = optimizer.optimizeIR(rawIR, outputIR);
    EXPECT_TRUE(ok);
    EXPECT_EQ(outputIR, rawIR);
}

TEST(LLVMOptimizerTest, TargetMachineOptimizationFoldsConstants) {
    using namespace sp::codegen;
    OptimizerOptions opts;
    opts.level = OptLevel::O2;
    LLVMOptimizer optimizer(opts);

    std::string rawIR = R"(
define i32 @calc() {
entry:
  %a = add i32 20, 22
  ret i32 %a
}
)";

    std::string outputIR;
    bool ok = optimizer.optimizeIR(rawIR, outputIR);
    EXPECT_TRUE(ok);
    // Under -O2 with LLVM, %a = add i32 20, 22 folds directly into ret i32 42
    EXPECT_NE(outputIR.find("42"), std::string::npos);
}
