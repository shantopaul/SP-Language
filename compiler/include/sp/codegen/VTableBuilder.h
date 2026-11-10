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

// Phase 6 slice B: a static field lives in a module-level global, not
// in the instance layout (statics were never allocated anywhere —
// reads returned zero and `ClassName.field` segfaulted).
struct StaticFieldInfo {
    std::string name;
    std::string typeName;
    ir::IRTypePtr irType;
    std::string globalName; // e.g. "@MathUtils_PI"
    bool isConst = false;
};

struct VTableSlotInfo {
    std::string methodName;
    std::string mangledName; // e.g. "Animal_speak" or "Dog_speak"
    uint32_t slotIndex;
    ir::IRTypePtr functionType;
    std::string originatingClass;
};

// Phase 6 slice A: one entry of the global interface-slot table.
// Every interface method in the program gets exactly one global slot
// (keyed by declaring interface + method + signature suffix), and
// every class vtable starts with this region so an interface-typed
// call dispatches to the same slot index no matter which class the
// object really is. Slots a class does not fill stay null
// (mangledName empty) — they are unreachable through that class,
// since sema forbids viewing it through an interface it does not
// implement.
struct InterfaceSlotInfo {
    std::string key; // e.g. "Drawable::draw__void"
    std::string interfaceName;
    std::string methodName;
    ir::IRTypePtr functionType; // (ptr %this, params...) -> return
};

struct ClassLayout {
    std::string name;
    std::string baseClass;
    bool hasVTable = true;
    bool isReprC = false;
    uint32_t objectSize = 0; // Size in bytes
    ir::IRStructTypePtr structType;
    std::vector<FieldLayoutInfo> fields;
    std::unordered_map<std::string, uint32_t> fieldIndexMap;
    // Phase 6 slice B: static fields by name (module globals).
    std::unordered_map<std::string, StaticFieldInfo> staticFields;
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
    // Phase 8 slice A: enumerate every layout (destructor generation).
    [[nodiscard]] const std::unordered_map<std::string, ClassLayout>& getAllLayouts() const noexcept { return m_layouts; }
    [[nodiscard]] int getFieldIndex(const std::string& className, const std::string& fieldName) const;
    [[nodiscard]] int getMethodVTableSlot(const std::string& className, const std::string& methodName) const;
    // Signature-aware slot lookup: methodKey is the overload key, i.e.
    // methodName + overloadSuffix(paramTypes) (see below). Used for
    // overloaded methods, whose plain name is ambiguous.
    [[nodiscard]] int getMethodVTableSlotByKey(const std::string& className, const std::string& methodKey) const;

    // Helper to map type names to IR types
    ir::IRTypePtr mapType(const std::string& typeName) const;
    [[nodiscard]] const sema::SemanticAnalyzer& getSema() const noexcept { return m_sema; }
    static std::string mangleGenericName(const std::string& name);

    // Phase 3 overload mangling (shared by IRBuilder and VTableBuilder).
    //
    // A single-definition function keeps its plain LLVM name
    // (e.g. `foo`, `Animal_speak`) so existing IR/tests are unchanged.
    // When a name is overloaded, the LLVM name gains a signature suffix:
    //   base + "__" + join(sanitized(param types), "_")   (no params -> "__void")
    // The param types are sema-canonical type strings (Type::toString()),
    // e.g. `int`, `string`, `int[]`, `ptr<int>`, `Foo?`. Sanitization keeps
    // [A-Za-z0-9_] and replaces every other character with '_':
    //   add(int, string)  -> add__int_string
    //   add(int, int)     -> add__int_int
    //   get(int[])        -> get__int__
    //   f(ptr<int>)       -> f__ptr_int_
    //   f()               -> f__void
    //   Animal_speak()    -> Animal_speak__void  (when overloaded)
    // The vtable slot KEY for a method is methodName + overloadSuffix(...),
    // always suffixed (even when not overloaded), so a derived override of
    // the same signature finds the base slot; the stored mangledName (the
    // LLVM function name) only carries the suffix when overloaded.
    static std::string sanitizeMangledType(const std::string& typeName);
    static std::string overloadSuffix(const std::vector<std::string>& canonicalParamTypes);
    static std::string mangleOverloaded(const std::string& baseName,
                                        const std::vector<std::string>& canonicalParamTypes,
                                        bool isOverloaded);

    // Canonical (sema) param-type strings for a method decl, taken from the
    // class's MemberMethod table (so `int32` and `int` spell the same
    // signature), and whether a method name is overloaded in a class
    // (more than one distinct visible signature). Used by IRBuilder and by
    // the vtable builder itself so definition and call sites agree.
    static std::vector<std::string> canonicalParamTypes(const sema::ClassType& clsType,
                                                        const ast::FunctionDecl& decl);
    static bool isMethodOverloaded(const sema::ClassType& clsType, const std::string& methodName);

private:
    void buildClassLayout(const ast::ClassDecl& classDecl, ir::IRModule& irModule);
    void buildSpecializedClassLayout(const std::shared_ptr<sema::ClassType>& clsType, ir::IRModule& irModule);
    void buildStructLayout(const ast::StructDecl& structDecl, ir::IRModule& irModule);

    // Phase 6 slice A: global interface-slot scheme (see
    // InterfaceSlotInfo). buildInterfaceSlots() must run before any
    // class layout is built; buildInterfaceLayout() registers a
    // {ptr}-only layout per interface so interface-typed calls can
    // dispatch through the same vtable path as class-typed calls.
    void buildInterfaceSlots();
    void buildInterfaceLayout(const std::shared_ptr<sema::InterfaceType>& iface, ir::IRModule& irModule);
    // Global slot indices whose qualified key has this method-part
    // key (methodName + suffix) AND whose interface is in
    // implementedInterfaces (empty list = no interface slots).
    std::vector<uint32_t> interfaceSlotsForMethod(
        const std::string& methodKey, const std::vector<std::string>& implementedInterfaces) const;

    const sema::SemanticAnalyzer& m_sema;
    std::unordered_map<std::string, ClassLayout> m_layouts;
    std::unordered_map<std::string, const ast::ClassDecl*> m_classDecls;
    std::vector<InterfaceSlotInfo> m_ifaceSlots;
    std::unordered_map<std::string, uint32_t> m_ifaceSlotIndex;
    bool m_ifaceSlotsBuilt = false;
};

} // namespace sp::codegen
