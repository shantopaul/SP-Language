#include "sp/sema/Type.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Statement.h"
#include <sstream>
#include <algorithm>

namespace sp::sema {

bool Type::isNumeric() const noexcept {
    return isInteger() || isFloatingPoint();
}

bool Type::isInteger() const noexcept {
    return isSignedInteger() || isUnsignedInteger();
}

bool Type::isSignedInteger() const noexcept {
    switch (m_kind) {
        case TypeKind::Int8:
        case TypeKind::Int16:
        case TypeKind::Int32:
        case TypeKind::Int64:
            return true;
        default:
            return false;
    }
}

bool Type::isUnsignedInteger() const noexcept {
    switch (m_kind) {
        case TypeKind::UInt8:
        case TypeKind::UInt16:
        case TypeKind::UInt32:
        case TypeKind::UInt64:
            return true;
        default:
            return false;
    }
}

bool Type::isFloatingPoint() const noexcept {
    return m_kind == TypeKind::Float || m_kind == TypeKind::Double;
}

bool Type::equals(const Type& other) const {
    if (this == &other) return true;
    return m_kind == other.m_kind;
}

bool Type::canImplicitlyConvertTo(const Type& target) const {
    if (equals(target)) return true;
    if (m_kind == TypeKind::Error || target.getKind() == TypeKind::Error) return true;
    if (target.getKind() == TypeKind::Any) return true;

    // Numeric widening conversions
    switch (m_kind) {
        case TypeKind::Int8:
            return target.getKind() == TypeKind::Int16 ||
                   target.getKind() == TypeKind::Int32 ||
                   target.getKind() == TypeKind::Int64 ||
                   target.getKind() == TypeKind::Double ||
                   target.getKind() == TypeKind::Float;
        case TypeKind::UInt8:
            return target.getKind() == TypeKind::UInt16 ||
                   target.getKind() == TypeKind::UInt32 ||
                   target.getKind() == TypeKind::UInt64 ||
                   target.getKind() == TypeKind::Int16 ||
                   target.getKind() == TypeKind::Int32 ||
                   target.getKind() == TypeKind::Int64 ||
                   target.getKind() == TypeKind::Double ||
                   target.getKind() == TypeKind::Float;
        case TypeKind::Int16:
            return target.getKind() == TypeKind::Int32 ||
                   target.getKind() == TypeKind::Int64 ||
                   target.getKind() == TypeKind::Double ||
                   target.getKind() == TypeKind::Float;
        case TypeKind::UInt16:
            return target.getKind() == TypeKind::UInt32 ||
                   target.getKind() == TypeKind::UInt64 ||
                   target.getKind() == TypeKind::Int32 ||
                   target.getKind() == TypeKind::Int64 ||
                   target.getKind() == TypeKind::Double ||
                   target.getKind() == TypeKind::Float;
        case TypeKind::Int32:
            return target.getKind() == TypeKind::Int64 ||
                   target.getKind() == TypeKind::Double;
        case TypeKind::UInt32:
            return target.getKind() == TypeKind::UInt64 ||
                   target.getKind() == TypeKind::Int64 ||
                   target.getKind() == TypeKind::Double;
        case TypeKind::Int64:
        case TypeKind::UInt64:
            return target.getKind() == TypeKind::Double;
        case TypeKind::Float:
            return target.getKind() == TypeKind::Double;
        default:
            break;
    }

    return false;
}

bool Type::isSubtypeOf(const Type& target) const {
    if (equals(target)) return true;
    if (target.getKind() == TypeKind::Any) return true;
    if (m_kind == TypeKind::Error || target.getKind() == TypeKind::Error) return true;

    // T is a subtype of T?
    if (target.getKind() == TypeKind::Nullable) {
        const auto& nullableTarget = static_cast<const NullableType&>(target);
        if (m_kind == TypeKind::Null) return true;
        return isSubtypeOf(*nullableTarget.getUnderlyingType());
    }

    return false;
}

bool Type::isAssignableTo(const Type& target) const {
    if (equals(target)) return true;
    if (m_kind == TypeKind::Error || target.getKind() == TypeKind::Error) return true;
    if (target.getKind() == TypeKind::Any) return true;
    if (canImplicitlyConvertTo(target)) return true;
    if (isSubtypeOf(target)) return true;

    // Target is Nullable
    if (target.getKind() == TypeKind::Nullable) {
        if (m_kind == TypeKind::Null) return true;
        const auto& nullableTarget = static_cast<const NullableType&>(target);
        return isAssignableTo(*nullableTarget.getUnderlyingType());
    }

    // Target is Pointer
    if (m_kind == TypeKind::Null && target.getKind() == TypeKind::Pointer) return true;

    return false;
}

// ArrayType
std::string ArrayType::toString() const {
    return (m_elementType ? m_elementType->toString() : "unknown") + "[]";
}

bool ArrayType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Array) return false;
    const auto& arr = static_cast<const ArrayType&>(other);
    return m_elementType && arr.m_elementType && m_elementType->equals(*arr.m_elementType);
}

bool ArrayType::isAssignableTo(const Type& target) const {
    if (Type::isAssignableTo(target)) return true;
    if (target.getKind() == TypeKind::Array) {
        const auto& arr = static_cast<const ArrayType&>(target);
        return m_elementType && arr.m_elementType && m_elementType->equals(*arr.m_elementType);
    }
    return false;
}

// MapType
std::string MapType::toString() const {
    std::string k = m_keyType ? m_keyType->toString() : "unknown";
    std::string v = m_valueType ? m_valueType->toString() : "unknown";
    return "Map<" + k + ", " + v + ">";
}

bool MapType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Map) return false;
    const auto& map = static_cast<const MapType&>(other);
    return m_keyType && map.m_keyType && m_keyType->equals(*map.m_keyType) &&
           m_valueType && map.m_valueType && m_valueType->equals(*map.m_valueType);
}

bool MapType::isAssignableTo(const Type& target) const {
    if (Type::isAssignableTo(target)) return true;
    if (target.getKind() == TypeKind::Map) {
        const auto& map = static_cast<const MapType&>(target);
        return m_keyType && map.m_keyType && m_keyType->equals(*map.m_keyType) &&
               m_valueType && map.m_valueType && m_valueType->equals(*map.m_valueType);
    }
    return false;
}

// NullableType
std::string NullableType::toString() const {
    return (m_underlyingType ? m_underlyingType->toString() : "unknown") + "?";
}

bool NullableType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Nullable) return false;
    const auto& nullType = static_cast<const NullableType&>(other);
    return m_underlyingType && nullType.m_underlyingType && m_underlyingType->equals(*nullType.m_underlyingType);
}

bool NullableType::isAssignableTo(const Type& target) const {
    if (Type::isAssignableTo(target)) return true;
    if (target.getKind() == TypeKind::Nullable) {
        const auto& targetNull = static_cast<const NullableType&>(target);
        return m_underlyingType && targetNull.m_underlyingType && m_underlyingType->isAssignableTo(*targetNull.m_underlyingType);
    }
    return false;
}

// FunctionType
std::string FunctionType::toString() const {
    std::string res = "fn(";
    for (size_t i = 0; i < m_paramTypes.size(); ++i) {
        if (i > 0) res += ", ";
        res += m_paramTypes[i] ? m_paramTypes[i]->toString() : "unknown";
    }
    res += ") -> ";
    res += m_returnType ? m_returnType->toString() : "void";
    return res;
}

