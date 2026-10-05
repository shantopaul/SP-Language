#include "sp/codegen/ClassGenerator.h"
#include "sp/codegen/VTableBuilder.h"
#include "sp/ir/IRBuilder.h"

namespace sp::codegen {

ClassGenerator::ClassGenerator(ir::IRBuilder& builder, ir::IRModule& module, const VTableBuilder& vtableBuilder)
    : m_builder(builder), m_module(module), m_vtableBuilder(vtableBuilder) {}

void ClassGenerator::generateClass(const ast::ClassDecl& classDecl) {
    const auto* layout = m_vtableBuilder.getClassLayout(classDecl.getName());
    if (!layout) return;

    // Check if an explicit constructor is defined
    const ast::FunctionDecl* explicitCtor = nullptr;
    for (const auto& member : classDecl.getMembers()) {
        if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
            if (fn->getName() == classDecl.getName()) {
                explicitCtor = fn;
                break;
            }
        }
    }

    if (explicitCtor) {
        generateConstructor(classDecl, explicitCtor);
    } else {
        generateDefaultConstructor(classDecl);
    }

    // Generate all instance and static methods
    for (const auto& member : classDecl.getMembers()) {
        if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
            if (fn->getName() != classDecl.getName()) {
                generateMethod(classDecl, *fn);
            }
        }
    }
}

void ClassGenerator::generateConstructor(const ast::ClassDecl& classDecl, const ast::FunctionDecl* ctorDecl) {
    const auto* layout = m_vtableBuilder.getClassLayout(classDecl.getName());
    if (!layout) return;

    m_builder.compileConstructor(classDecl, ctorDecl, *layout);
}

void ClassGenerator::generateDefaultConstructor(const ast::ClassDecl& classDecl) {
    const auto* layout = m_vtableBuilder.getClassLayout(classDecl.getName());
    if (!layout) return;

    m_builder.compileDefaultConstructor(classDecl, *layout);
}

void ClassGenerator::generateMethod(const ast::ClassDecl& classDecl, const ast::FunctionDecl& methodDecl) {
    const auto* layout = m_vtableBuilder.getClassLayout(classDecl.getName());
    if (!layout) return;

    m_builder.compileMethod(classDecl, methodDecl, *layout);
}

} // namespace sp::codegen
