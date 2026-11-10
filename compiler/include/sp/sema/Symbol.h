#pragma once

#include "sp/diagnostics/SourceLocation.h"
#include "sp/sema/Type.h"
#include <string>
#include <memory>
#include <vector>

namespace sp::ast {
class FunctionDecl;
}

namespace sp::sema {

enum class SymbolKind {
    Variable,
    Constant,
    Parameter,
    Function,
    Method,
    Class,
    Interface,
    Struct,
    Enum,
    EnumMember,
    Module
};

class Symbol : public std::enable_shared_from_this<Symbol> {
public:
    Symbol(SymbolKind kind,
           std::string name,
           TypePtr type,
           diagnostics::SourceLocation loc,
           std::string visibility = "public",
           bool isStatic = false)
        : m_kind(kind),
          m_name(std::move(name)),
          m_type(std::move(type)),
          m_location(std::move(loc)),
          m_visibility(std::move(visibility)),
          m_isStatic(isStatic) {}

    virtual ~Symbol() = default;

    [[nodiscard]] SymbolKind getKind() const noexcept { return m_kind; }
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const TypePtr& getType() const noexcept { return m_type; }
    void setType(TypePtr type) noexcept { m_type = std::move(type); }

    [[nodiscard]] const diagnostics::SourceLocation& getLocation() const noexcept { return m_location; }
    [[nodiscard]] const std::string& getVisibility() const noexcept { return m_visibility; }
    [[nodiscard]] bool isStatic() const noexcept { return m_isStatic; }

    [[nodiscard]] bool isVariable() const noexcept {
        return m_kind == SymbolKind::Variable || m_kind == SymbolKind::Constant || m_kind == SymbolKind::Parameter;
    }
    [[nodiscard]] bool isFunction() const noexcept {
        return m_kind == SymbolKind::Function || m_kind == SymbolKind::Method;
    }
    [[nodiscard]] bool isType() const noexcept {
        return m_kind == SymbolKind::Class || m_kind == SymbolKind::Interface ||
               m_kind == SymbolKind::Struct || m_kind == SymbolKind::Enum;
    }

private:
    SymbolKind m_kind;
    std::string m_name;
    TypePtr m_type;
    diagnostics::SourceLocation m_location;
    std::string m_visibility;
    bool m_isStatic{false};
};

using SymbolPtr = std::shared_ptr<Symbol>;

class VariableSymbol : public Symbol {
public:
    VariableSymbol(std::string name,
                   TypePtr type,
                   diagnostics::SourceLocation loc,
                   bool isConst = false,
                   bool isParam = false,
                   std::string visibility = "public",
                   bool isStatic = false)
        : Symbol(isConst ? SymbolKind::Constant : (isParam ? SymbolKind::Parameter : SymbolKind::Variable),
                 std::move(name), std::move(type), std::move(loc), std::move(visibility), isStatic),
          m_isConst(isConst), m_isParam(isParam) {}

    [[nodiscard]] bool isConst() const noexcept { return m_isConst; }
    [[nodiscard]] bool isParameter() const noexcept { return m_isParam; }

private:
    bool m_isConst{false};
    bool m_isParam{false};
};

class FunctionSymbol : public Symbol {
public:
    FunctionSymbol(std::string name,
                   std::shared_ptr<FunctionType> fnType,
                   diagnostics::SourceLocation loc,
                   std::string visibility = "public",
                   bool isStatic = false,
                   bool isMethod = false,
                   bool isAbstract = false,
                   bool isOverride = false)
        : Symbol(isMethod ? SymbolKind::Method : SymbolKind::Function,
                 std::move(name), fnType, std::move(loc), std::move(visibility), isStatic),
          m_fnType(fnType),
          m_isMethod(isMethod),
          m_isAbstract(isAbstract),
          m_isOverride(isOverride) {}

