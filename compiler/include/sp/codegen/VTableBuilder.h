#pragma once

#include "sp/ast/Declaration.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRModule.h"
#include "sp/ir/IRType.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace sp::codegen {

struct FieldLayoutInfo {
    std::string name;
    std::string typeName;
    ir::IRTypePtr irType;
    uint32_t index;       // Struct element index
    std::string className;
};

struct VTableSlotInfo {
    std::string methodName;
    std::string mangledName; // e.g. "Animal_speak" or "Dog_speak"
    uint32_t slotIndex;
    ir::IRTypePtr functionType;
    std::string originatingClass;
};

struct ClassLayout {
    std::string name;
    std::string baseClass;
    bool hasVTable = true;
    uint32_t objectSize = 0; // Size in bytes
    ir::IRStructTypePtr structType;
    std::vector<FieldLayoutInfo> fields;
    std::unordered_map<std::string, uint32_t> fieldIndexMap;
    std::vector<VTableSlotInfo> vtableSlots;
    std::unordered_map<std::string, uint32_t> methodSlotMap;
    std::string vtableName; // e.g. "@_vtable_Animal"
};

class VTableBuilder {
public:
    explicit VTableBuilder(const sema::SemanticAnalyzer& sema);

    // Build layouts for all classes and structs in the AST
    void buildProgramLayouts(const ast::Program& program, ir::IRModule& irModule);

    // Query layouts
    [[nodiscard]] const ClassLayout* getClassLayout(const std::string& className) const;
    [[nodiscard]] int getFieldIndex(const std::string& className, const std::string& fieldName) const;
    [[nodiscard]] int getMethodVTableSlot(const std::string& className, const std::string& methodName) const;

    // Helper to map type names to IR types
    ir::IRTypePtr mapType(const std::string& typeName) const;
    [[nodiscard]] const sema::SemanticAnalyzer& getSema() const noexcept { return m_sema; }

private:
    void buildClassLayout(const ast::ClassDecl& classDecl, ir::IRModule& irModule);
    void buildStructLayout(const ast::StructDecl& structDecl, ir::IRModule& irModule);

    const sema::SemanticAnalyzer& m_sema;
    std::unordered_map<std::string, ClassLayout> m_layouts;
    std::unordered_map<std::string, const ast::ClassDecl*> m_classDecls;
};

} // namespace sp::codegen
