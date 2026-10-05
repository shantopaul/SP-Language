#pragma once

#include "sp/ast/Declaration.h"
#include "sp/ir/IRModule.h"

namespace sp::ir {
class IRBuilder;
}

namespace sp::codegen {

class VTableBuilder;
struct ClassLayout;

class ClassGenerator {
public:
    ClassGenerator(ir::IRBuilder& builder, ir::IRModule& module, const VTableBuilder& vtableBuilder);

    // Generate LLVM IR functions for a class: constructors and methods
    void generateClass(const ast::ClassDecl& classDecl);

    // Generate constructor: @ClassName_ClassName(ptr %this, ...)
    void generateConstructor(const ast::ClassDecl& classDecl, const ast::FunctionDecl* ctorDecl);

    // Generate default constructor if no explicit constructor exists
    void generateDefaultConstructor(const ast::ClassDecl& classDecl);

    // Generate instance or static method: @ClassName_methodName(...)
    void generateMethod(const ast::ClassDecl& classDecl, const ast::FunctionDecl& methodDecl);

private:
    ir::IRBuilder& m_builder;
    [[maybe_unused]] ir::IRModule& m_module;
    const VTableBuilder& m_vtableBuilder;
};

} // namespace sp::codegen
