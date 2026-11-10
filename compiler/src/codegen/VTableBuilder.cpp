#include "sp/codegen/VTableBuilder.h"
#include <algorithm>
#include <unordered_set>

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
    if (typeName == "ptr" || typeName.rfind("ptr<", 0) == 0) return ir::IRType::getPointer();

    if (typeName == "Result" || typeName.rfind("Result<", 0) == 0) {
        return ir::IRType::getStruct("struct.Result", {ir::IRType::getInt32(), ir::IRType::getInt64()});
    }
    if (typeName == "Option" || typeName.rfind("Option<", 0) == 0) {
        return ir::IRType::getStruct("struct.Option", {ir::IRType::getInt32(), ir::IRType::getInt64()});
    }

    auto userType = m_sema.getTypeContext().findUserType(typeName);
    if (userType && userType->isEnum()) {
        auto en = std::static_pointer_cast<sema::EnumType>(userType);
        if (en && en->isTaggedUnion()) {
            std::string name = en->getName();
            if (name.rfind("Result", 0) == 0) name = "Result";
            else if (name.rfind("Option", 0) == 0) name = "Option";
            return ir::IRType::getStruct("struct." + name, {ir::IRType::getInt32(), ir::IRType::getInt64()});
        }
        return ir::IRType::getInt32();
    }

    // Objects, classes, interfaces, structs are pointers
    return ir::IRType::getPointer();
}

std::string VTableBuilder::mangleGenericName(const std::string& name) {
    std::string result;
    for (char c : name) {
        if (c == '<' || c == '>' || c == ',' || c == ' ') {
            if (c != '>' && c != ' ') result += '_';
        } else if (c == '[') {
            // Phase 5 slice A: array types (e.g. Task<int[]>) must
            // mangle to a valid LLVM identifier, and must NOT collide
            // with the element type (Task<int[]> vs Task<int>). `[`
            // becomes `_arr`, `]` is dropped, so `int[]` -> `int_arr`.
            result += "_arr";
        } else if (c == ']') {
            // dropped (the `_arr` already marks the array)
        } else if (c == '?') {
            // Nullable marker (e.g. `string?` -> `string_q`).
            result += "_q";
        } else {
            result += c;
        }
    }
    while (!result.empty() && result.back() == '_') {
        result.pop_back();
    }
    return result;
}

std::string VTableBuilder::sanitizeMangledType(const std::string& typeName) {
    std::string result;
    result.reserve(typeName.size());
    for (char c : typeName) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
            result += c;
        } else {
            result += '_';
        }
    }
    return result;
}

std::string VTableBuilder::overloadSuffix(const std::vector<std::string>& canonicalParamTypes) {
    if (canonicalParamTypes.empty()) {
        return "__void";
    }
    std::string suffix = "__";
    for (size_t i = 0; i < canonicalParamTypes.size(); ++i) {
        if (i > 0) suffix += '_';
        suffix += sanitizeMangledType(canonicalParamTypes[i]);
    }
    return suffix;
}

std::string VTableBuilder::mangleOverloaded(const std::string& baseName,
                                           const std::vector<std::string>& canonicalParamTypes,
                                           bool isOverloaded) {
    if (!isOverloaded) return baseName;
    return baseName + overloadSuffix(canonicalParamTypes);
}

// Match a decl to its MemberMethod: same name + arity; prefer the
// candidate whose canonical param types spell the same as the decl's
// raw param strings. (Aliases such as `int32` vs `int` make a raw
// string comparison unreliable on its own.)
static const sema::MemberMethod* findBestMemberMethod(const sema::ClassType& clsType,
                                                      const ast::FunctionDecl& decl) {
    const sema::MemberMethod* best = nullptr;
    int bestScore = -1;
    for (const auto& mm : clsType.getMethods()) {
        if (mm.name != decl.getName()) continue;
        if (!mm.functionType) continue;
        const auto& params = mm.functionType->getParamTypes();
        if (params.size() != decl.getParams().size()) continue;
        int score = 0;
        for (size_t i = 0; i < params.size(); ++i) {
            if (params[i] && params[i]->toString() == decl.getParams()[i].getType()) ++score;
        }
        if (score > bestScore) {
            bestScore = score;
            best = &mm;
        }
    }
    return best;
}

