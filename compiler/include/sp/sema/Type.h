#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace sp::ast {
class ClassDecl;
class EnumDecl;
}

namespace sp::sema {

enum class TypeKind {
    // Primitives
    Void,
    Null,
    Bool,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    Char,
    String,

    // Compound
    Array,
    Map,
    Nullable,
    Function,
    Pointer,

    // User-defined nominal types
    Class,
    Interface,
    Struct,
    Enum,

    // Special compiler types
    Any,
    Unknown,
    Error
};

class ClassType;
class InterfaceType;

class Type : public std::enable_shared_from_this<Type> {
public:
    explicit Type(TypeKind kind) : m_kind(kind) {}
    virtual ~Type() = default;

    [[nodiscard]] TypeKind getKind() const noexcept { return m_kind; }
    [[nodiscard]] virtual std::string toString() const = 0;

    // Type queries
    [[nodiscard]] bool isNumeric() const noexcept;
    [[nodiscard]] bool isInteger() const noexcept;
    [[nodiscard]] bool isSignedInteger() const noexcept;
    [[nodiscard]] bool isUnsignedInteger() const noexcept;
    [[nodiscard]] bool isFloatingPoint() const noexcept;
    [[nodiscard]] bool isFloat() const noexcept { return m_kind == TypeKind::Float; }
    [[nodiscard]] bool isDouble() const noexcept { return m_kind == TypeKind::Double; }
    [[nodiscard]] bool isBoolean() const noexcept { return m_kind == TypeKind::Bool; }
    [[nodiscard]] bool isChar() const noexcept { return m_kind == TypeKind::Char; }
    [[nodiscard]] bool isString() const noexcept { return m_kind == TypeKind::String; }
    [[nodiscard]] bool isVoid() const noexcept { return m_kind == TypeKind::Void; }
    [[nodiscard]] bool isNull() const noexcept { return m_kind == TypeKind::Null; }
    [[nodiscard]] bool isNullable() const noexcept { return m_kind == TypeKind::Nullable; }
    [[nodiscard]] bool isPointer() const noexcept { return m_kind == TypeKind::Pointer; }
    [[nodiscard]] bool isArray() const noexcept { return m_kind == TypeKind::Array; }
    [[nodiscard]] bool isMap() const noexcept { return m_kind == TypeKind::Map; }
    [[nodiscard]] bool isFunction() const noexcept { return m_kind == TypeKind::Function; }
    [[nodiscard]] bool isClass() const noexcept { return m_kind == TypeKind::Class; }
    [[nodiscard]] bool isInterface() const noexcept { return m_kind == TypeKind::Interface; }
    [[nodiscard]] bool isStruct() const noexcept { return m_kind == TypeKind::Struct; }
    [[nodiscard]] bool isEnum() const noexcept { return m_kind == TypeKind::Enum; }
    [[nodiscard]] bool isReferenceType() const noexcept { return m_kind == TypeKind::Class || m_kind == TypeKind::Interface; }
    [[nodiscard]] bool isValueType() const noexcept { return !isReferenceType(); }
    [[nodiscard]] bool isAny() const noexcept { return m_kind == TypeKind::Any; }
    [[nodiscard]] bool isError() const noexcept { return m_kind == TypeKind::Error; }
    [[nodiscard]] bool isUnknown() const noexcept { return m_kind == TypeKind::Unknown; }

    // Relationship and conversion checks
    [[nodiscard]] virtual bool equals(const Type& other) const;
    [[nodiscard]] virtual bool canImplicitlyConvertTo(const Type& target) const;
    [[nodiscard]] virtual bool isSubtypeOf(const Type& target) const;
    [[nodiscard]] virtual bool isAssignableTo(const Type& target) const;

    bool operator==(const Type& other) const { return equals(other); }
    bool operator!=(const Type& other) const { return !equals(other); }

private:
    TypeKind m_kind;
};

using TypePtr = std::shared_ptr<Type>;

// Primitive types (int, float, bool, string, etc.)
class PrimitiveType : public Type {
public:
    PrimitiveType(TypeKind kind, std::string name)
        : Type(kind), m_name(std::move(name)) {}

    [[nodiscard]] std::string toString() const override { return m_name; }

private:
    std::string m_name;
};

// Array types (e.g. int[], string[])
class ArrayType : public Type {
public:
    explicit ArrayType(TypePtr elementType)
        : Type(TypeKind::Array), m_elementType(std::move(elementType)) {}

    [[nodiscard]] const TypePtr& getElementType() const noexcept { return m_elementType; }
    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    TypePtr m_elementType;
};

// Map types (e.g. Map<string, int>)
class MapType : public Type {
public:
    MapType(TypePtr keyType, TypePtr valueType)
        : Type(TypeKind::Map), m_keyType(std::move(keyType)), m_valueType(std::move(valueType)) {}