bool FunctionType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Function) return false;
    const auto& fn = static_cast<const FunctionType&>(other);
    if (m_paramTypes.size() != fn.m_paramTypes.size()) return false;
    for (size_t i = 0; i < m_paramTypes.size(); ++i) {
        if (!m_paramTypes[i]->equals(*fn.m_paramTypes[i])) return false;
    }
    return m_returnType->equals(*fn.m_returnType);
}

bool FunctionType::isAssignableTo(const Type& target) const {
    if (target.getKind() == TypeKind::Pointer) return true;
    return equals(target) || Type::isAssignableTo(target);
}

// PointerType
std::string PointerType::toString() const {
    return "ptr<" + (m_pointeeType ? m_pointeeType->toString() : "void") + ">";
}

bool PointerType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Pointer) return false;
    const auto& otherPtr = static_cast<const PointerType&>(other);
    if (!m_pointeeType || !otherPtr.m_pointeeType) return true;
    return m_pointeeType->equals(*otherPtr.m_pointeeType);
}

bool PointerType::isAssignableTo(const Type& target) const {
    if (target.isAny()) return true;
    if (target.getKind() != TypeKind::Pointer) return false;
    const auto& otherPtr = static_cast<const PointerType&>(target);
    if (!m_pointeeType || m_pointeeType->isVoid() ||
        !otherPtr.m_pointeeType || otherPtr.m_pointeeType->isVoid()) {
        return true;
    }
    return m_pointeeType->isAssignableTo(*otherPtr.m_pointeeType);
}

// ClassType
std::optional<MemberField> ClassType::findField(const std::string& name) const {
    auto it = m_fields.find(name);
    if (it != m_fields.end()) {
        return it->second;
    }
    if (m_superClass) {
        return m_superClass->findField(name);
    }
    return std::nullopt;
}

std::vector<MemberMethod> ClassType::findMethods(const std::string& name) const {
    std::vector<MemberMethod> res;
    for (const auto& method : m_methods) {
        if (method.name == name) {
            res.push_back(method);
        }
    }
    if (m_superClass) {
        auto superMethods = m_superClass->findMethods(name);
        for (auto& sm : superMethods) {
            bool overridden = false;
            for (const auto& rm : res) {
                if (rm.name == sm.name && rm.functionType->equals(*sm.functionType)) {
                    overridden = true;
                    break;
                }
            }
            if (!overridden) {
                res.push_back(std::move(sm));
            }
        }
    }
    return res;
}

bool ClassType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Class) return false;
    const auto& cls = static_cast<const ClassType&>(other);
    return m_name == cls.m_name;
}

bool ClassType::isSubtypeOf(const Type& target) const {
    if (equals(target)) return true;

    // Check superclass
    if (m_superClass && m_superClass->isSubtypeOf(target)) {
        return true;
    }

    // Check implemented interfaces
    for (const auto& iface : m_interfaces) {
        if (iface->isSubtypeOf(target)) {
            return true;
        }
    }

    return Type::isSubtypeOf(target);
}

bool ClassType::isAssignableTo(const Type& target) const {
    return isSubtypeOf(target) || Type::isAssignableTo(target);
}

// InterfaceType
std::vector<MemberMethod> InterfaceType::findMethods(const std::string& name) const {
    std::vector<MemberMethod> res;
    for (const auto& method : m_methods) {
        if (method.name == name) {
            res.push_back(method);
        }
    }
    for (const auto& parentIface : m_superInterfaces) {
        auto parentMethods = parentIface->findMethods(name);
        res.insert(res.end(), parentMethods.begin(), parentMethods.end());
    }
    return res;
}

bool InterfaceType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Interface) return false;
    const auto& iface = static_cast<const InterfaceType&>(other);
    return m_name == iface.m_name;
}

bool InterfaceType::isSubtypeOf(const Type& target) const {
    if (equals(target)) return true;
    for (const auto& parentIface : m_superInterfaces) {
        if (parentIface->isSubtypeOf(target)) {
            return true;
        }
    }
    return Type::isSubtypeOf(target);
}

bool InterfaceType::isAssignableTo(const Type& target) const {
    return isSubtypeOf(target) || Type::isAssignableTo(target);
}

// StructType
std::optional<MemberField> StructType::findField(const std::string& name) const {
    auto it = m_fields.find(name);
    if (it != m_fields.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool StructType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Struct) return false;
    const auto& st = static_cast<const StructType&>(other);
    return m_name == st.m_name;
}

// EnumType
bool EnumType::equals(const Type& other) const {
    if (other.getKind() != TypeKind::Enum) return false;
    const auto& en = static_cast<const EnumType&>(other);
    return m_name == en.m_name;
}

bool EnumType::isAssignableTo(const Type& target) const {
    if (equals(target)) return true;
    if (target.getKind() == TypeKind::Enum) {
        const auto& otherEnum = static_cast<const EnumType&>(target);
        // Result variance: Result<T, any> or Result<any, E> -> Result<T, E>
        if (m_name.rfind("Result<", 0) == 0 && otherEnum.m_name.rfind("Result<", 0) == 0) {
            auto thisArgs = TypeContext::splitTypeArgs(m_name.substr(7, m_name.size() - 8));
            auto otherArgs = TypeContext::splitTypeArgs(otherEnum.m_name.substr(7, otherEnum.m_name.size() - 8));
            if (thisArgs.size() == 2 && otherArgs.size() == 2) {
                bool tMatch = (thisArgs[0] == "any" || thisArgs[0] == otherArgs[0]);
                bool eMatch = (thisArgs[1] == "any" || thisArgs[1] == otherArgs[1]);
                if (tMatch && eMatch) return true;
            }
        }
        // Option variance: Option<any> -> Option<T>
        if (m_name.rfind("Option<", 0) == 0 && otherEnum.m_name.rfind("Option<", 0) == 0) {
            auto thisArgs = TypeContext::splitTypeArgs(m_name.substr(7, m_name.size() - 8));
            auto otherArgs = TypeContext::splitTypeArgs(otherEnum.m_name.substr(7, otherEnum.m_name.size() - 8));
            if (thisArgs.size() == 1 && otherArgs.size() == 1) {
                if (thisArgs[0] == "any" || thisArgs[0] == otherArgs[0]) return true;
            }
        }
    }
    if (!m_isTaggedUnion && target.isInteger()) return true;
    return Type::isAssignableTo(target);
}

