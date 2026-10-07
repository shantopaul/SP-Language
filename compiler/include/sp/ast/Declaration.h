#pragma once

#include "sp/ast/Statement.h"
#include <string>
#include <vector>
#include <memory>

namespace sp::ast {

class Parameter {
public:
    Parameter(std::string type, std::string name, ExprPtr defaultValue = nullptr)
        : m_type(std::move(type)), m_name(std::move(name)), m_defaultValue(std::move(defaultValue)) {}

    [[nodiscard]] const std::string& getType() const noexcept { return m_type; }
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] bool hasDefaultValue() const noexcept { return m_defaultValue != nullptr; }
    [[nodiscard]] const ExpressionNode* getDefaultValue() const noexcept { return m_defaultValue.get(); }
    [[nodiscard]] ExprPtr& getDefaultValuePtr() noexcept { return m_defaultValue; }

private:
    std::string m_type;
    std::string m_name;
    ExprPtr m_defaultValue;
};

class FunctionDecl : public StatementNode {
public:

    FunctionDecl(std::string name,
                 std::vector<std::string> typeParams,
                 std::vector<Parameter> params,
                 std::string returnType,
                 std::unique_ptr<BlockStmt> body,
                 ExprPtr shortBody,
                 std::string visibility,
                 bool isStatic,
                 bool isOverride,
                 bool isAbstract,
                 bool isAsync,
                 diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::FunctionDecl, std::move(loc)),
          m_name(std::move(name)),
          m_typeParams(std::move(typeParams)),
          m_params(std::move(params)),
          m_returnType(std::move(returnType)),
          m_body(std::move(body)),
          m_shortBody(std::move(shortBody)),
          m_visibility(std::move(visibility)),
          m_isStatic(isStatic),
          m_isOverride(isOverride),
          m_isAbstract(isAbstract),
          m_isAsync(isAsync) {}

