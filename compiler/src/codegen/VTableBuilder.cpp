#include "sp/codegen/VTableBuilder.h"
#include <algorithm>

namespace sp::codegen {

VTableBuilder::VTableBuilder(const sema::SemanticAnalyzer& sema)
    : m_sema(sema) {}

ir::IRTypePtr VTableBuilder::mapType(const std::string& typeName) const {
    if (typeName.empty() || typeName == "void") return ir::IRType::getVoid();
    if (typeName == "bool") return ir::IRType::getInt1();
    if (typeName == "int8" || typeName == "uint8" || typeName == "char" || typeName == "byte") return ir::IRType::getInt8();
    if (typeName == "int16" || typeName == "uint16") return ir::IRType::getInt16();
    if (typeName == "int32" || typeName == "uint32" || typeName == "int") return ir::IRType::getInt32();
    if (typeName == "int64" || typeName == "uint64" || typeName == "uint") return ir::IRType::getInt64();
    if (typeName == "float") return ir::IRType::getFloat();
    if (typeName == "double") return ir::IRType::getDouble();
    if (typeName == "string") return ir::IRType::getPointer(ir::IRType::getInt8());

    auto userType = m_sema.getTypeContext().findUserType(typeName);
    if (userType && userType->isEnum()) {
        return ir::IRType::getInt32();
    }

    // Objects, classes, interfaces, structs are pointers
    return ir::IRType::getPointer();
}

void VTableBuilder::buildProgramLayouts(const ast::Program& program, ir::IRModule& irModule) {
    // 1. Index all class and struct declarations
    for (const auto& decl : program.getDeclarations()) {
        if (!decl) continue;
        if (decl->getKind() == ast::ASTNodeKind::ClassDecl) {
            const auto& classDecl = static_cast<const ast::ClassDecl&>(*decl);
            m_classDecls[classDecl.getName()] = &classDecl;
        }
    }

    // 2. Build layouts for all classes
    for (const auto& [name, classDecl] : m_classDecls) {
        buildClassLayout(*classDecl, irModule);
    }

    // 3. Build layouts for all structs
    for (const auto& decl : program.getDeclarations()) {
        if (!decl) continue;
        if (decl->getKind() == ast::ASTNodeKind::StructDecl) {
            const auto& structDecl = static_cast<const ast::StructDecl&>(*decl);
            buildStructLayout(structDecl, irModule);
        }
    }
}

void VTableBuilder::buildClassLayout(const ast::ClassDecl& classDecl, ir::IRModule& irModule) {
    const std::string& className = classDecl.getName();
    if (m_layouts.find(className) != m_layouts.end()) {
        return; // Already built
    }

    ClassLayout layout;
    layout.name = className;
    layout.baseClass = classDecl.getBaseClass();
    layout.hasVTable = true;
    layout.vtableName = "@_vtable_" + className;

    // Element types for %class.ClassName = type { ... }
    std::vector<ir::IRTypePtr> elementTypes;
    // Slot 0 is always the vtable pointer: ptr
    elementTypes.push_back(ir::IRType::getPointer());
    uint32_t currentFieldIndex = 1;
    uint32_t totalBytes = 8; // vtable pointer size

    // Inherit fields and vtable from base class
    if (classDecl.hasBaseClass()) {
        const std::string& baseName = classDecl.getBaseClass();
        auto baseIt = m_classDecls.find(baseName);
        if (baseIt != m_classDecls.end()) {
            buildClassLayout(*baseIt->second, irModule);
            const auto& baseLayout = m_layouts[baseName];

            // Copy base fields
            for (const auto& bf : baseLayout.fields) {
                layout.fields.push_back(bf);
                layout.fieldIndexMap[bf.name] = bf.index;
                elementTypes.push_back(bf.irType);
                currentFieldIndex++;
                totalBytes += 8;
            }

            // Copy base vtable slots
            layout.vtableSlots = baseLayout.vtableSlots;
            layout.methodSlotMap = baseLayout.methodSlotMap;
        }
    }

    // Add this class's own fields
    for (const auto& member : classDecl.getMembers()) {
        if (!member) continue;
        if (member->getKind() == ast::ASTNodeKind::VarDeclStmt) {
            const auto& varDecl = static_cast<const ast::VarDeclStmt&>(*member);
            if (varDecl.isStatic()) continue; // Static fields are not in instance layout

            FieldLayoutInfo fieldInfo;
            fieldInfo.name = varDecl.getName();
            fieldInfo.typeName = varDecl.getType();
            fieldInfo.irType = mapType(varDecl.getType());
            fieldInfo.index = currentFieldIndex++;
            fieldInfo.className = className;

            layout.fields.push_back(fieldInfo);
            layout.fieldIndexMap[fieldInfo.name] = fieldInfo.index;
            elementTypes.push_back(fieldInfo.irType);
            totalBytes += 8;
        }
    }

    // Process methods and construct vtable slots
    for (const auto& member : classDecl.getMembers()) {
        if (!member) continue;
        if (member->getKind() == ast::ASTNodeKind::FunctionDecl) {
            const auto& fnDecl = static_cast<const ast::FunctionDecl&>(*member);
            if (fnDecl.isStatic()) continue; // Static methods are not in vtable

            // Constructors are not virtual methods in vtable
            if (fnDecl.getName() == className || fnDecl.getName() == "init") {
                continue;
            }

            std::string methodName = fnDecl.getName();
            std::string mangledName = className + "_" + methodName;

            auto slotIt = layout.methodSlotMap.find(methodName);
            if (slotIt != layout.methodSlotMap.end()) {
                // Method override: update existing slot
                uint32_t slotIdx = slotIt->second;
                layout.vtableSlots[slotIdx].mangledName = mangledName;
                layout.vtableSlots[slotIdx].originatingClass = className;
            } else {
                // New virtual method: append to vtable
                uint32_t newSlotIdx = static_cast<uint32_t>(layout.vtableSlots.size());
                VTableSlotInfo slotInfo;
                slotInfo.methodName = methodName;
                slotInfo.mangledName = mangledName;
                slotInfo.slotIndex = newSlotIdx;
                slotInfo.originatingClass = className;

                layout.vtableSlots.push_back(slotInfo);
                layout.methodSlotMap[methodName] = newSlotIdx;
            }
        }
    }

    layout.objectSize = (totalBytes < 8) ? 8 : totalBytes;
    layout.structType = ir::IRType::getStruct("class." + className, elementTypes);

    // Register struct type in IRModule
    irModule.addStructType("class." + className, elementTypes);

    // Register global vtable in IRModule
    std::string vtableRawVal = "[";
    if (layout.vtableSlots.empty()) {
        vtableRawVal += "ptr null";
    } else {
        for (size_t i = 0; i < layout.vtableSlots.size(); ++i) {
            if (i > 0) vtableRawVal += ", ";
            vtableRawVal += "ptr @" + layout.vtableSlots[i].mangledName;
        }
    }
    vtableRawVal += "]";

    auto vtableArrayType = ir::IRType::getPointer(); // Array of function pointers
    irModule.addGlobalVariable(layout.vtableName, vtableArrayType, true, vtableRawVal);

    m_layouts[className] = std::move(layout);
}

void VTableBuilder::buildStructLayout(const ast::StructDecl& structDecl, ir::IRModule& irModule) {
    const std::string& structName = structDecl.getName();
    if (m_layouts.find(structName) != m_layouts.end()) {
        return;
    }

    ClassLayout layout;
    layout.name = structName;
    layout.hasVTable = false;

    std::vector<ir::IRTypePtr> elementTypes;
    uint32_t currentFieldIndex = 0;
    uint32_t totalBytes = 0;

    for (const auto& fieldPtr : structDecl.getFields()) {
        if (!fieldPtr) continue;
        FieldLayoutInfo fieldInfo;
        fieldInfo.name = fieldPtr->getName();
        fieldInfo.typeName = fieldPtr->getType();
        fieldInfo.irType = mapType(fieldPtr->getType());
        fieldInfo.index = currentFieldIndex++;
        fieldInfo.className = structName;

        layout.fields.push_back(fieldInfo);
        layout.fieldIndexMap[fieldInfo.name] = fieldInfo.index;
        elementTypes.push_back(fieldInfo.irType);
        totalBytes += 8;
    }

    layout.objectSize = totalBytes == 0 ? 8 : totalBytes;
    layout.structType = ir::IRType::getStruct("struct." + structName, elementTypes);

    irModule.addStructType("struct." + structName, elementTypes);
    m_layouts[structName] = std::move(layout);
}

const ClassLayout* VTableBuilder::getClassLayout(const std::string& className) const {
    auto it = m_layouts.find(className);
    if (it != m_layouts.end()) {
        return &it->second;
    }
    return nullptr;
}

int VTableBuilder::getFieldIndex(const std::string& className, const std::string& fieldName) const {
    const auto* layout = getClassLayout(className);
    if (!layout) return -1;
    auto it = layout->fieldIndexMap.find(fieldName);
    if (it != layout->fieldIndexMap.end()) {
        return static_cast<int>(it->second);
    }
    return -1;
}

int VTableBuilder::getMethodVTableSlot(const std::string& className, const std::string& methodName) const {
    const auto* layout = getClassLayout(className);
    if (!layout) return -1;
    auto it = layout->methodSlotMap.find(methodName);
    if (it != layout->methodSlotMap.end()) {
        return static_cast<int>(it->second);
    }
    return -1;
}

} // namespace sp::codegen
