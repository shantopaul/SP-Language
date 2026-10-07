#pragma once

#include "sp/ast/ASTVisitor.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/sema/Type.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sp::sema {

class TypeChecker;

enum class EscapeState {
    LocalNonEscaping, // Confined entirely within local function frame; eligible for ARC optimization
    GlobalEscaping    // Escapes function activation frame via return, heap store, or escaping capture
};

struct VarEscapeInfo {
    std::string name;
    EscapeState state{EscapeState::LocalNonEscaping};
    bool isReferenceType{false};
    std::vector<std::string> escapeReasons;
};

class FunctionEscapeSummary {
public:
    explicit FunctionEscapeSummary(std::string name = "")
        : m_functionName(std::move(name)) {}

    [[nodiscard]] const std::string& getFunctionName() const noexcept { return m_functionName; }
    void setFunctionName(std::string name) { m_functionName = std::move(name); }

    void recordVariable(const std::string& name, bool isRef);
    void markEscaping(const std::string& name, const std::string& reason);
    [[nodiscard]] bool escapes(const std::string& name) const;
    [[nodiscard]] EscapeState getEscapeState(const std::string& name) const;
    [[nodiscard]] const std::unordered_map<std::string, VarEscapeInfo>& getVariables() const noexcept {
        return m_variables;
    }
    [[nodiscard]] const VarEscapeInfo* getVariableInfo(const std::string& name) const;

private:
    std::string m_functionName;
    std::unordered_map<std::string, VarEscapeInfo> m_variables;
};

class EscapeAnalysis : public ast::ASTVisitor {
public:
    explicit EscapeAnalysis(const TypeChecker* typeChecker = nullptr);
    ~EscapeAnalysis() override = default;

    void analyze(const ast::Program& program);
    void analyzeFunction(const ast::FunctionDecl& fnDecl);

    [[nodiscard]] const FunctionEscapeSummary* getSummary(const std::string& funcName) const;
    [[nodiscard]] bool escapes(const std::string& funcName, const std::string& varName) const;
    [[nodiscard]] EscapeState getEscapeState(const std::string& funcName, const std::string& varName) const;
    [[nodiscard]] const std::unordered_map<std::string, FunctionEscapeSummary>& getAllSummaries() const noexcept {
        return m_summaries;
    }

    // AST Visitor overrides
    // Expressions
    void visit(const ast::LiteralExpr& node) override;
    void visit(const ast::IdentifierExpr& node) override;
    void visit(const ast::BinaryExpr& node) override;
    void visit(const ast::UnaryExpr& node) override;
    void visit(const ast::CallExpr& node) override;
    void visit(const ast::MemberExpr& node) override;
    void visit(const ast::IndexExpr& node) override;
    void visit(const ast::ArrayExpr& node) override;
    void visit(const ast::MapExpr& node) override;
    void visit(const ast::LambdaExpr& node) override;
    void visit(const ast::TernaryExpr& node) override;
    void visit(const ast::AssignExpr& node) override;
    void visit(const ast::NewExpr& node) override;

    // Statements
    void visit(const ast::BlockStmt& node) override;
    void visit(const ast::ExprStmt& node) override;
    void visit(const ast::VarDeclStmt& node) override;
    void visit(const ast::IfStmt& node) override;
    void visit(const ast::WhileStmt& node) override;
    void visit(const ast::DoWhileStmt& node) override;
    void visit(const ast::ForStmt& node) override;
    void visit(const ast::ForInStmt& node) override;
    void visit(const ast::SwitchStmt& node) override;
    void visit(const ast::MatchStmt& node) override;
    void visit(const ast::ReturnStmt& node) override;
    void visit(const ast::BreakStmt& node) override;
    void visit(const ast::ContinueStmt& node) override;
    void visit(const ast::TryCatchStmt& node) override;
    void visit(const ast::ThrowStmt& node) override;

    // Declarations
    void visit(const ast::FunctionDecl& node) override;
    void visit(const ast::ClassDecl& node) override;
    void visit(const ast::InterfaceDecl& node) override;
    void visit(const ast::StructDecl& node) override;
    void visit(const ast::EnumDecl& node) override;
    void visit(const ast::ModuleDecl& node) override;
    void visit(const ast::ImportDecl& node) override;
    void visit(const ast::Program& node) override;

private:
    void collectIdentifiers(const ast::ExpressionNode* expr, std::vector<std::string>& outNames);
    void propagateAliases();

    const TypeChecker* m_typeChecker{nullptr};
    std::unordered_map<std::string, FunctionEscapeSummary> m_summaries;
    FunctionEscapeSummary* m_currentSummary{nullptr};
    std::unordered_map<std::string, std::unordered_set<std::string>> m_aliasGraph;
};

} // namespace sp::sema