    FunctionDecl(std::string name,
                 std::vector<Parameter> params,
                 std::string returnType,
                 std::unique_ptr<BlockStmt> body,
                 ExprPtr shortBody,
                 std::string visibility,
                 bool isStatic,
                 bool isOverride,
                 bool isAbstract,
                 bool isAsync,
                 diagnostics::SourceLocation loc)
        : FunctionDecl(std::move(name), {}, std::move(params), std::move(returnType),
                       std::move(body), std::move(shortBody), std::move(visibility),
                       isStatic, isOverride, isAbstract, isAsync, std::move(loc)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<std::string>& getTypeParams() const noexcept { return m_typeParams; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeParams.empty(); }
    [[nodiscard]] const std::vector<Parameter>& getParams() const noexcept { return m_params; }
    [[nodiscard]] const std::string& getReturnType() const noexcept { return m_returnType; }
    void setReturnType(std::string returnType) noexcept { m_returnType = std::move(returnType); }
    [[nodiscard]] bool hasBody() const noexcept { return m_body != nullptr; }
    [[nodiscard]] const BlockStmt* getBody() const noexcept { return m_body.get(); }
    [[nodiscard]] std::unique_ptr<BlockStmt>& getBodyPtr() noexcept { return m_body; }
    [[nodiscard]] bool isShortForm() const noexcept { return m_shortBody != nullptr; }
    [[nodiscard]] const ExpressionNode* getShortBody() const noexcept { return m_shortBody.get(); }
    [[nodiscard]] ExprPtr& getShortBodyPtr() noexcept { return m_shortBody; }

    [[nodiscard]] const std::string& getVisibility() const noexcept { return m_visibility; }
    [[nodiscard]] bool isStatic() const noexcept { return m_isStatic; }
    [[nodiscard]] bool isOverride() const noexcept { return m_isOverride; }
    [[nodiscard]] bool isAbstract() const noexcept { return m_isAbstract; }
    [[nodiscard]] bool isAsync() const noexcept { return m_isAsync; }
    [[nodiscard]] bool isUnsafe() const noexcept { return m_isUnsafe; }
    void setUnsafe(bool unsafe) noexcept { m_isUnsafe = unsafe; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_name;
    std::vector<std::string> m_typeParams;
    std::vector<Parameter> m_params;
    std::string m_returnType;
    std::unique_ptr<BlockStmt> m_body;
    ExprPtr m_shortBody;
    std::string m_visibility;
    bool m_isStatic;
    bool m_isOverride;
    bool m_isAbstract;
    bool m_isAsync;
    bool m_isUnsafe{false};
};

class ClassDecl : public StatementNode {
public:
    ClassDecl(std::string name,
              std::vector<std::string> typeParams,
              std::string baseClass,
              std::vector<std::string> interfaces,
              std::vector<ASTNodePtr> members,
              std::string visibility,
              bool isAbstract,
              diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ClassDecl, std::move(loc)),
          m_name(std::move(name)),
          m_typeParams(std::move(typeParams)),
          m_baseClass(std::move(baseClass)),
          m_interfaces(std::move(interfaces)),
          m_members(std::move(members)),
          m_visibility(std::move(visibility)),
          m_isAbstract(isAbstract) {}

    ClassDecl(std::string name,
              std::string baseClass,
              std::vector<std::string> interfaces,
              std::vector<ASTNodePtr> members,
              std::string visibility,
              bool isAbstract,
              diagnostics::SourceLocation loc)
        : ClassDecl(std::move(name), {}, std::move(baseClass), std::move(interfaces),
                    std::move(members), std::move(visibility), isAbstract, std::move(loc)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<std::string>& getTypeParams() const noexcept { return m_typeParams; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeParams.empty(); }
    [[nodiscard]] const std::string& getBaseClass() const noexcept { return m_baseClass; }
    [[nodiscard]] bool hasBaseClass() const noexcept { return !m_baseClass.empty(); }
    [[nodiscard]] const std::vector<std::string>& getInterfaces() const noexcept { return m_interfaces; }
    [[nodiscard]] const std::vector<ASTNodePtr>& getMembers() const noexcept { return m_members; }
    [[nodiscard]] std::vector<ASTNodePtr>& getMembers() noexcept { return m_members; }
    [[nodiscard]] const std::string& getVisibility() const noexcept { return m_visibility; }
    [[nodiscard]] bool isAbstract() const noexcept { return m_isAbstract; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_name;
    std::vector<std::string> m_typeParams;
    std::string m_baseClass;
    std::vector<std::string> m_interfaces;
    std::vector<ASTNodePtr> m_members;
    std::string m_visibility;
    bool m_isAbstract;
};

class InterfaceDecl : public StatementNode {
public:
    InterfaceDecl(std::string name,
                  std::vector<std::unique_ptr<FunctionDecl>> methods,
                  std::string visibility,
                  diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::InterfaceDecl, std::move(loc)),
          m_name(std::move(name)),
          m_methods(std::move(methods)),
          m_visibility(std::move(visibility)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<std::unique_ptr<FunctionDecl>>& getMethods() const noexcept { return m_methods; }
    [[nodiscard]] std::vector<std::unique_ptr<FunctionDecl>>& getMethods() noexcept { return m_methods; }
    [[nodiscard]] const std::string& getVisibility() const noexcept { return m_visibility; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_name;
    std::vector<std::unique_ptr<FunctionDecl>> m_methods;
    std::string m_visibility;
};

class StructDecl : public StatementNode {
public:
    StructDecl(std::string name,
               std::vector<std::unique_ptr<VarDeclStmt>> fields,
               std::string visibility,
               diagnostics::SourceLocation loc,
               bool isReprC = false)
        : StatementNode(ASTNodeKind::StructDecl, std::move(loc)),
          m_name(std::move(name)),
          m_fields(std::move(fields)),
          m_visibility(std::move(visibility)),
          m_isReprC(isReprC) {}

    StructDecl(std::string name,
               std::vector<std::unique_ptr<VarDeclStmt>> fields,
               std::string visibility,
               bool isReprC,
               diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::StructDecl, std::move(loc)),
          m_name(std::move(name)),
          m_fields(std::move(fields)),
          m_visibility(std::move(visibility)),
          m_isReprC(isReprC) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<std::unique_ptr<VarDeclStmt>>& getFields() const noexcept { return m_fields; }
    [[nodiscard]] std::vector<std::unique_ptr<VarDeclStmt>>& getFields() noexcept { return m_fields; }
    [[nodiscard]] const std::string& getVisibility() const noexcept { return m_visibility; }
    [[nodiscard]] bool isReprC() const noexcept { return m_isReprC; }
    void setReprC(bool reprC) noexcept { m_isReprC = reprC; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_name;
    std::vector<std::unique_ptr<VarDeclStmt>> m_fields;
    std::string m_visibility;
    bool m_isReprC{false};
};

class EnumVariant {
public:
    EnumVariant(std::string name, ExprPtr value = nullptr, std::vector<std::string> payloadTypes = {})
        : m_name(std::move(name)), m_value(std::move(value)), m_payloadTypes(std::move(payloadTypes)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] bool hasValue() const noexcept { return m_value != nullptr; }
    [[nodiscard]] const ExpressionNode* getValue() const noexcept { return m_value.get(); }
    [[nodiscard]] ExprPtr& getValuePtr() noexcept { return m_value; }
    [[nodiscard]] bool hasPayload() const noexcept { return !m_payloadTypes.empty(); }
    [[nodiscard]] const std::vector<std::string>& getPayloadTypes() const noexcept { return m_payloadTypes; }
    [[nodiscard]] std::vector<std::string>& getPayloadTypes() noexcept { return m_payloadTypes; }

private:
    std::string m_name;
    ExprPtr m_value;
    std::vector<std::string> m_payloadTypes;
};

class EnumDecl : public StatementNode {
public:
    EnumDecl(std::string name,
             std::vector<std::string> typeParams,
             std::vector<EnumVariant> variants,
             std::string visibility,
             diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::EnumDecl, std::move(loc)),
          m_name(std::move(name)),
          m_typeParams(std::move(typeParams)),
          m_variants(std::move(variants)),
          m_visibility(std::move(visibility)) {}

    EnumDecl(std::string name,
             std::vector<EnumVariant> variants,
             std::string visibility,
             diagnostics::SourceLocation loc)
        : EnumDecl(std::move(name), {}, std::move(variants), std::move(visibility), std::move(loc)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<std::string>& getTypeParams() const noexcept { return m_typeParams; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeParams.empty(); }
    [[nodiscard]] const std::vector<EnumVariant>& getVariants() const noexcept { return m_variants; }
    [[nodiscard]] std::vector<EnumVariant>& getVariants() noexcept { return m_variants; }
    [[nodiscard]] const std::string& getVisibility() const noexcept { return m_visibility; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_name;
    std::vector<std::string> m_typeParams;
    std::vector<EnumVariant> m_variants;
    std::string m_visibility;
};

class ModuleDecl : public StatementNode {
public:
    ModuleDecl(std::string name, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ModuleDecl, std::move(loc)),
          m_name(std::move(name)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_name;
};

class ImportDecl : public StatementNode {
public:
    ImportDecl(std::string path, bool isWildcard, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ImportDecl, std::move(loc)),
          m_path(std::move(path)),
          m_isWildcard(isWildcard) {}

    [[nodiscard]] const std::string& getPath() const noexcept { return m_path; }
    [[nodiscard]] bool isWildcard() const noexcept { return m_isWildcard; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_path;
    bool m_isWildcard;
};

class Program : public ASTNode {
public:
    Program(std::vector<ASTNodePtr> declarations, diagnostics::SourceLocation loc)
        : ASTNode(ASTNodeKind::Program, std::move(loc)),
          m_declarations(std::move(declarations)) {}

    [[nodiscard]] const std::vector<ASTNodePtr>& getDeclarations() const noexcept { return m_declarations; }
    [[nodiscard]] std::vector<ASTNodePtr>& getDeclarations() noexcept { return m_declarations; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::vector<ASTNodePtr> m_declarations;
};

} // namespace sp::ast