    [[nodiscard]] const TypePtr& getKeyType() const noexcept { return m_keyType; }
    [[nodiscard]] const TypePtr& getValueType() const noexcept { return m_valueType; }
    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    TypePtr m_keyType;
    TypePtr m_valueType;
};

// Nullable types (e.g. int?, string?, Person?)
class NullableType : public Type {
public:
    explicit NullableType(TypePtr underlyingType)
        : Type(TypeKind::Nullable), m_underlyingType(std::move(underlyingType)) {}

    [[nodiscard]] const TypePtr& getUnderlyingType() const noexcept { return m_underlyingType; }
    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    TypePtr m_underlyingType;
};

// Function types: (paramTypes) -> returnType
class FunctionType : public Type {
public:
    FunctionType(std::vector<TypePtr> paramTypes, TypePtr returnType)
        : Type(TypeKind::Function), m_paramTypes(std::move(paramTypes)), m_returnType(std::move(returnType)) {}

    [[nodiscard]] const std::vector<TypePtr>& getParamTypes() const noexcept { return m_paramTypes; }
    [[nodiscard]] const TypePtr& getReturnType() const noexcept { return m_returnType; }
    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    std::vector<TypePtr> m_paramTypes;
    TypePtr m_returnType;
};

// Raw pointer types: ptr<T>
class PointerType : public Type {
public:
    explicit PointerType(TypePtr pointeeType)
        : Type(TypeKind::Pointer), m_pointeeType(std::move(pointeeType)) {}

    [[nodiscard]] const TypePtr& getPointeeType() const noexcept { return m_pointeeType; }
    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    TypePtr m_pointeeType;
};

// Field descriptor for Nominal types
struct MemberField {
    std::string name;
    TypePtr type;
    std::string visibility; // "public", "private", "protected", "internal"
    bool isStatic{false};
    bool isConst{false};
    bool isWeak{false};
};

// Method descriptor for Nominal types
struct MemberMethod {
    std::string name;
    std::shared_ptr<FunctionType> functionType;
    std::string visibility; // "public", "private", "protected", "internal"
    bool isStatic{false};
    bool isAbstract{false};
    bool isOverride{false};
};

// Class type
class ClassType : public Type {
public:
    explicit ClassType(std::string name)
        : Type(TypeKind::Class), m_name(std::move(name)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] std::string toString() const override { return m_name; }

    void setSuperclass(std::shared_ptr<ClassType> superClass) { m_superClass = std::move(superClass); }
    [[nodiscard]] const std::shared_ptr<ClassType>& getSuperclass() const noexcept { return m_superClass; }

    void addInterface(std::shared_ptr<InterfaceType> iface) { m_interfaces.push_back(std::move(iface)); }
    [[nodiscard]] const std::vector<std::shared_ptr<InterfaceType>>& getInterfaces() const noexcept { return m_interfaces; }

    void setAbstract(bool isAbstract) noexcept { m_isAbstract = isAbstract; }
    [[nodiscard]] bool isAbstract() const noexcept { return m_isAbstract; }

    void addField(MemberField field) { m_fields[field.name] = std::move(field); }
    [[nodiscard]] std::optional<MemberField> findField(const std::string& name) const;

    void addMethod(MemberMethod method) { m_methods.push_back(std::move(method)); }
    [[nodiscard]] std::vector<MemberMethod> findMethods(const std::string& name) const;

    [[nodiscard]] const std::unordered_map<std::string, MemberField>& getFields() const noexcept { return m_fields; }
    [[nodiscard]] const std::vector<MemberMethod>& getMethods() const noexcept { return m_methods; }

    void setTypeParams(std::vector<std::string> params) { m_typeParams = std::move(params); }
    [[nodiscard]] const std::vector<std::string>& getTypeParams() const noexcept { return m_typeParams; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeParams.empty(); }

    void setASTDecl(const ast::ClassDecl* decl) noexcept { m_astDecl = decl; }
    [[nodiscard]] const ast::ClassDecl* getASTDecl() const noexcept { return m_astDecl; }

    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isSubtypeOf(const Type& target) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    std::string m_name;
    std::vector<std::string> m_typeParams;
    const ast::ClassDecl* m_astDecl{nullptr};
    std::shared_ptr<ClassType> m_superClass{nullptr};
    std::vector<std::shared_ptr<InterfaceType>> m_interfaces;
    std::unordered_map<std::string, MemberField> m_fields;
    std::vector<MemberMethod> m_methods;
    bool m_isAbstract{false};
};

// Interface type
class InterfaceType : public Type {
public:
    explicit InterfaceType(std::string name)
        : Type(TypeKind::Interface), m_name(std::move(name)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] std::string toString() const override { return m_name; }