    [[nodiscard]] const std::shared_ptr<FunctionType>& getFunctionType() const noexcept { return m_fnType; }
    void setFunctionType(std::shared_ptr<FunctionType> fnType) noexcept {
        m_fnType = fnType;
        setType(fnType);
    }
    [[nodiscard]] bool isMethod() const noexcept { return m_isMethod; }
    [[nodiscard]] bool isAbstract() const noexcept { return m_isAbstract; }
    [[nodiscard]] bool isOverride() const noexcept { return m_isOverride; }
    // Phase 3 slice C: whether the function is declared `async`. This
    // is a plain bool (set from the ASTDecl by NameResolver, or copied
    // from the MemberMethod at resolution time for methods, because
    // method symbols are rebuilt without an ASTDecl). It must NOT call
    // through m_astDecl here: FunctionDecl is only forward-declared in
    // this header, so m_astDecl->isAsync() would be an incomplete-type
    // error in every TU that includes this header.
    void setAsync(bool value) noexcept { m_isAsync = value; }
    [[nodiscard]] bool isAsync() const noexcept { return m_isAsync; }

    void setTypeParams(std::vector<std::string> params) { m_typeParams = std::move(params); }
    [[nodiscard]] const std::vector<std::string>& getTypeParams() const noexcept { return m_typeParams; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeParams.empty(); }

    void setASTDecl(const ast::FunctionDecl* decl) noexcept { m_astDecl = decl; }
    [[nodiscard]] const ast::FunctionDecl* getASTDecl() const noexcept { return m_astDecl; }

    void addOverload(std::shared_ptr<FunctionSymbol> overload) {
        m_overloads.push_back(std::move(overload));
    }
    [[nodiscard]] const std::vector<std::shared_ptr<FunctionSymbol>>& getOverloads() const noexcept { return m_overloads; }

private:
    std::shared_ptr<FunctionType> m_fnType;
    std::vector<std::string> m_typeParams;
    const ast::FunctionDecl* m_astDecl{nullptr};
    bool m_isMethod{false};
    bool m_isAbstract{false};
    bool m_isOverride{false};
    bool m_isAsync{false};
    std::vector<std::shared_ptr<FunctionSymbol>> m_overloads;
};

class ClassSymbol : public Symbol {
public:
    ClassSymbol(std::string name,
                std::shared_ptr<ClassType> classType,
                diagnostics::SourceLocation loc,
                std::string visibility = "public")
        : Symbol(SymbolKind::Class, std::move(name), classType, std::move(loc), std::move(visibility)),
          m_classType(std::move(classType)) {}

    [[nodiscard]] const std::shared_ptr<ClassType>& getClassType() const noexcept { return m_classType; }

private:
    std::shared_ptr<ClassType> m_classType;
};

class InterfaceSymbol : public Symbol {
public:
    InterfaceSymbol(std::string name,
                    std::shared_ptr<InterfaceType> interfaceType,
                    diagnostics::SourceLocation loc,
                    std::string visibility = "public")
        : Symbol(SymbolKind::Interface, std::move(name), interfaceType, std::move(loc), std::move(visibility)),
          m_interfaceType(std::move(interfaceType)) {}

    [[nodiscard]] const std::shared_ptr<InterfaceType>& getInterfaceType() const noexcept { return m_interfaceType; }

private:
    std::shared_ptr<InterfaceType> m_interfaceType;
};

class StructSymbol : public Symbol {
public:
    StructSymbol(std::string name,
                 std::shared_ptr<StructType> structType,
                 diagnostics::SourceLocation loc,
                 std::string visibility = "public")
        : Symbol(SymbolKind::Struct, std::move(name), structType, std::move(loc), std::move(visibility)),
          m_structType(std::move(structType)) {}

    [[nodiscard]] const std::shared_ptr<StructType>& getStructType() const noexcept { return m_structType; }

private:
    std::shared_ptr<StructType> m_structType;
};

class EnumSymbol : public Symbol {
public:
    EnumSymbol(std::string name,
               std::shared_ptr<EnumType> enumType,
               diagnostics::SourceLocation loc,
               std::string visibility = "public")
        : Symbol(SymbolKind::Enum, std::move(name), enumType, std::move(loc), std::move(visibility)),
          m_enumType(std::move(enumType)) {}

    [[nodiscard]] const std::shared_ptr<EnumType>& getEnumType() const noexcept { return m_enumType; }

private:
    std::shared_ptr<EnumType> m_enumType;
};

} // namespace sp::sema
