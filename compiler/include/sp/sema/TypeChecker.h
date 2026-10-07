#pragma once

#include "sp/ast/ASTVisitor.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/sema/SymbolTable.h"
#include "sp/sema/Type.h"
#include <unordered_map>
#include <vector>

namespace sp::sema {

class TypeChecker : public ast::ASTVisitor {
public:
    TypeChecker(TypeContext& typeCtx, SymbolTable& symbolTable, diagnostics::DiagnosticEngine& diagnostics)
        : m_typeCtx(typeCtx), m_symbolTable(symbolTable), m_diagnostics(diagnostics) {}

    void check(const ast::Program& program);

    TypePtr checkExpr(const ast::ExpressionNode& expr);
    void checkStmt(const ast::StatementNode& stmt);

    [[nodiscard]] TypePtr getNodeType(const ast::ExpressionNode* expr) const {
        auto it = m_nodeTypes.find(expr);
        if (it != m_nodeTypes.end()) return it->second;
        return m_typeCtx.getError();
    }

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
    void visit(const ast::TryExpr& node) override;

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
    bool hasGuaranteedReturn(const ast::StatementNode& stmt);
    void verifyInterfaceImplementations(const ast::ClassDecl& decl, const std::shared_ptr<ClassType>& clsType);

    std::shared_ptr<FunctionSymbol> resolveOverload(
        const std::vector<std::shared_ptr<FunctionSymbol>>& candidates,
        const std::vector<TypePtr>& argTypes,
        const diagnostics::SourceLocation& loc,
        const std::string& name
    );

    TypeContext& m_typeCtx;
    SymbolTable& m_symbolTable;
    diagnostics::DiagnosticEngine& m_diagnostics;

    TypePtr m_currentExprType{nullptr};
    TypePtr m_currentFunctionReturnType{nullptr};
    std::shared_ptr<ClassType> m_currentEnclosingClass{nullptr};
    int m_loopDepth{0};
    int m_switchDepth{0};

    std::unordered_map<const ast::ExpressionNode*, TypePtr> m_nodeTypes;
};

} // namespace sp::sema