// TypeContext implementation
TypeContext::TypeContext() {
    m_voidType = std::make_shared<PrimitiveType>(TypeKind::Void, "void");
    m_nullType = std::make_shared<PrimitiveType>(TypeKind::Null, "null");
    m_boolType = std::make_shared<PrimitiveType>(TypeKind::Bool, "bool");
    m_int8Type = std::make_shared<PrimitiveType>(TypeKind::Int8, "int8");
    m_uint8Type = std::make_shared<PrimitiveType>(TypeKind::UInt8, "uint8");
    m_int16Type = std::make_shared<PrimitiveType>(TypeKind::Int16, "int16");
    m_uint16Type = std::make_shared<PrimitiveType>(TypeKind::UInt16, "uint16");
    m_int32Type = std::make_shared<PrimitiveType>(TypeKind::Int32, "int");
    m_uint32Type = std::make_shared<PrimitiveType>(TypeKind::UInt32, "uint");
    m_int64Type = std::make_shared<PrimitiveType>(TypeKind::Int64, "int64");
    m_uint64Type = std::make_shared<PrimitiveType>(TypeKind::UInt64, "uint64");
    m_floatType = std::make_shared<PrimitiveType>(TypeKind::Float, "float");
    m_doubleType = std::make_shared<PrimitiveType>(TypeKind::Double, "double");
    m_charType = std::make_shared<PrimitiveType>(TypeKind::Char, "char");
    m_stringType = std::make_shared<PrimitiveType>(TypeKind::String, "string");
    m_anyType = std::make_shared<PrimitiveType>(TypeKind::Any, "any");
    m_unknownType = std::make_shared<PrimitiveType>(TypeKind::Unknown, "unknown");
    m_errorType = std::make_shared<PrimitiveType>(TypeKind::Error, "<type-error>");

    auto ioCls = createClassType("IO");
    ioCls->addMethod({"println", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    ioCls->addMethod({"print", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    ioCls->addMethod({"eprintln", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    ioCls->addMethod({"eprint", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    ioCls->addMethod({"readLine", getFunctionType({}, m_stringType), "public", true, false, false});
    ioCls->addMethod({"readln", getFunctionType({}, m_stringType), "public", true, false, false});
    ioCls->addMethod({"flush", getFunctionType({}, m_voidType), "public", true, false, false});
    ioCls->addMethod({"eflush", getFunctionType({}, m_voidType), "public", true, false, false});

    auto consoleCls = createClassType("Console");
    consoleCls->addMethod({"println", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    consoleCls->addMethod({"print", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    consoleCls->addMethod({"eprintln", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    consoleCls->addMethod({"eprint", getFunctionType({m_stringType}, m_voidType), "public", true, false, false});
    consoleCls->addMethod({"readLine", getFunctionType({}, m_stringType), "public", true, false, false});
    consoleCls->addMethod({"readln", getFunctionType({}, m_stringType), "public", true, false, false});
    consoleCls->addMethod({"flush", getFunctionType({}, m_voidType), "public", true, false, false});
    consoleCls->addMethod({"eflush", getFunctionType({}, m_voidType), "public", true, false, false});

    auto strCls = createClassType("String");
    strCls->addMethod({"length", getFunctionType({m_stringType}, m_int64Type), "public", true, false, false});
    strCls->addMethod({"upper", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});
    strCls->addMethod({"lower", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});
    strCls->addMethod({"trim", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});
    strCls->addMethod({"contains", getFunctionType({m_stringType, m_stringType}, m_boolType), "public", true, false, false});
    strCls->addMethod({"indexOf", getFunctionType({m_stringType, m_stringType}, m_int64Type), "public", true, false, false});
    strCls->addMethod({"replace", getFunctionType({m_stringType, m_stringType, m_stringType}, m_stringType), "public", true, false, false});
    strCls->addMethod({"substring", getFunctionType({m_stringType, m_int64Type, m_int64Type}, m_stringType), "public", true, false, false});
    strCls->addMethod({"split", getFunctionType({m_stringType, m_stringType}, getArrayType(m_stringType)), "public", true, false, false});
    strCls->addMethod({"reverse", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});

    auto mathCls = createClassType("Math");
    mathCls->addField({"PI", m_doubleType, "public", true, true});
    mathCls->addField({"E", m_doubleType, "public", true, true});
    mathCls->addField({"TAU", m_doubleType, "public", true, true});
    mathCls->addMethod({"sqrt", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"pow", getFunctionType({m_doubleType, m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"sin", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"cos", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"tan", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"asin", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"acos", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"atan", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"atan2", getFunctionType({m_doubleType, m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"floor", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"ceil", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"round", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"abs", getFunctionType({m_int32Type}, m_int32Type), "public", true, false, false});
    mathCls->addMethod({"abs", getFunctionType({m_int64Type}, m_int64Type), "public", true, false, false});
    mathCls->addMethod({"abs", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"min", getFunctionType({m_int32Type, m_int32Type}, m_int32Type), "public", true, false, false});
    mathCls->addMethod({"min", getFunctionType({m_int64Type, m_int64Type}, m_int64Type), "public", true, false, false});
    mathCls->addMethod({"min", getFunctionType({m_doubleType, m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"max", getFunctionType({m_int32Type, m_int32Type}, m_int32Type), "public", true, false, false});
    mathCls->addMethod({"max", getFunctionType({m_int64Type, m_int64Type}, m_int64Type), "public", true, false, false});
    mathCls->addMethod({"max", getFunctionType({m_doubleType, m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"log", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"log10", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"log2", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});
    mathCls->addMethod({"exp", getFunctionType({m_doubleType}, m_doubleType), "public", true, false, false});

    auto fsCls = createClassType("FS");
    fsCls->addMethod({"readText", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});
    fsCls->addMethod({"writeText", getFunctionType({m_stringType, m_stringType}, m_boolType), "public", true, false, false});
    fsCls->addMethod({"appendText", getFunctionType({m_stringType, m_stringType}, m_boolType), "public", true, false, false});
    fsCls->addMethod({"exists", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fsCls->addMethod({"delete", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fsCls->addMethod({"deleteFile", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fsCls->addMethod({"createDir", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fsCls->addMethod({"listDir", getFunctionType({m_stringType}, getArrayType(m_stringType)), "public", true, false, false});
    fsCls->addMethod({"isFile", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fsCls->addMethod({"isDir", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});

    auto fileCls = createClassType("File");
    fileCls->addMethod({"readText", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});
    fileCls->addMethod({"writeText", getFunctionType({m_stringType, m_stringType}, m_boolType), "public", true, false, false});
    fileCls->addMethod({"appendText", getFunctionType({m_stringType, m_stringType}, m_boolType), "public", true, false, false});
    fileCls->addMethod({"exists", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fileCls->addMethod({"delete", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fileCls->addMethod({"deleteFile", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});
    fileCls->addMethod({"isFile", getFunctionType({m_stringType}, m_boolType), "public", true, false, false});

    auto timeCls = createClassType("Time");
    timeCls->addMethod({"now", getFunctionType({}, m_int32Type), "public", true, false, false});
    timeCls->addMethod({"nowSeconds", getFunctionType({}, m_int32Type), "public", true, false, false});
    timeCls->addMethod({"nowNanos", getFunctionType({}, m_int64Type), "public", true, false, false});
    timeCls->addMethod({"sleep", getFunctionType({m_int32Type}, m_voidType), "public", true, false, false});
    timeCls->addMethod({"sleep", getFunctionType({m_int64Type}, m_voidType), "public", true, false, false});
    timeCls->addMethod({"sleepSeconds", getFunctionType({m_int32Type}, m_voidType), "public", true, false, false});
    timeCls->addMethod({"sleepSeconds", getFunctionType({m_int64Type}, m_voidType), "public", true, false, false});
    timeCls->addMethod({"format", getFunctionType({m_int32Type}, m_stringType), "public", true, false, false});
    timeCls->addMethod({"format", getFunctionType({m_int64Type}, m_stringType), "public", true, false, false});
    timeCls->addMethod({"parse", getFunctionType({m_stringType}, m_int32Type), "public", true, false, false});

    auto durationCls = createClassType("Duration");
    durationCls->addField({"millis", m_int32Type, "public", false, false});

    // Async (Phase 2 slice B2): Task<T> is the awaitable type. It is an
    // opaque handle at runtime (a pointer to the runtime Task); the type
    // parameter exists so sema can unwrap `await e` (e: Task<T>) to T
    // (see TypeChecker::visit(AwaitExpr)). Tasks are created by the
    // sleep_async / delayed_value builtins and by calling an `async fn`.
    // Phase 5 slice A adds the cancellation API: cancel() marks the task
    // done+cancelled (a body that has not started yet never runs; a body
    // suspended at an await stops at its next resume), isReady() reports
    // completion, wasCancelled() reports cancellation.
    auto taskCls = createClassType("Task");
    taskCls->setTypeParams({"T"});
    taskCls->addMethod({"cancel", getFunctionType({}, m_boolType), "public", false, false, false});
    taskCls->addMethod({"isReady", getFunctionType({}, m_boolType), "public", false, false, false});
    taskCls->addMethod({"wasCancelled", getFunctionType({}, m_boolType), "public", false, false, false});
    durationCls->addMethod({"fromMillis", getFunctionType({m_int32Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"fromMillis", getFunctionType({m_int64Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"fromSeconds", getFunctionType({m_int32Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"fromSeconds", getFunctionType({m_int64Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"fromMinutes", getFunctionType({m_int32Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"fromMinutes", getFunctionType({m_int64Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"fromHours", getFunctionType({m_int32Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"fromHours", getFunctionType({m_int64Type}, durationCls), "public", true, false, false});
    durationCls->addMethod({"toMillis", getFunctionType({}, m_int32Type), "public", false, false, false});
    durationCls->addMethod({"toSeconds", getFunctionType({}, m_int32Type), "public", false, false, false});

    auto osCls = createClassType("OS");
    osCls->addMethod({"getEnv", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});
    osCls->addMethod({"setEnv", getFunctionType({m_stringType, m_stringType}, m_boolType), "public", true, false, false});
    osCls->addMethod({"platform", getFunctionType({}, m_stringType), "public", true, false, false});
    osCls->addMethod({"arch", getFunctionType({}, m_stringType), "public", true, false, false});
    osCls->addMethod({"args", getFunctionType({}, getArrayType(m_stringType)), "public", true, false, false});
    osCls->addMethod({"exit", getFunctionType({m_int32Type}, m_voidType), "public", true, false, false});
    osCls->addMethod({"exit", getFunctionType({m_int64Type}, m_voidType), "public", true, false, false});

    auto procCls = createClassType("Process");
    procCls->addMethod({"exec", getFunctionType({m_stringType}, m_int32Type), "public", true, false, false});
    procCls->addMethod({"spawn", getFunctionType({m_stringType}, m_int32Type), "public", true, false, false});
    procCls->addMethod({"run", getFunctionType({m_stringType}, m_stringType), "public", true, false, false});
    procCls->addMethod({"pid", getFunctionType({}, m_int32Type), "public", true, false, false});

    auto arenaCls = createClassType("Arena");
    arenaCls->addField({"handle", m_int64Type, "private", false, false});
    arenaCls->addMethod({"alloc", getFunctionType({m_int32Type}, m_int32Type), "public", false, false, false});
    arenaCls->addMethod({"alloc", getFunctionType({m_int64Type}, m_int64Type), "public", false, false, false});
    arenaCls->addMethod({"allocAligned", getFunctionType({m_int32Type, m_int32Type}, m_int32Type), "public", false, false, false});
    arenaCls->addMethod({"allocAligned", getFunctionType({m_int64Type, m_int64Type}, m_int64Type), "public", false, false, false});
    arenaCls->addMethod({"reset", getFunctionType({}, m_voidType), "public", false, false, false});
    arenaCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});
    arenaCls->addMethod({"allocatedBytes", getFunctionType({}, m_int32Type), "public", false, false, false});

    auto memCls = createClassType("Mem");
    memCls->addMethod({"totalAllocatedBytes", getFunctionType({}, m_int32Type), "public", true, false, false});
    memCls->addMethod({"totalLiveObjects", getFunctionType({}, m_int32Type), "public", true, false, false});

    // Built-in Result<T, E> template enum
    auto resultEnum = createEnumType("Result");
    resultEnum->setTypeParams({"T", "E"});
    resultEnum->setTaggedUnion(true);
    resultEnum->addVariant("Ok", 0, {"T"}, {m_anyType});
    resultEnum->addVariant("Err", 1, {"E"}, {m_anyType});

    // Built-in Option<T> template enum
    auto optionEnum = createEnumType("Option");
    optionEnum->setTypeParams({"T"});
    optionEnum->setTaggedUnion(true);
    optionEnum->addVariant("Some", 0, {"T"}, {m_anyType});
    optionEnum->addVariant("None", 1, {}, {});

    // Built-in IOError enum
    auto ioErrorEnum = createEnumType("IOError");
    ioErrorEnum->addVariant("NotFound", 0);
    ioErrorEnum->addVariant("PermissionDenied", 1);
    ioErrorEnum->addVariant("AlreadyExists", 2);
    ioErrorEnum->addVariant("InvalidInput", 3);
    ioErrorEnum->addVariant("UnexpectedEOF", 4);
    ioErrorEnum->addVariant("Other", 5);

    auto resultStrIO = specializeEnumType(resultEnum, {"string", "IOError"});
    auto resultBoolIO = specializeEnumType(resultEnum, {"bool", "IOError"});
    auto resultStrArrIO = specializeEnumType(resultEnum, {"string[]", "IOError"});
    if (resultStrIO && resultBoolIO) {
        fsCls->addMethod({"tryReadText", getFunctionType({m_stringType}, resultStrIO), "public", true, false, false});
        fsCls->addMethod({"tryWriteText", getFunctionType({m_stringType, m_stringType}, resultBoolIO), "public", true, false, false});
        fsCls->addMethod({"tryAppendText", getFunctionType({m_stringType, m_stringType}, resultBoolIO), "public", true, false, false});
        fsCls->addMethod({"tryDelete", getFunctionType({m_stringType}, resultBoolIO), "public", true, false, false});
        fsCls->addMethod({"tryCreateDir", getFunctionType({m_stringType}, resultBoolIO), "public", true, false, false});
        // Phase 38 slice E: tryListDir (FS only — File has no listDir).
        if (resultStrArrIO) {
            fsCls->addMethod({"tryListDir", getFunctionType({m_stringType}, resultStrArrIO), "public", true, false, false});
        }

        fileCls->addMethod({"tryReadText", getFunctionType({m_stringType}, resultStrIO), "public", true, false, false});
        fileCls->addMethod({"tryWriteText", getFunctionType({m_stringType, m_stringType}, resultBoolIO), "public", true, false, false});
        fileCls->addMethod({"tryAppendText", getFunctionType({m_stringType, m_stringType}, resultBoolIO), "public", true, false, false});
        fileCls->addMethod({"tryDelete", getFunctionType({m_stringType}, resultBoolIO), "public", true, false, false});

        ioCls->addMethod({"tryReadLine", getFunctionType({}, resultStrIO), "public", true, false, false});
    }

    // ========================================================================
    // Phase 15: Concurrency and Synchronization Types (std.sync)
    // ========================================================================
    auto threadCls = createClassType("Thread");
    threadCls->addField({"handle", m_int64Type, "public", false, false});
    threadCls->addMethod({"spawn", getFunctionType({m_int64Type, m_int64Type}, threadCls), "public", true, false, false});
    threadCls->addMethod({"spawn", getFunctionType({m_int64Type}, threadCls), "public", true, false, false});
    threadCls->addMethod({"id", getFunctionType({}, m_int64Type), "public", true, false, false});
    threadCls->addMethod({"hardwareConcurrency", getFunctionType({}, m_int64Type), "public", true, false, false});
    threadCls->addMethod({"yield", getFunctionType({}, m_voidType), "public", true, false, false});
    threadCls->addMethod({"sleep", getFunctionType({m_int64Type}, m_voidType), "public", true, false, false});
    threadCls->addMethod({"sleep", getFunctionType({m_int32Type}, m_voidType), "public", true, false, false});
    threadCls->addMethod({"join", getFunctionType({}, m_voidType), "public", false, false, false});
    threadCls->addMethod({"detach", getFunctionType({}, m_voidType), "public", false, false, false});

    auto mutexCls = createClassType("Mutex");
    mutexCls->addField({"handle", m_int64Type, "public", false, false});
    mutexCls->addMethod({"lock", getFunctionType({}, m_voidType), "public", false, false, false});
    mutexCls->addMethod({"tryLock", getFunctionType({}, m_boolType), "public", false, false, false});
    mutexCls->addMethod({"unlock", getFunctionType({}, m_voidType), "public", false, false, false});
    mutexCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});

    auto rwlockCls = createClassType("RwLock");
    rwlockCls->addField({"handle", m_int64Type, "public", false, false});
    rwlockCls->addMethod({"readLock", getFunctionType({}, m_voidType), "public", false, false, false});
    rwlockCls->addMethod({"tryReadLock", getFunctionType({}, m_boolType), "public", false, false, false});
    rwlockCls->addMethod({"readUnlock", getFunctionType({}, m_voidType), "public", false, false, false});
    rwlockCls->addMethod({"writeLock", getFunctionType({}, m_voidType), "public", false, false, false});
    rwlockCls->addMethod({"tryWriteLock", getFunctionType({}, m_boolType), "public", false, false, false});
    rwlockCls->addMethod({"writeUnlock", getFunctionType({}, m_voidType), "public", false, false, false});
    rwlockCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});

    auto semCls = createClassType("Semaphore");
    semCls->addField({"handle", m_int64Type, "public", false, false});
    semCls->addMethod({"acquire", getFunctionType({}, m_voidType), "public", false, false, false});
    semCls->addMethod({"tryAcquire", getFunctionType({}, m_boolType), "public", false, false, false});
    semCls->addMethod({"release", getFunctionType({}, m_voidType), "public", false, false, false});
    semCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});

    auto cvCls = createClassType("ConditionVariable");
    cvCls->addField({"handle", m_int64Type, "public", false, false});
    cvCls->addMethod({"wait", getFunctionType({mutexCls}, m_voidType), "public", false, false, false});
    cvCls->addMethod({"waitTimeout", getFunctionType({mutexCls, m_int64Type}, m_boolType), "public", false, false, false});
    cvCls->addMethod({"waitTimeout", getFunctionType({mutexCls, m_int32Type}, m_boolType), "public", false, false, false});
    cvCls->addMethod({"notifyOne", getFunctionType({}, m_voidType), "public", false, false, false});
    cvCls->addMethod({"notifyAll", getFunctionType({}, m_voidType), "public", false, false, false});
    cvCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});

    auto atomicIntCls = createClassType("AtomicInt");
    atomicIntCls->addField({"handle", m_int64Type, "public", false, false});
    atomicIntCls->addMethod({"get", getFunctionType({}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"load", getFunctionType({}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"set", getFunctionType({m_int64Type}, m_voidType), "public", false, false, false});
    atomicIntCls->addMethod({"set", getFunctionType({m_int32Type}, m_voidType), "public", false, false, false});
    atomicIntCls->addMethod({"store", getFunctionType({m_int64Type}, m_voidType), "public", false, false, false});
    atomicIntCls->addMethod({"store", getFunctionType({m_int32Type}, m_voidType), "public", false, false, false});
    atomicIntCls->addMethod({"add", getFunctionType({m_int64Type}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"add", getFunctionType({m_int32Type}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"sub", getFunctionType({m_int64Type}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"sub", getFunctionType({m_int32Type}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"increment", getFunctionType({}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"decrement", getFunctionType({}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"exchange", getFunctionType({m_int64Type}, m_int64Type), "public", false, false, false});
    atomicIntCls->addMethod({"compareAndSwap", getFunctionType({m_int64Type, m_int64Type}, m_boolType), "public", false, false, false});
    atomicIntCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});

    auto atomicBoolCls = createClassType("AtomicBool");
    atomicBoolCls->addField({"handle", m_int64Type, "public", false, false});
    atomicBoolCls->addMethod({"get", getFunctionType({}, m_boolType), "public", false, false, false});
    atomicBoolCls->addMethod({"load", getFunctionType({}, m_boolType), "public", false, false, false});
    atomicBoolCls->addMethod({"set", getFunctionType({m_boolType}, m_voidType), "public", false, false, false});
    atomicBoolCls->addMethod({"store", getFunctionType({m_boolType}, m_voidType), "public", false, false, false});
    atomicBoolCls->addMethod({"exchange", getFunctionType({m_boolType}, m_boolType), "public", false, false, false});
    atomicBoolCls->addMethod({"compareAndSwap", getFunctionType({m_boolType, m_boolType}, m_boolType), "public", false, false, false});
    atomicBoolCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});

    auto channelCls = createClassType("Channel");
    channelCls->setTypeParams({"T"});
    channelCls->addField({"handle", m_int64Type, "public", false, false});
    channelCls->addMethod({"send", getFunctionType({m_int64Type}, m_boolType), "public", false, false, false});
    channelCls->addMethod({"send", getFunctionType({m_anyType}, m_boolType), "public", false, false, false});
    channelCls->addMethod({"recv", getFunctionType({}, m_int64Type), "public", false, false, false});
    channelCls->addMethod({"trySend", getFunctionType({m_int64Type}, m_boolType), "public", false, false, false});
    channelCls->addMethod({"trySend", getFunctionType({m_anyType}, m_boolType), "public", false, false, false});
    channelCls->addMethod({"close", getFunctionType({}, m_voidType), "public", false, false, false});
    channelCls->addMethod({"isClosed", getFunctionType({}, m_boolType), "public", false, false, false});
    channelCls->addMethod({"size", getFunctionType({}, m_int64Type), "public", false, false, false});
    channelCls->addMethod({"capacity", getFunctionType({}, m_int64Type), "public", false, false, false});
    channelCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});

    auto poolCls = createClassType("ThreadPool");
    poolCls->addField({"handle", m_int64Type, "public", false, false});
    poolCls->addMethod({"submit", getFunctionType({m_int64Type, m_int64Type}, m_voidType), "public", false, false, false});
    poolCls->addMethod({"waitIdle", getFunctionType({}, m_voidType), "public", false, false, false});
    poolCls->addMethod({"threadCount", getFunctionType({}, m_int64Type), "public", false, false, false});
    poolCls->addMethod({"destroy", getFunctionType({}, m_voidType), "public", false, false, false});
}

TypePtr TypeContext::getArrayType(TypePtr elementType) {
    if (!elementType) return m_errorType;
    std::string key = elementType->toString() + "[]";
    auto it = m_cachedTypes.find(key);
    if (it != m_cachedTypes.end()) {
        return it->second;
    }
    auto arr = std::make_shared<ArrayType>(std::move(elementType));
    m_cachedTypes[key] = arr;
    return arr;
}

TypePtr TypeContext::getMapType(TypePtr keyType, TypePtr valueType) {
    if (!keyType || !valueType) return m_errorType;
    std::string key = "Map<" + keyType->toString() + "," + valueType->toString() + ">";
    auto it = m_cachedTypes.find(key);
    if (it != m_cachedTypes.end()) {
        return it->second;
    }
    auto map = std::make_shared<MapType>(std::move(keyType), std::move(valueType));
    m_cachedTypes[key] = map;
    return map;
}

TypePtr TypeContext::getNullableType(TypePtr underlyingType) {
    if (!underlyingType) return m_errorType;
    if (underlyingType->isNullable() || underlyingType->isNull() || underlyingType->isVoid()) {
        return underlyingType;
    }
    std::string key = underlyingType->toString() + "?";
    auto it = m_cachedTypes.find(key);
    if (it != m_cachedTypes.end()) {
        return it->second;
    }
    auto nullable = std::make_shared<NullableType>(std::move(underlyingType));
    m_cachedTypes[key] = nullable;
    return nullable;
}

TypePtr TypeContext::getPointerType(TypePtr pointeeType) {
    if (!pointeeType) pointeeType = m_voidType;
    std::string key = "ptr<" + pointeeType->toString() + ">";
    auto it = m_cachedTypes.find(key);
    if (it != m_cachedTypes.end()) {
        return it->second;
    }
    auto ptrType = std::make_shared<PointerType>(std::move(pointeeType));
    m_cachedTypes[key] = ptrType;
    return ptrType;
}

std::shared_ptr<FunctionType> TypeContext::getFunctionType(std::vector<TypePtr> paramTypes, TypePtr returnType) {
    if (!returnType) returnType = m_voidType;
    return std::make_shared<FunctionType>(std::move(paramTypes), std::move(returnType));
}

std::shared_ptr<ClassType> TypeContext::createClassType(const std::string& name) {
    auto cls = std::make_shared<ClassType>(name);
    m_userTypes[name] = cls;
    return cls;
}

std::shared_ptr<InterfaceType> TypeContext::createInterfaceType(const std::string& name) {
    auto iface = std::make_shared<InterfaceType>(name);
    m_userTypes[name] = iface;
    return iface;
}

std::shared_ptr<StructType> TypeContext::createStructType(const std::string& name) {
    auto st = std::make_shared<StructType>(name);
    m_userTypes[name] = st;
    return st;
}

std::shared_ptr<EnumType> TypeContext::createEnumType(const std::string& name) {
    auto en = std::make_shared<EnumType>(name);
    m_userTypes[name] = en;
    return en;
}

void TypeContext::registerUserType(const std::string& name, TypePtr type) {
    m_userTypes[name] = std::move(type);
}

TypePtr TypeContext::findUserType(const std::string& name) const {
    auto it = m_userTypes.find(name);
    if (it != m_userTypes.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> TypeContext::splitTypeArgs(const std::string& inner) {
    std::vector<std::string> args;
    std::string current;
    int depth = 0;
    for (char c : inner) {
        if (c == '<') {
            depth++;
            current += c;
        } else if (c == '>') {
            depth--;
            current += c;
        } else if (c == ',' && depth == 0) {
            size_t f = current.find_first_not_of(" \t\n\r");
            size_t l = current.find_last_not_of(" \t\n\r");
            if (f != std::string::npos) {
                args.push_back(current.substr(f, l - f + 1));
            }
            current.clear();
        } else {
            current += c;
        }
    }
    size_t f = current.find_first_not_of(" \t\n\r");
    size_t l = current.find_last_not_of(" \t\n\r");
    if (f != std::string::npos) {
        args.push_back(current.substr(f, l - f + 1));
    }
    return args;
}

std::string TypeContext::substituteType(const std::string& typeStr,
                                       const std::vector<std::string>& typeParams,
                                       const std::vector<std::string>& typeArgs) {
    std::string result = typeStr;
    for (size_t i = 0; i < typeParams.size() && i < typeArgs.size(); ++i) {
        const std::string& param = typeParams[i];
        const std::string& arg = typeArgs[i];

        if (result == param) {
            return arg;
        }
        if (result == param + "[]") {
            return arg + "[]";
        }
        if (result == param + "?") {
            return arg + "?";
        }
        size_t pos = 0;
        while ((pos = result.find(param, pos)) != std::string::npos) {
            bool leftOk = (pos == 0 || (!isalnum(result[pos - 1]) && result[pos - 1] != '_'));
            bool rightOk = (pos + param.size() >= result.size() || (!isalnum(result[pos + param.size()]) && result[pos + param.size()] != '_'));
            if (leftOk && rightOk) {
                result.replace(pos, param.size(), arg);
                pos += arg.size();
            } else {
                pos += param.size();
            }
        }
    }
    return result;
}

std::shared_ptr<ClassType> TypeContext::specializeClassType(
    const std::shared_ptr<ClassType>& baseCls,
    const std::vector<std::string>& typeArgs) {

    std::string specName = baseCls->getName() + "<";
    for (size_t i = 0; i < typeArgs.size(); ++i) {
        if (i > 0) specName += ", ";
        specName += typeArgs[i];
    }
    specName += ">";

    auto existing = findUserType(specName);
    if (existing && existing->isClass()) {
        return std::static_pointer_cast<ClassType>(existing);
    }

    auto specCls = createClassType(specName);
    specCls->setAbstract(baseCls->isAbstract());
    specCls->setASTDecl(baseCls->getASTDecl());

    const auto& typeParams = baseCls->getTypeParams();

    if (baseCls->getASTDecl()) {
        for (const auto& member : baseCls->getASTDecl()->getMembers()) {
            if (!member) continue;
            if (member->getKind() == ast::ASTNodeKind::VarDeclStmt) {
                const auto& varDecl = static_cast<const ast::VarDeclStmt&>(*member);
                std::string specFieldTypeStr = substituteType(varDecl.getType(), typeParams, typeArgs);
                auto fType = resolveType(specFieldTypeStr);
                if (!fType) fType = getAny();
                specCls->addField({
                    .name = varDecl.getName(),
                    .type = fType,
                    .visibility = varDecl.getVisibility().empty() ? "public" : varDecl.getVisibility(),
                    .isStatic = varDecl.isStatic(),
                    .isConst = varDecl.isConst()
                });
            } else if (member->getKind() == ast::ASTNodeKind::FunctionDecl) {
                const auto& fnDecl = static_cast<const ast::FunctionDecl&>(*member);
                std::vector<TypePtr> paramTypes;
                for (const auto& param : fnDecl.getParams()) {
                    std::string specParamTypeStr = substituteType(param.getType(), typeParams, typeArgs);
                    auto pType = resolveType(specParamTypeStr);
                    paramTypes.push_back(pType ? pType : getAny());
                }
                std::string specRetTypeStr = substituteType(fnDecl.getReturnType(), typeParams, typeArgs);
                auto rType = resolveType(specRetTypeStr);
                if (!rType) rType = getVoid();
                auto fnType = getFunctionType(std::move(paramTypes), std::move(rType));
                specCls->addMethod({
                    .name = fnDecl.getName(),
                    .functionType = fnType,
                    .visibility = fnDecl.getVisibility().empty() ? "public" : fnDecl.getVisibility(),
                    .isStatic = fnDecl.isStatic(),
                    .isAbstract = fnDecl.isAbstract(),
                    .isOverride = fnDecl.isOverride()
                });
            }
        }
    } else {
        for (const auto& [fname, field] : baseCls->getFields()) {
            std::string substitutedTypeStr = substituteType(field.type->toString(), typeParams, typeArgs);
            auto fType = resolveType(substitutedTypeStr);
            specCls->addField({
                .name = fname,
                .type = fType ? fType : field.type,
                .visibility = field.visibility,
                .isStatic = field.isStatic,
                .isConst = field.isConst
            });
        }
        for (const auto& method : baseCls->getMethods()) {
            specCls->addMethod(method);
        }
    }

    return specCls;
}

std::shared_ptr<EnumType> TypeContext::specializeEnumType(
    const std::shared_ptr<EnumType>& baseEnum,
    const std::vector<std::string>& typeArgs) {

    std::string specName = baseEnum->getName() + "<";
    for (size_t i = 0; i < typeArgs.size(); ++i) {
        if (i > 0) specName += ", ";
        specName += typeArgs[i];
    }
    specName += ">";

    auto existing = findUserType(specName);
    if (existing && existing->isEnum()) {
        return std::static_pointer_cast<EnumType>(existing);
    }

    auto specEnum = createEnumType(specName);
    specEnum->setTaggedUnion(baseEnum->isTaggedUnion());
    specEnum->setASTDecl(baseEnum->getASTDecl());

    const auto& typeParams = baseEnum->getTypeParams();

    if (baseEnum->getASTDecl()) {
        for (const auto& variant : baseEnum->getASTDecl()->getVariants()) {
            std::vector<std::string> specPayloadNames;
            std::vector<TypePtr> specPayloadTypes;
            for (const auto& pTypeStr : variant.getPayloadTypes()) {
                std::string specStr = substituteType(pTypeStr, typeParams, typeArgs);
                specPayloadNames.push_back(specStr);
                auto pType = resolveType(specStr);
                specPayloadTypes.push_back(pType ? pType : getAny());
            }
            specEnum->addVariant(variant.getName(), std::nullopt, specPayloadNames, specPayloadTypes);
        }
    } else {
        for (const auto& vName : baseEnum->getVariants()) {
            const auto& pNames = baseEnum->getVariantPayloadNames(vName);
            std::vector<std::string> specPayloadNames;
            std::vector<TypePtr> specPayloadTypes;
            for (const auto& pTypeStr : pNames) {
                std::string specStr = substituteType(pTypeStr, typeParams, typeArgs);
                specPayloadNames.push_back(specStr);
                auto pType = resolveType(specStr);
                specPayloadTypes.push_back(pType ? pType : getAny());
            }
            specEnum->addVariant(vName, baseEnum->getVariantValue(vName), specPayloadNames, specPayloadTypes);
        }
    }

    return specEnum;
}

TypePtr TypeContext::resolveType(const std::string& rawTypeStr) {
    // Strip leading and trailing whitespace
    size_t first = rawTypeStr.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return m_unknownType;
    size_t last = rawTypeStr.find_last_not_of(" \t\n\r");
    std::string s = rawTypeStr.substr(first, (last - first + 1));

    if (s.empty()) return m_unknownType;

    // Check raw pointer types: ptr, ptr<T>, *T
    if (s == "ptr") {
        return getPointerType(m_voidType);
    }
    if (s.rfind("ptr<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        auto elem = resolveType(inner);
        return getPointerType(elem);
    }
    if (s.front() == '*' && s.size() > 1) {
        auto elem = resolveType(s.substr(1));
        return getPointerType(elem);
    }

    // Check nullable suffix '?'
    if (s.back() == '?') {
        std::string inner = s.substr(0, s.size() - 1);
        auto base = resolveType(inner);
        return getNullableType(base);
    }

    // Check array suffix '[]'
    if (s.size() > 2 && s.substr(s.size() - 2) == "[]") {
        std::string inner = s.substr(0, s.size() - 2);
        auto elem = resolveType(inner);
        return getArrayType(elem);
    }

    // Check Array<T>
    if (s.rfind("Array<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        auto elem = resolveType(inner);
        return getArrayType(elem);
    }

    // Check List<T>
    if (s.rfind("List<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        auto elem = resolveType(inner);
        std::string specName = "List<" + (elem ? elem->toString() : "any") + ">";
        auto existing = findUserType(specName);
        if (existing && existing->isClass()) {
            return existing;
        }
        auto listCls = createClassType(specName);
        listCls->addMethod({"push", getFunctionType({elem}, m_voidType), "public", false, false, false});
        listCls->addMethod({"add", getFunctionType({elem}, m_voidType), "public", false, false, false});
        listCls->addMethod({"pop", getFunctionType({}, elem), "public", false, false, false});
        listCls->addMethod({"get", getFunctionType({m_int64Type}, elem), "public", false, false, false});
        listCls->addMethod({"set", getFunctionType({m_int64Type, elem}, m_voidType), "public", false, false, false});
        listCls->addMethod({"size", getFunctionType({}, m_int32Type), "public", false, false, false});
        listCls->addMethod({"length", getFunctionType({}, m_int32Type), "public", false, false, false});
        listCls->addMethod({"isEmpty", getFunctionType({}, m_boolType), "public", false, false, false});
        listCls->addMethod({"contains", getFunctionType({elem ? elem : m_int64Type}, m_boolType), "public", false, false, false});
        listCls->addMethod({"clear", getFunctionType({}, m_voidType), "public", false, false, false});
        listCls->addMethod({"removeAt", getFunctionType({m_int64Type}, m_voidType), "public", false, false, false});
        listCls->addMethod({"map", getFunctionType({getAny()}, listCls), "public", false, false, false});
        listCls->addMethod({"filter", getFunctionType({getAny()}, listCls), "public", false, false, false});
        listCls->addMethod({"reduce", getFunctionType({getAny(), elem ? elem : m_int64Type}, elem ? elem : m_int64Type), "public", false, false, false});
        // Phase 6 slice B: the 1-argument form reduce(fn) was rejected
        // by sema ("No matching overload") even though IRBuilder has
        // always lowered it (initial accumulator = the type's zero
        // value: 0 for numbers, "" for strings via null-as-empty in
        // sp_rt_str_concat). Register it as a real overload so the two
        // paths agree. The accumulator must be the element type; a
        // differently-typed accumulator (a true generic fold) is still
        // unsupported.
        listCls->addMethod({"reduce", getFunctionType({getAny()}, elem ? elem : m_int64Type), "public", false, false, false});
        return listCls;
    }

    // Check Map<K, V>
    if (s.rfind("Map<", 0) == 0 && s.back() == '>') {
        size_t comma = s.find(',');
        if (comma != std::string::npos) {
            std::string kStr = s.substr(4, comma - 4);
            std::string vStr = s.substr(comma + 1, s.size() - comma - 2);
            return getMapType(resolveType(kStr), resolveType(vStr));
        }
    }

    // Check Set<T>
    if (s.rfind("Set<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        auto elem = resolveType(inner);
        std::string specName = "Set<" + (elem ? elem->toString() : "any") + ">";
        auto existing = findUserType(specName);
        if (existing && existing->isClass()) {
            return existing;
        }
        auto setCls = createClassType(specName);
        setCls->addMethod({"add", getFunctionType({elem ? elem : m_int64Type}, m_voidType), "public", false, false, false});
        setCls->addMethod({"contains", getFunctionType({elem ? elem : m_int64Type}, m_boolType), "public", false, false, false});
        setCls->addMethod({"remove", getFunctionType({elem ? elem : m_int64Type}, m_voidType), "public", false, false, false});
        setCls->addMethod({"clear", getFunctionType({}, m_voidType), "public", false, false, false});
        setCls->addMethod({"size", getFunctionType({}, m_int32Type), "public", false, false, false});
        setCls->addMethod({"length", getFunctionType({}, m_int32Type), "public", false, false, false});
        setCls->addMethod({"isEmpty", getFunctionType({}, m_boolType), "public", false, false, false});
        return setCls;
    }

    // Check Stack<T>
    if (s.rfind("Stack<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        auto elem = resolveType(inner);
        std::string specName = "Stack<" + (elem ? elem->toString() : "any") + ">";
        auto existing = findUserType(specName);
        if (existing && existing->isClass()) {
            return existing;
        }
        auto stackCls = createClassType(specName);
        stackCls->addMethod({"push", getFunctionType({elem ? elem : m_int64Type}, m_voidType), "public", false, false, false});
        stackCls->addMethod({"pop", getFunctionType({}, elem ? elem : m_int64Type), "public", false, false, false});
        stackCls->addMethod({"peek", getFunctionType({}, elem ? elem : m_int64Type), "public", false, false, false});
        stackCls->addMethod({"size", getFunctionType({}, m_int32Type), "public", false, false, false});
        stackCls->addMethod({"length", getFunctionType({}, m_int32Type), "public", false, false, false});
        stackCls->addMethod({"isEmpty", getFunctionType({}, m_boolType), "public", false, false, false});
        stackCls->addMethod({"clear", getFunctionType({}, m_voidType), "public", false, false, false});
        return stackCls;
    }

    // Check Queue<T>
    if (s.rfind("Queue<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        auto elem = resolveType(inner);
        std::string specName = "Queue<" + (elem ? elem->toString() : "any") + ">";
        auto existing = findUserType(specName);
        if (existing && existing->isClass()) {
            return existing;
        }
        auto queueCls = createClassType(specName);
        queueCls->addMethod({"enqueue", getFunctionType({elem ? elem : m_int64Type}, m_voidType), "public", false, false, false});
        queueCls->addMethod({"dequeue", getFunctionType({}, elem ? elem : m_int64Type), "public", false, false, false});
        queueCls->addMethod({"peek", getFunctionType({}, elem ? elem : m_int64Type), "public", false, false, false});
        queueCls->addMethod({"size", getFunctionType({}, m_int32Type), "public", false, false, false});
        queueCls->addMethod({"length", getFunctionType({}, m_int32Type), "public", false, false, false});
        queueCls->addMethod({"isEmpty", getFunctionType({}, m_boolType), "public", false, false, false});
        queueCls->addMethod({"clear", getFunctionType({}, m_voidType), "public", false, false, false});
        return queueCls;
    }

    // Check PriorityQueue<T>
    if (s.rfind("PriorityQueue<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        auto elem = resolveType(inner);
        std::string specName = "PriorityQueue<" + (elem ? elem->toString() : "any") + ">";
        auto existing = findUserType(specName);
        if (existing && existing->isClass()) {
            return existing;
        }
        auto pqCls = createClassType(specName);
        pqCls->addMethod({"push", getFunctionType({elem ? elem : m_int64Type, m_int32Type}, m_voidType), "public", false, false, false});
        pqCls->addMethod({"pop", getFunctionType({}, elem ? elem : m_int64Type), "public", false, false, false});
        pqCls->addMethod({"peek", getFunctionType({}, elem ? elem : m_int64Type), "public", false, false, false});
        pqCls->addMethod({"peekPriority", getFunctionType({}, m_int32Type), "public", false, false, false});
        pqCls->addMethod({"size", getFunctionType({}, m_int32Type), "public", false, false, false});
        pqCls->addMethod({"length", getFunctionType({}, m_int32Type), "public", false, false, false});
        pqCls->addMethod({"isEmpty", getFunctionType({}, m_boolType), "public", false, false, false});
        pqCls->addMethod({"clear", getFunctionType({}, m_voidType), "public", false, false, false});
        return pqCls;
    }

    // Check primitive types
    if (s == "void") return m_voidType;
    if (s == "null") return m_nullType;
    if (s == "bool") return m_boolType;
    if (s == "int" || s == "int32") return m_int32Type;
    if (s == "uint" || s == "uint32") return m_uint32Type;
    if (s == "int8") return m_int8Type;
    if (s == "uint8" || s == "byte") return m_uint8Type;
    if (s == "int16") return m_int16Type;
    if (s == "uint16") return m_uint16Type;
    if (s == "int64") return m_int64Type;
    if (s == "uint64") return m_uint64Type;
    if (s == "float") return m_floatType;
    if (s == "double") return m_doubleType;
    if (s == "char") return m_charType;
    if (s == "string") return m_stringType;
    if (s == "any") return m_anyType;
    if (s == "unknown") return m_unknownType;

    // Check registered user types (classes, interfaces, structs, enums)
    auto userType = findUserType(s);
    if (userType) return userType;

    // Check Result<T, E>
    if (s.rfind("Result<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        std::vector<std::string> args = splitTypeArgs(inner);
        if (args.size() == 2) {
            auto baseResult = findUserType("Result");
            if (baseResult && baseResult->isEnum()) {
                return specializeEnumType(std::static_pointer_cast<EnumType>(baseResult), args);
            }
        }
    }

    // Check Option<T>
    if (s.rfind("Option<", 0) == 0 && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
        std::vector<std::string> args = splitTypeArgs(inner);
        if (args.size() == 1) {
            auto baseOption = findUserType("Option");
            if (baseOption && baseOption->isEnum()) {
                return specializeEnumType(std::static_pointer_cast<EnumType>(baseOption), args);
            }
        }
    }

    // Check user-defined generic class or enum specialization (e.g. Box<int>, Pair<string, int>)
    if (s.find('<') != std::string::npos && s.back() == '>') {
        size_t openBracket = s.find('<');
        std::string baseName = s.substr(0, openBracket);
        auto baseType = findUserType(baseName);
        if (baseType && baseType->isClass()) {
            auto baseCls = std::static_pointer_cast<ClassType>(baseType);
            std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
            std::vector<std::string> typeArgs = splitTypeArgs(inner);
            return specializeClassType(baseCls, typeArgs);
        }
        if (baseType && baseType->isEnum()) {
            auto baseEnum = std::static_pointer_cast<EnumType>(baseType);
            std::string inner = s.substr(openBracket + 1, s.size() - openBracket - 2);
            std::vector<std::string> typeArgs = splitTypeArgs(inner);
            return specializeEnumType(baseEnum, typeArgs);
        }
    }

    // If unresolved at this moment, return an unknown nominal placeholder or null
    return nullptr;
}

TypePtr TypeContext::getCommonType(const TypePtr& a, const TypePtr& b) {
    if (!a || !b) return m_errorType;
    if (a->equals(*b)) return a;
    if (a->isError() || b->isError()) return m_errorType;

    if (a->isAssignableTo(*b)) return b;
    if (b->isAssignableTo(*a)) return a;

    // Numeric promotion
    if (a->isNumeric() && b->isNumeric()) {
        if (a->getKind() == TypeKind::Double || b->getKind() == TypeKind::Double) {
            return m_doubleType;
        }
        if (a->getKind() == TypeKind::Float || b->getKind() == TypeKind::Float) {
            return m_floatType;
        }
        if (a->getKind() == TypeKind::Int64 || b->getKind() == TypeKind::Int64) {
            return m_int64Type;
        }
        if (a->getKind() == TypeKind::UInt64 || b->getKind() == TypeKind::UInt64) {
            return m_uint64Type;
        }
        return m_int32Type;
    }

    // Null and non-null T -> T?
    if (a->isNull() && !b->isNull()) return getNullableType(b);
    if (b->isNull() && !a->isNull()) return getNullableType(a);

    return m_anyType;
}

} // namespace sp::sema