std::vector<std::string> VTableBuilder::canonicalParamTypes(const sema::ClassType& clsType,
                                                           const ast::FunctionDecl& decl) {
    std::vector<std::string> out;
    if (const auto* best = findBestMemberMethod(clsType, decl)) {
        for (const auto& p : best->functionType->getParamTypes()) {
            out.push_back(p ? p->toString() : "any");
        }
        return out;
    }
    // Fallback: raw strings (decl not found in the class table).
    for (const auto& p : decl.getParams()) out.push_back(p.getType());
    return out;
}

bool VTableBuilder::isMethodOverloaded(const sema::ClassType& clsType, const std::string& methodName) {
    // findMethods dedupes overrides (same signature), so size > 1 means
    // more than one distinct signature is visible with this name.
    return clsType.findMethods(methodName).size() > 1;
}

// --- Phase 6 slice A: global interface-slot scheme -----------------------
//
// The IR type of a sema type name, with a trailing '?' stripped: the
// IR type of T? is the IR type of T (a nullable value type is its zero
// value at the IR level). Without the strip, `int?` would map to ptr
// and a vtable call returning int? would be declared `call ptr`.
static ir::IRTypePtr mapTypeUnwrappingNullable(const VTableBuilder& vb, const std::string& typeName) {
    std::string name = typeName;
    while (!name.empty() && name.back() == '?') name.pop_back();
    return vb.mapType(name);
}

// (ptr %this, params...) -> return, from a sema method signature.
// Only the return type is consumed by the vtable dispatch (see
// IRBuilder); the params are recorded for completeness.
static ir::IRTypePtr methodFunctionType(const VTableBuilder& vb, const sema::FunctionType& fnT) {
    std::vector<ir::IRTypePtr> params;
    params.push_back(ir::IRType::getPointer()); // %this
    for (const auto& pt : fnT.getParamTypes()) {
        params.push_back(mapTypeUnwrappingNullable(vb, pt ? pt->toString() : "any"));
    }
    auto ret = fnT.getReturnType();
    return ir::IRType::getFunction(
        mapTypeUnwrappingNullable(vb, ret ? ret->toString() : "void"), params);
}

// (declaring interface name, method) pairs for every method an
// interface exposes — its own methods plus, recursively, its
// super-interfaces' (super-interfaces have no parser syntax today,
// so the closure is usually just the interface itself, but the slot
// scheme stays correct if that changes).
static void collectExposedMethods(
    const std::shared_ptr<sema::InterfaceType>& iface,
    std::vector<std::pair<std::string, sema::MemberMethod>>& out,
    std::unordered_set<std::string>& visited) {
    if (!iface || !visited.insert(iface->getName()).second) return;
    for (const auto& super : iface->getSuperInterfaces()) {
        collectExposedMethods(super, out, visited);
    }
    for (const auto& mm : iface->getMethods()) {
        out.emplace_back(iface->getName(), mm);
    }
}

void VTableBuilder::buildInterfaceSlots() {
    if (m_ifaceSlotsBuilt) return;
    m_ifaceSlotsBuilt = true;

    // Collect every interface, sorted by name: the slot order must be
    // deterministic (getUserTypes() iterates an unordered_map), or
    // vtable layouts would differ between builds.
    std::vector<std::shared_ptr<sema::InterfaceType>> ifaces;
    for (const auto& [name, userType] : m_sema.getTypeContext().getUserTypes()) {
        if (userType && userType->isInterface()) {
            ifaces.push_back(std::static_pointer_cast<sema::InterfaceType>(userType));
        }
    }
    std::sort(ifaces.begin(), ifaces.end(),
              [](const auto& a, const auto& b) { return a->getName() < b->getName(); });

    // One slot per (declaring interface, method, signature). A method
    // is registered under EVERY interface that exposes it (same slot
    // index), so a call through any of those interface types finds
    // the same slot.
    std::unordered_map<std::string, uint32_t> slotByDeclaringKey;
    for (const auto& iface : ifaces) {
        std::vector<std::pair<std::string, sema::MemberMethod>> exposed;
        std::unordered_set<std::string> visited;
        collectExposedMethods(iface, exposed, visited);
        for (const auto& [declaringIface, mm] : exposed) {
            if (!mm.functionType) continue;
            std::vector<std::string> paramTypes;
            for (const auto& pt : mm.functionType->getParamTypes()) {
                paramTypes.push_back(pt ? pt->toString() : "any");
            }
            std::string methodKey = mm.name + overloadSuffix(paramTypes);
            std::string declaringKey = declaringIface + "::" + methodKey;
            uint32_t slotIdx;
            auto it = slotByDeclaringKey.find(declaringKey);
            if (it != slotByDeclaringKey.end()) {
                slotIdx = it->second;
            } else {
                slotIdx = static_cast<uint32_t>(m_ifaceSlots.size());
                slotByDeclaringKey[declaringKey] = slotIdx;
                InterfaceSlotInfo info;
                info.key = declaringKey;
                info.interfaceName = declaringIface;
                info.methodName = mm.name;
                info.functionType = methodFunctionType(*this, *mm.functionType);
                m_ifaceSlots.push_back(std::move(info));
            }
            m_ifaceSlotIndex[iface->getName() + "::" + methodKey] = slotIdx;
        }
    }
}

