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
    return equals(target) || Type::isAssignableTo(target);
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
    if (target.isInteger()) return true;
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

TypePtr TypeContext::resolveType(const std::string& rawTypeStr) {
    // Strip leading and trailing whitespace
    size_t first = rawTypeStr.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return m_unknownType;
    size_t last = rawTypeStr.find_last_not_of(" \t\n\r");
    std::string s = rawTypeStr.substr(first, (last - first + 1));

    if (s.empty()) return m_unknownType;

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
        listCls->addMethod({"size", getFunctionType({}, m_int64Type), "public", false, false, false});
        listCls->addMethod({"length", getFunctionType({}, m_int64Type), "public", false, false, false});
        listCls->addMethod({"isEmpty", getFunctionType({}, m_boolType), "public", false, false, false});
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

    // Check user-defined generic class specialization (e.g. Box<int>, Pair<string, int>)
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
