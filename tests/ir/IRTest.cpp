#include <gtest/gtest.h>
#include "sp/ir/IRType.h"
#include "sp/ir/IRValue.h"
#include "sp/ir/IRInstruction.h"
#include "sp/ir/IRBasicBlock.h"
#include "sp/ir/IRFunction.h"
#include "sp/ir/IRModule.h"
#include "sp/ir/IRPrinter.h"

using namespace sp::ir;

TEST(IRTypeTest, BasicTypesAndToString) {
    auto voidTy = IRType::getVoid();
    auto i1Ty = IRType::getInt1();
    auto i8Ty = IRType::getInt8();
    auto i32Ty = IRType::getInt32();
    auto i64Ty = IRType::getInt64();
    auto floatTy = IRType::getFloat();
    auto doubleTy = IRType::getDouble();
    auto ptrTy = IRType::getPointer(i32Ty);
    auto funcTy = IRType::getFunction(i32Ty, {i32Ty, i32Ty});

    EXPECT_TRUE(voidTy->isVoid());
    EXPECT_TRUE(i1Ty->isInt1());
    EXPECT_TRUE(i32Ty->isInteger());
    EXPECT_TRUE(doubleTy->isFloatingPoint());
    EXPECT_TRUE(ptrTy->isPointer());
    EXPECT_TRUE(funcTy->isFunction());

    EXPECT_EQ(voidTy->toString(), "void");
    EXPECT_EQ(i1Ty->toString(), "i1");
    EXPECT_EQ(i32Ty->toString(), "i32");
    EXPECT_EQ(i64Ty->toString(), "i64");
    EXPECT_EQ(floatTy->toString(), "float");
    EXPECT_EQ(doubleTy->toString(), "double");
    EXPECT_EQ(ptrTy->toString(), "ptr");
    EXPECT_EQ(funcTy->toString(), "i32(i32, i32)");
}

TEST(IRValueTest, ConstantsAndParameters) {
    auto cInt = std::make_shared<ConstantInt>(IRType::getInt32(), 42);
    auto cFloat = std::make_shared<ConstantFloat>(IRType::getDouble(), 3.14);
    auto cBool = std::make_shared<ConstantBool>(true);
    auto cStr = std::make_shared<ConstantString>("hello", "@.str.0");
    auto cNull = std::make_shared<ConstantNull>();
    auto param = std::make_shared<ParameterValue>(IRType::getInt64(), "%x", 0);

    EXPECT_EQ(cInt->getValue(), 42);
    EXPECT_EQ(cInt->toString(), "42");
    EXPECT_EQ(cFloat->getValue(), 3.14);
    EXPECT_EQ(cBool->getValue(), true);
    EXPECT_EQ(cBool->toString(), "true");
    EXPECT_EQ(cStr->toString(), "@.str.0");
    EXPECT_EQ(cNull->toString(), "null");
    EXPECT_EQ(param->getName(), "%x");
}

TEST(IRInstructionTest, MemoryArithmeticAndBranchInstructions) {
    auto i32 = IRType::getInt32();
    auto allocaInst = std::make_shared<AllocaInst>(i32, "%x");
    EXPECT_EQ(allocaInst->format(), "%x = alloca i32");

    auto c10 = std::make_shared<ConstantInt>(i32, 10);
    auto storeInst = std::make_shared<StoreInst>(c10, allocaInst);
    EXPECT_EQ(storeInst->format(), "store i32 10, ptr %x");

    auto loadInst = std::make_shared<LoadInst>(i32, allocaInst, "%val");
    EXPECT_EQ(loadInst->format(), "%val = load i32, ptr %x");

    auto addInst = std::make_shared<BinaryInst>(IROpcode::Add, loadInst, c10, "%sum");
    EXPECT_EQ(addInst->format(), "%sum = add i32 %val, 10");

    auto cmpInst = std::make_shared<CompareInst>(IROpcode::ICmpSgt, addInst, c10, "%cmp");
    EXPECT_EQ(cmpInst->format(), "%cmp = icmp sgt i32 %sum, 10");

    auto condBr = std::make_shared<CondBranchInst>(cmpInst, "then_0", "else_0");
    EXPECT_EQ(condBr->format(), "br i1 %cmp, label %then_0, label %else_0");

    auto br = std::make_shared<BranchInst>("merge_0");
    EXPECT_EQ(br->format(), "br label %merge_0");

    auto ret = std::make_shared<ReturnInst>(addInst);
    EXPECT_EQ(ret->format(), "ret i32 %sum");
}

TEST(IRModuleTest, BuildAndPrintModule) {
    IRModule module("test_mod");

    // Runtime functions should be auto-declared
    EXPECT_NE(module.getFunction("sp_rt_println_string"), nullptr);
    EXPECT_NE(module.getFunction("sp_rt_println_int"), nullptr);

    // Build main function
    auto mainFunc = module.createFunction("main", IRType::getInt32(), {});
    auto entryBB = mainFunc->createBasicBlock("entry");

    auto helloStr = module.getOrCreateStringLiteral("Hello, SP!");
    auto printCall = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string",
                                                std::vector<IRValuePtr>{helloStr});
    entryBB->addInstruction(printCall);

    auto retVal = std::make_shared<ConstantInt>(IRType::getInt32(), 0);
    entryBB->addInstruction(std::make_shared<ReturnInst>(retVal));

    IRPrinter printer;
    std::string output = printer.print(module);

    EXPECT_NE(output.find("; ModuleID = 'test_mod'"), std::string::npos);
    EXPECT_NE(output.find("@.str.0 = private unnamed_addr constant [11 x i8] c\"Hello, SP!\\00\", align 1"), std::string::npos);
    EXPECT_NE(output.find("declare void @sp_rt_println_string(ptr)"), std::string::npos);
    EXPECT_NE(output.find("define i32 @main() {"), std::string::npos);
    EXPECT_NE(output.find("call void @sp_rt_println_string(ptr @.str.0)"), std::string::npos);
    EXPECT_NE(output.find("ret i32 0"), std::string::npos);
}