void VTableBuilder::buildInterfaceLayout(const std::shared_ptr<sema::InterfaceType>& iface, ir::IRModule& irModule) {
    const std::string& ifaceName = iface->getName();
    if (m_layouts.find(ifaceName) != m_layouts.end()) return;

    // An interface has no fields and no instances of its own: the
    // object layout is just { vtable ptr }. No vtable global is
    // emitted — no object is ever created with an interface vtable;
    // the layout exists only so interface-typed calls can use the
    // same vtable-dispatch path as class-typed calls (slot 0 load
    // + slot[index] load), with indices taken from the global table.
    ClassLayout layout;
    layout.name = ifaceName;
    layout.hasVTable = true;
    layout.vtableName = "@_vtable_" + ifaceName;
    layout.objectSize = 8;

    std::vector<ir::IRTypePtr> elementTypes{ir::IRType::getPointer()};
    layout.structType = ir::IRType::getStruct("class." + ifaceName, elementTypes);
    irModule.addStructType("class." + ifaceName, elementTypes);

    layout.vtableSlots.resize(m_ifaceSlots.size());
    for (size_t i = 0; i < m_ifaceSlots.size(); ++i) {
        VTableSlotInfo slot;
        slot.methodName = m_ifaceSlots[i].methodName;
        slot.mangledName = "";
        slot.slotIndex = static_cast<uint32_t>(i);
        slot.functionType = m_ifaceSlots[i].functionType;
        slot.originatingClass = m_ifaceSlots[i].interfaceName;
        layout.vtableSlots[i] = slot;
    }
    for (const auto& [key, idx] : m_ifaceSlotIndex) {
        layout.methodSlotMap[key] = idx;
    }

    m_layouts[ifaceName] = std::move(layout);
}

std::vector<uint32_t> VTableBuilder::interfaceSlotsForMethod(
    const std::string& methodKey, const std::vector<std::string>& implementedInterfaces) const {
    std::vector<uint32_t> out;
    if (implementedInterfaces.empty() || m_ifaceSlots.empty()) return out;
    for (const auto& ifaceName : implementedInterfaces) {
        auto it = m_ifaceSlotIndex.find(ifaceName + "::" + methodKey);
        if (it != m_ifaceSlotIndex.end()) out.push_back(it->second);
    }
    return out;
}