    void addSuperInterface(std::shared_ptr<InterfaceType> iface) { m_superInterfaces.push_back(std::move(iface)); }
    [[nodiscard]] const std::vector<std::shared_ptr<InterfaceType>>& getSuperInterfaces() const noexcept { return m_superInterfaces; }

    void addMethod(MemberMethod method) { m_methods.push_back(std::move(method)); }
    [[nodiscard]] std::vector<MemberMethod> findMethods(const std::string& name) const;
    [[nodiscard]] const std::vector<MemberMethod>& getMethods() const noexcept { return m_methods; }

    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isSubtypeOf(const Type& target) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    std::string m_name;
    std::vector<std::shared_ptr<InterfaceType>> m_superInterfaces;
    std::vector<MemberMethod> m_methods;
};

// Struct type (value type)
class StructType : public Type {
public:
    explicit StructType(std::string name)
        : Type(TypeKind::Struct), m_name(std::move(name)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] std::string toString() const override { return m_name; }

    void addField(MemberField field) { m_fields[field.name] = std::move(field); }
    [[nodiscard]] std::optional<MemberField> findField(const std::string& name) const;
    [[nodiscard]] const std::unordered_map<std::string, MemberField>& getFields() const noexcept { return m_fields; }

    [[nodiscard]] bool equals(const Type& other) const override;

private:
    std::string m_name;
    std::unordered_map<std::string, MemberField> m_fields;
};

// Enum type
class EnumType : public Type {
public:
    explicit EnumType(std::string name)
        : Type(TypeKind::Enum), m_name(std::move(name)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] std::string toString() const override { return m_name; }

    void addVariant(std::string name, std::optional<int64_t> value = std::nullopt,
                    std::vector<std::string> payloadTypeNames = {},
                    std::vector<TypePtr> payloadTypes = {}) {
        m_variants.push_back(name);
        if (value.has_value()) {
            m_values[name] = value.value();
        }
        if (!payloadTypeNames.empty()) {
            m_variantPayloadNames[name] = std::move(payloadTypeNames);
            m_variantPayloadTypes[name] = std::move(payloadTypes);
            m_isTaggedUnion = true;
        }
    }

    [[nodiscard]] bool hasVariant(const std::string& name) const noexcept {
        for (const auto& v : m_variants) {
            if (v == name) return true;
        }
        return false;
    }

    [[nodiscard]] int64_t getVariantValue(const std::string& name) const {
        auto it = m_values.find(name);
        if (it != m_values.end()) {
            return it->second;
        }
        for (size_t i = 0; i < m_variants.size(); ++i) {
            if (m_variants[i] == name) return static_cast<int64_t>(i);
        }
        return 0;
    }

    [[nodiscard]] const std::vector<std::string>& getVariants() const noexcept { return m_variants; }

    [[nodiscard]] bool isTaggedUnion() const noexcept { return m_isTaggedUnion; }
    void setTaggedUnion(bool val) noexcept { m_isTaggedUnion = val; }

    [[nodiscard]] bool hasPayload(const std::string& name) const noexcept {
        auto it = m_variantPayloadTypes.find(name);
        return it != m_variantPayloadTypes.end() && !it->second.empty();
    }

    [[nodiscard]] const std::vector<TypePtr>& getVariantPayload(const std::string& name) const {
        static const std::vector<TypePtr> emptyTypes;
        auto it = m_variantPayloadTypes.find(name);
        if (it != m_variantPayloadTypes.end()) {
            return it->second;
        }
        return emptyTypes;
    }

    [[nodiscard]] const std::vector<std::string>& getVariantPayloadNames(const std::string& name) const {
        static const std::vector<std::string> emptyNames;
        auto it = m_variantPayloadNames.find(name);
        if (it != m_variantPayloadNames.end()) {
            return it->second;
        }
        return emptyNames;
    }

    void setTypeParams(std::vector<std::string> params) { m_typeParams = std::move(params); }
    [[nodiscard]] const std::vector<std::string>& getTypeParams() const noexcept { return m_typeParams; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeParams.empty(); }

    void setASTDecl(const ast::EnumDecl* decl) noexcept { m_astDecl = decl; }
    [[nodiscard]] const ast::EnumDecl* getASTDecl() const noexcept { return m_astDecl; }

    [[nodiscard]] bool equals(const Type& other) const override;
    [[nodiscard]] bool isAssignableTo(const Type& target) const override;

private:
    std::string m_name;
    std::vector<std::string> m_typeParams;
    const ast::EnumDecl* m_astDecl{nullptr};
    std::vector<std::string> m_variants;
    std::unordered_map<std::string, int64_t> m_values;
    std::unordered_map<std::string, std::vector<std::string>> m_variantPayloadNames;
    std::unordered_map<std::string, std::vector<TypePtr>> m_variantPayloadTypes;
    bool m_isTaggedUnion{false};
};

// Central Type Context caching singleton primitives and factories
class TypeContext {
public:
    TypeContext();
    ~TypeContext() = default;

