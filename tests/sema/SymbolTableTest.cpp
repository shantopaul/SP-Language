#include <gtest/gtest.h>
#include "sp/sema/SymbolTable.h"

using namespace sp::sema;
using namespace sp::diagnostics;

TEST(SymbolTableTest, ScopeNestingAndShadowing) {
    TypeContext ctx;
    SymbolTable symTable;

    SourceLocation loc("test.sp", 1, 1);
    auto varOuter = std::make_shared<VariableSymbol>("x", ctx.getInt(), loc);
    EXPECT_TRUE(symTable.define(varOuter));

    EXPECT_EQ(symTable.lookup("x"), varOuter);

    // Enter block scope and shadow 'x' with string
    symTable.enterScope(ScopeKind::Block);
    auto varInner = std::make_shared<VariableSymbol>("x", ctx.getString(), loc);
    EXPECT_TRUE(symTable.define(varInner));

    EXPECT_EQ(symTable.lookup("x"), varInner);
    EXPECT_EQ(symTable.lookup("x")->getType(), ctx.getString());

    // Exit block scope - restores outer 'x'
    symTable.exitScope();
    EXPECT_EQ(symTable.lookup("x"), varOuter);
    EXPECT_EQ(symTable.lookup("x")->getType(), ctx.getInt());
}

TEST(SymbolTableTest, DuplicateVariableInSameScopeFails) {
    TypeContext ctx;
    SymbolTable symTable;

    SourceLocation loc("test.sp", 1, 1);
    auto var1 = std::make_shared<VariableSymbol>("count", ctx.getInt(), loc);
    auto var2 = std::make_shared<VariableSymbol>("count", ctx.getDouble(), loc);

    EXPECT_TRUE(symTable.define(var1));
    EXPECT_FALSE(symTable.define(var2)); // Duplicate in same scope rejected
}

TEST(SymbolTableTest, FunctionOverloadingSupport) {
    TypeContext ctx;
    SymbolTable symTable;

    SourceLocation loc("test.sp", 1, 1);
    auto fnInt = std::make_shared<FunctionSymbol>(
        "compute",
        ctx.getFunctionType({ctx.getInt()}, ctx.getInt()),
        loc
    );
    auto fnDouble = std::make_shared<FunctionSymbol>(
        "compute",
        ctx.getFunctionType({ctx.getDouble()}, ctx.getDouble()),
        loc
    );
    auto fnDuplicate = std::make_shared<FunctionSymbol>(
        "compute",
        ctx.getFunctionType({ctx.getInt()}, ctx.getString()),
        loc
    );

    // First overload succeeds
    EXPECT_TRUE(symTable.define(fnInt));

    // Second overload with different parameter type succeeds
    EXPECT_TRUE(symTable.define(fnDouble));

    // Duplicate overload with identical parameter type fails
    EXPECT_FALSE(symTable.define(fnDuplicate));

    auto overloads = symTable.lookupFunctions("compute");
    EXPECT_EQ(overloads.size(), 2);
}

TEST(SymbolTableTest, BuiltinFunctionsInitialization) {
    TypeContext ctx;
    SymbolTable symTable;
    symTable.initBuiltins(ctx);

    auto printlnOverloads = symTable.lookupFunctions("println");
    EXPECT_GE(printlnOverloads.size(), 4);

    EXPECT_NE(symTable.lookup("readln"), nullptr);
    EXPECT_NE(symTable.lookup("assert"), nullptr);
    EXPECT_NE(symTable.lookup("panic"), nullptr);
    EXPECT_NE(symTable.lookup("len"), nullptr);
}