void VTableBuilder::buildProgramLayouts(const ast::Program& program, ir::IRModule& irModule) {
    // 0. Global interface-slot table first: every class layout starts
    // with this region, and every interface gets a layout over it.
    buildInterfaceSlots();
    for (const auto& [typeName, userType] : m_sema.getTypeContext().getUserTypes()) {
        if (userType && userType->isInterface()) {
            buildInterfaceLayout(std::static_pointer_cast<sema::InterfaceType>(userType), irModule);
        }
    }

    // 1. Index all class and struct declarations
    for (const auto& decl : program.getDeclarations()) {
        if (!decl) continue;
        if (decl->getKind() == ast::ASTNodeKind::ClassDecl) {
            const auto& classDecl = static_cast<const ast::ClassDecl&>(*decl);
            m_classDecls[classDecl.getName()] = &classDecl;
        }
    }

    // 2. Build layouts for all non-generic classes
    for (const auto& [name, classDecl] : m_classDecls) {
        buildClassLayout(*classDecl, irModule);
    }

    // 2b. Build layouts for all specialized generic classes
    for (const auto& [typeName, userType] : m_sema.getTypeContext().getUserTypes()) {
        if (userType && userType->isClass()) {
            auto clsType = std::static_pointer_cast<sema::ClassType>(userType);
            if (clsType->getName().find('<') != std::string::npos) {
                buildSpecializedClassLayout(clsType, irModule);
            }
        }
    }

    // 3. Build layouts for all structs
    for (const auto& decl : program.getDeclarations()) {
        if (!decl) continue;
        if (decl->getKind() == ast::ASTNodeKind::StructDecl) {
            const auto& structDecl = static_cast<const ast::StructDecl&>(*decl);
            buildStructLayout(structDecl, irModule);
        }
    }

    // 3b. Build layouts for tagged union enums (Result, Option, custom tagged unions)
    auto buildTaggedUnionLayout = [&](const std::string& name) {
        if (name.empty()) return;
        if (name.rfind("Result", 0) == 0 && name != "Result") {
            if (m_layouts.count("Result")) {
                m_layouts[name] = m_layouts["Result"];
            }
            return;
        }
        if (name.rfind("Option", 0) == 0 && name != "Option") {
            if (m_layouts.count("Option")) {
                m_layouts[name] = m_layouts["Option"];
            }
            return;
        }
        if (m_layouts.find(name) != m_layouts.end()) return;

        std::string validName = mangleGenericName(name);
        ClassLayout layout;
        layout.name = name;
        layout.hasVTable = false;
        FieldLayoutInfo f0{.name = "tag", .typeName = "int32", .irType = ir::IRType::getInt32(), .index = 0, .className = name};
        FieldLayoutInfo f1{.name = "payload", .typeName = "int64", .irType = ir::IRType::getInt64(), .index = 1, .className = name};
        layout.fields.push_back(f0);
        layout.fields.push_back(f1);
        layout.fieldIndexMap["tag"] = 0;
        layout.fieldIndexMap["payload"] = 1;
        layout.objectSize = 16;
        layout.structType = ir::IRType::getStruct("struct." + validName, {ir::IRType::getInt32(), ir::IRType::getInt64()});
        irModule.addStructType("struct." + validName, {ir::IRType::getInt32(), ir::IRType::getInt64()});
        m_layouts[name] = layout;
        m_layouts[validName] = layout;
    };

    buildTaggedUnionLayout("Result");
    buildTaggedUnionLayout("Option");

    for (const auto& decl : program.getDeclarations()) {
        if (!decl) continue;
        if (decl->getKind() == ast::ASTNodeKind::EnumDecl) {
            const auto& ed = static_cast<const ast::EnumDecl&>(*decl);
            bool hasPayload = false;
            for (const auto& v : ed.getVariants()) {
                if (v.hasPayload()) { hasPayload = true; break; }
            }
            if (hasPayload) {
                buildTaggedUnionLayout(ed.getName());
            }
        }
    }

    for (const auto& [typeName, userType] : m_sema.getTypeContext().getUserTypes()) {
        if (userType && userType->isEnum()) {
            auto en = std::static_pointer_cast<sema::EnumType>(userType);
            if (en && en->isTaggedUnion()) {
                buildTaggedUnionLayout(en->getName());
                std::string mangled = mangleGenericName(en->getName());
                buildTaggedUnionLayout(mangled);
            }
        }
    }
}

