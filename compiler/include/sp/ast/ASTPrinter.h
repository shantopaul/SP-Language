#pragma once

#include "sp/ast/ASTVisitor.h"
#include <string>
#include <sstream>

namespace sp::ast {

class ASTNode;

class ASTPrinter : public ASTVisitor {
public:
    ASTPrinter() = default;

    std::string print(const ASTNode& node);

    // Expressions
    void visit(const LiteralExpr& node) override;
    void visit(const IdentifierExpr& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const CallExpr& node) override;
    void visit(const MemberExpr& node) override;
    void visit(const IndexExpr& node) override;
    void visit(const ArrayExpr& node) override;
    void visit(const MapExpr& node) override;
    void visit(const LambdaExpr& node) override;
    void visit(const TernaryExpr& node) override;
    void visit(const AssignExpr& node) override;
    void visit(const NewExpr& node) override;
    void visit(const TryExpr& node) override;
    void visit(const AwaitExpr& node) override;
    void visit(const ComptimeExpr& node) override;

    // Statements
    void visit(const BlockStmt& node) override;
    void visit(const ExprStmt& node) override;
    void visit(const VarDeclStmt& node) override;
    void visit(const IfStmt& node) override;
    void visit(const WhileStmt& node) override;
    void visit(const DoWhileStmt& node) override;
    void visit(const ForStmt& node) override;
    void visit(const ForInStmt& node) override;
    void visit(const SwitchStmt& node) override;
    void visit(const MatchStmt& node) override;
    void visit(const ReturnStmt& node) override;
    void visit(const BreakStmt& node) override;
    void visit(const ContinueStmt& node) override;
    void visit(const TryCatchStmt& node) override;
    void visit(const ThrowStmt& node) override;

    // Declarations
    void visit(const FunctionDecl& node) override;
    void visit(const ClassDecl& node) override;
    void visit(const InterfaceDecl& node) override;
    void visit(const StructDecl& node) override;
    void visit(const EnumDecl& node) override;
    void visit(const ModuleDecl& node) override;
    void visit(const ImportDecl& node) override;
    void visit(const Program& node) override;

private:
    std::ostringstream m_out;
    int m_indent{0};

    void indent();
    void line(const std::string& text);
};

} // namespace sp::ast