    // Primitives accessors
    [[nodiscard]] const TypePtr& getVoid() const noexcept { return m_voidType; }
    [[nodiscard]] const TypePtr& getNull() const noexcept { return m_nullType; }
    [[nodiscard]] const TypePtr& getBool() const noexcept { return m_boolType; }
    [[nodiscard]] const TypePtr& getInt8() const noexcept { return m_int8Type; }
    [[nodiscard]] const TypePtr& getUInt8() const noexcept { return m_uint8Type; }
    [[nodiscard]] const TypePtr& getInt16() const noexcept { return m_int16Type; }
    [[nodiscard]] const TypePtr& getUInt16() const noexcept { return m_uint16Type; }
    [[nodiscard]] const TypePtr& getInt32() const noexcept { return m_int32Type; }
    [[nodiscard]] const TypePtr& getUInt32() const noexcept { return m_uint32Type; }
    [[nodiscard]] const TypePtr& getInt64() const noexcept { return m_int64Type; }
    [[nodiscard]] const TypePtr& getUInt64() const noexcept { return m_uint64Type; }
    [[nodiscard]] const TypePtr& getFloat() const noexcept { return m_floatType; }
    [[nodiscard]] const TypePtr& getDouble() const noexcept { return m_doubleType; }
    [[nodiscard]] const TypePtr& getChar() const noexcept { return m_charType; }
    [[nodiscard]] const TypePtr& getString() const noexcept { return m_stringType; }
    [[nodiscard]] const TypePtr& getAny() const noexcept { return m_anyType; }
    [[nodiscard]] const TypePtr& getUnknown() const noexcept { return m_unknownType; }
    [[nodiscard]] const TypePtr& getError() const noexcept { return m_errorType; }

    // Aliases
    [[nodiscard]] const TypePtr& getInt() const noexcept { return m_int32Type; }
    [[nodiscard]] const TypePtr& getUInt() const noexcept { return m_uint32Type; }
    [[nodiscard]] const TypePtr& getByte() const noexcept { return m_uint8Type; }

    // Compound factory methods
    TypePtr getArrayType(TypePtr elementType);
    TypePtr getMapType(TypePtr keyType, TypePtr valueType);
    TypePtr getNullableType(TypePtr underlyingType);
    TypePtr getPointerType(TypePtr pointeeType);
    std::shared_ptr<FunctionType> getFunctionType(std::vector<TypePtr> paramTypes, TypePtr returnType);

    // User-defined nominal type factory & registry
    std::shared_ptr<ClassType> createClassType(const std::string& name);
    std::shared_ptr<InterfaceType> createInterfaceType(const std::string& name);
    std::shared_ptr<StructType> createStructType(const std::string& name);
    std::shared_ptr<EnumType> createEnumType(const std::string& name);

    void registerUserType(const std::string& name, TypePtr type);
    [[nodiscard]] TypePtr findUserType(const std::string& name) const;
    [[nodiscard]] const std::unordered_map<std::string, TypePtr>& getUserTypes() const noexcept { return m_userTypes; }

    // Generic specialization
    std::shared_ptr<ClassType> specializeClassType(const std::shared_ptr<ClassType>& baseCls, const std::vector<std::string>& typeArgs);
    std::shared_ptr<EnumType> specializeEnumType(const std::shared_ptr<EnumType>& baseEnum, const std::vector<std::string>& typeArgs);
    static std::vector<std::string> splitTypeArgs(const std::string& inner);
    static std::string substituteType(const std::string& typeStr, const std::vector<std::string>& typeParams, const std::vector<std::string>& typeArgs);

    // Type string resolution (e.g. "int", "string?", "int[]", "Person")
    TypePtr resolveType(const std::string& typeStr);

    // Common type determination for binary expressions and ternaries
    TypePtr getCommonType(const TypePtr& a, const TypePtr& b);

private:
    TypePtr m_voidType;
    TypePtr m_nullType;
    TypePtr m_boolType;
    TypePtr m_int8Type;
    TypePtr m_uint8Type;
    TypePtr m_int16Type;
    TypePtr m_uint16Type;
    TypePtr m_int32Type;
    TypePtr m_uint32Type;
    TypePtr m_int64Type;
    TypePtr m_uint64Type;
    TypePtr m_floatType;
    TypePtr m_doubleType;
    TypePtr m_charType;
    TypePtr m_stringType;
    TypePtr m_anyType;
    TypePtr m_unknownType;
    TypePtr m_errorType;

    std::unordered_map<std::string, TypePtr> m_userTypes;
    std::unordered_map<std::string, TypePtr> m_cachedTypes;
};

} // namespace sp::sema