void VTableBuilder::buildClassLayout(const ast::ClassDecl& classDecl, ir::IRModule& irModule) {
    if (classDecl.isGeneric()) {
        return; // Generic templates are monomorphized into concrete layouts
    }
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
    bool baseCopied = false;
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

            // Phase 6 slice B: a derived class sees its base's statics
            // too (Derived.PI resolves to Base's global unless the
            // derived class declares its own).
            for (const auto& [sname, sinfo] : baseLayout.staticFields) {
                layout.staticFields[sname] = sinfo;
            }

            // Copy base vtable slots
            layout.vtableSlots = baseLayout.vtableSlots;
            layout.methodSlotMap = baseLayout.methodSlotMap;
            baseCopied = true;
        }
    }

    // Phase 6 slice A: the interface region. Every vtable starts with
    // the global interface slots (null until a method of this class
    // fills one), so an interface-typed call dispatches to the same
    // slot index for every implementing class. A derived class inherits
    // the region from its base (base copy above), so it is only
    // prepended when no base vtable was copied.
    if (!baseCopied) {
        for (size_t i = 0; i < m_ifaceSlots.size(); ++i) {
            VTableSlotInfo slot;
            slot.methodName = m_ifaceSlots[i].methodName;
            slot.mangledName = "";
            slot.slotIndex = static_cast<uint32_t>(i);
            slot.functionType = m_ifaceSlots[i].functionType;
            slot.originatingClass = m_ifaceSlots[i].interfaceName;
            layout.vtableSlots.push_back(slot);
            layout.methodSlotMap[m_ifaceSlots[i].key] = static_cast<uint32_t>(i);
        }
    }

    // Add this class's own fields
    for (const auto& member : classDecl.getMembers()) {
        if (!member) continue;
        if (member->getKind() == ast::ASTNodeKind::VarDeclStmt) {
            const auto& varDecl = static_cast<const ast::VarDeclStmt&>(*member);
            if (varDecl.isStatic()) {
                // Phase 6 slice B: static fields live in a module-level
                // global (zero-initialized; the static-init function
                // stores the declared initializer before main runs).
                // They are NOT in the instance layout.
                StaticFieldInfo info;
                info.name = varDecl.getName();
                info.typeName = varDecl.getType();
                info.irType = mapType(varDecl.getType());
                info.globalName = "@" + className + "_" + varDecl.getName();
                info.isConst = varDecl.isConst();
                info.isWeak = varDecl.isWeak();
                layout.staticFields[info.name] = info;
                irModule.addGlobalVariable(info.globalName, info.irType, /*isConstant=*/false, "zeroinitializer");
                continue;
            }

            FieldLayoutInfo fieldInfo;
            fieldInfo.name = varDecl.getName();
            fieldInfo.typeName = varDecl.getType();
            fieldInfo.irType = mapType(varDecl.getType());
            fieldInfo.index = currentFieldIndex++;
            fieldInfo.className = className;
            fieldInfo.isWeak = varDecl.isWeak();

            layout.fields.push_back(fieldInfo);
            layout.fieldIndexMap[fieldInfo.name] = fieldInfo.index;
            elementTypes.push_back(fieldInfo.irType);
            totalBytes += 8;
        }
    }

    // Sema class type for this class (canonical method signatures for
    // overload mangling; null for builtin classes without a decl).
    const sema::ClassType* clsType = nullptr;
    if (auto userType = m_sema.getTypeContext().findUserType(className)) {
        if (userType->isClass()) {
            clsType = static_cast<const sema::ClassType*>(userType.get());
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
            // Overload key: method name + signature suffix. The vtable map
            // is keyed by this, so same-name overloads occupy distinct
            // slots and a derived override (same signature) finds the base
            // slot. The stored mangledName only gains the suffix when the
            // name is overloaded in this class (single-definition methods
            // keep their plain `Class_method` LLVM name).
            std::vector<std::string> paramTypes;
            bool isOverloaded = false;
            if (clsType) {
                paramTypes = canonicalParamTypes(*clsType, fnDecl);
                isOverloaded = isMethodOverloaded(*clsType, methodName);
            } else {
                for (const auto& p : fnDecl.getParams()) paramTypes.push_back(p.getType());
            }
            std::string methodKey = methodName + overloadSuffix(paramTypes);
            std::string mangledName = mangleOverloaded(className + "_" + methodName, paramTypes, isOverloaded);

            // Phase 6 slice A: the slot's function type, from the sema
            // signature. Without it the vtable dispatch computed the
            // call's return type from the module's function table,
            // which only holds functions lowered so far — a forward
            // call to a value-returning method got return type void,
            // the call result became nullptr, and the verifier
            // segfaulted formatting the null operand. (Builtin classes
            // without a decl keep the module-lookup fallback.)
            ir::IRTypePtr fnType = nullptr;
            if (clsType) {
                if (const auto* mm = findBestMemberMethod(*clsType, fnDecl)) {
                    if (mm->functionType) fnType = methodFunctionType(*this, *mm->functionType);
                }
            }

            // Phase 6 slice A: a method implementing an interface
            // method belongs in that method's global interface slot(s)
            // — otherwise an interface-typed call would load a
            // different index for each class. An override updates
            // BOTH its inherited slot (for base-typed dispatch) and
            // the interface slot(s) (for interface-typed dispatch).
            auto ifaceSlots = interfaceSlotsForMethod(methodKey, classDecl.getInterfaces());
            auto slotIt = layout.methodSlotMap.find(methodKey);
            if (slotIt != layout.methodSlotMap.end()) {
                // Method override: update existing slot
                uint32_t slotIdx = slotIt->second;
                layout.vtableSlots[slotIdx].mangledName = mangledName;
                layout.vtableSlots[slotIdx].originatingClass = className;
                if (fnType) layout.vtableSlots[slotIdx].functionType = fnType;
            }
            for (uint32_t slotIdx : ifaceSlots) {
                layout.vtableSlots[slotIdx].mangledName = mangledName;
                layout.vtableSlots[slotIdx].originatingClass = className;
                if (fnType) layout.vtableSlots[slotIdx].functionType = fnType;
            }
            if (slotIt == layout.methodSlotMap.end() && ifaceSlots.empty()) {
                // New virtual method: append to vtable
                uint32_t newSlotIdx = static_cast<uint32_t>(layout.vtableSlots.size());
                VTableSlotInfo slotInfo;
                slotInfo.methodName = methodName;
                slotInfo.mangledName = mangledName;
                slotInfo.slotIndex = newSlotIdx;
                slotInfo.originatingClass = className;
                slotInfo.functionType = fnType;

                layout.vtableSlots.push_back(slotInfo);
                layout.methodSlotMap[methodKey] = newSlotIdx;
            } else if (slotIt == layout.methodSlotMap.end()) {
                // First declaration is an interface implementation:
                // the plain key resolves to the first interface slot
                // (all of them hold this same function).
                layout.methodSlotMap[methodKey] = ifaceSlots.front();
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
            // Phase 6 slice A: interface-region slots this class does
            // not fill are null — emitting `ptr @` for an empty name
            // would be invalid LLVM IR.
            if (layout.vtableSlots[i].mangledName.empty()) {
                vtableRawVal += "ptr null";
            } else {
                vtableRawVal += "ptr @" + layout.vtableSlots[i].mangledName;
            }
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
    layout.isReprC = structDecl.isReprC();

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

void VTableBuilder::buildSpecializedClassLayout(const std::shared_ptr<sema::ClassType>& clsType, ir::IRModule& irModule) {
    std::string className = mangleGenericName(clsType->getName());
    if (className.rfind("List", 0) == 0 || className.rfind("Map", 0) == 0 || className.rfind("Set", 0) == 0 ||
        className.rfind("Stack", 0) == 0 || className.rfind("Queue", 0) == 0 || className.rfind("PriorityQueue", 0) == 0 ||
        className.rfind("Channel", 0) == 0) {
        return;
    }
    if (m_layouts.find(className) != m_layouts.end()) {
        return;
    }

    ClassLayout layout;
    layout.name = className;
    layout.baseClass = clsType->getSuperclass() ? clsType->getSuperclass()->getName() : "";
    layout.hasVTable = true;
    layout.vtableName = "@_vtable_" + className;

    std::vector<ir::IRTypePtr> elementTypes;
    // Slot 0: vtable pointer
    elementTypes.push_back(ir::IRType::getPointer());
    uint32_t currentFieldIndex = 1;
    uint32_t totalBytes = 8;

    // Fields
    for (const auto& [fname, field] : clsType->getFields()) {
        ir::IRTypePtr fIrType = mapType(field.type->toString());
        FieldLayoutInfo fInfo{
            .name = fname,
            .typeName = field.type->toString(),
            .irType = fIrType,
            .index = currentFieldIndex++,
            .className = className,
            .isWeak = field.isWeak
        };
        layout.fields.push_back(fInfo);
        layout.fieldIndexMap[fname] = fInfo.index;
        elementTypes.push_back(fIrType);
        totalBytes += 8;
    }

    std::string baseTemplateName = clsType->getName();
    size_t ltPos = baseTemplateName.find('<');
    if (ltPos != std::string::npos) {
        baseTemplateName = baseTemplateName.substr(0, ltPos);
    }

    // Phase 5 slice A: Task<T> is a builtin opaque handle — its
    // methods (cancel / isReady / wasCancelled) lower directly to the
    // async runtime C ABI (see IRBuilder), so they have no IR function
    // bodies and must not occupy vtable slots (the vtable would
    // reference undefined functions). Task has no subclasses and no
    // virtual dispatch; the methods are resolved by the IRBuilder's
    // special-casing, not through the vtable. (An empty vtable emits
    // `ptr null`, which is valid.)
    bool isTaskClass = (baseTemplateName == "Task");

    // Methods / VTable slots
    uint32_t currentSlot = 0;
    std::vector<ir::IRValuePtr> vtableFnConstants;
    for (const auto& method : clsType->getMethods()) {
        if (isTaskClass) continue;
        if (method.name == baseTemplateName || method.name == "init" || method.name == className) {
            continue;
        }
        std::string mangledName = className + "_" + method.name;
        ir::IRTypePtr fnType = nullptr;
        if (method.functionType) {
            auto fnT = std::static_pointer_cast<sema::FunctionType>(method.functionType);
            std::vector<ir::IRTypePtr> paramTypes;
            paramTypes.push_back(ir::IRType::getPointer()); // %this
            for (const auto& pt : fnT->getParamTypes()) {
                paramTypes.push_back(mapType(pt->toString()));
            }
            fnType = ir::IRType::getFunction(mapType(fnT->getReturnType()->toString()), paramTypes);
        }

        VTableSlotInfo slot{
            .methodName = method.name,
            .mangledName = mangledName,
            .slotIndex = currentSlot,
            .functionType = fnType,
            .originatingClass = className
        };
        layout.vtableSlots.push_back(slot);
        layout.methodSlotMap[method.name] = currentSlot;
        // Phase 6 slice A: also register the overload key
        // (methodName + signature suffix) so the keyed lookup in
        // IRBuilder finds bare method calls in generic classes, the
        // same way it does for non-generic classes.
        if (method.functionType) {
            auto fnT = std::static_pointer_cast<sema::FunctionType>(method.functionType);
            std::vector<std::string> paramStrs;
            for (const auto& pt : fnT->getParamTypes()) paramStrs.push_back(pt->toString());
            layout.methodSlotMap[method.name + overloadSuffix(paramStrs)] = currentSlot;
        }
        ++currentSlot;
    }

    layout.objectSize = totalBytes;
    layout.structType = ir::IRType::getStruct("class." + className, elementTypes);
    irModule.addStructType("class." + className, elementTypes);

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

    auto vtableArrayType = ir::IRType::getPointer();
    irModule.addGlobalVariable(layout.vtableName, vtableArrayType, true, vtableRawVal);

    m_layouts[className] = layout;
    m_layouts[clsType->getName()] = layout;
}

const ClassLayout* VTableBuilder::getClassLayout(const std::string& className) const {
    auto it = m_layouts.find(className);
    if (it != m_layouts.end()) {
        return &it->second;
    }
    std::string mangled = mangleGenericName(className);
    auto it2 = m_layouts.find(mangled);
    if (it2 != m_layouts.end()) {
        return &it2->second;
    }
    size_t lt = className.find('<');
    if (lt != std::string::npos) {
        std::string base = className.substr(0, lt);
        auto it3 = m_layouts.find(base);
        if (it3 != m_layouts.end()) {
            return &it3->second;
        }
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
    // Legacy name-only lookup: returns the first slot with this method name.
    // Kept for non-overloaded callers and for generic-class layouts, whose
    // slot map is still keyed by plain name. Overloaded calls must use
    // getMethodVTableSlotByKey.
    // Phase 6 slice A: skip unfilled interface-region slots (empty
    // mangledName) — a null slot is never a valid dispatch target, and
    // it precedes a same-named appended method in the table.
    const auto* layout = getClassLayout(className);
    if (!layout) return -1;
    for (const auto& slot : layout->vtableSlots) {
        if (slot.methodName == methodName && !slot.mangledName.empty()) return static_cast<int>(slot.slotIndex);
    }
    return -1;
}

int VTableBuilder::getMethodVTableSlotByKey(const std::string& className, const std::string& methodKey) const {
    const auto* layout = getClassLayout(className);
    if (!layout) return -1;
    auto it = layout->methodSlotMap.find(methodKey);
    if (it != layout->methodSlotMap.end()) {
        return static_cast<int>(it->second);
    }
    return -1;
}

} // namespace sp::codegen
