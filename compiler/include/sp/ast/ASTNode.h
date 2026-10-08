#pragma once

#include "sp/diagnostics/SourceLocation.h"
#include <memory>
#include <string>
#include <vector>

namespace sp::ast {

class ASTVisitor;

enum class ASTNodeKind {
    // Declarations & Top-level
    Program,
    ModuleDecl,
    ImportDecl,
    FunctionDecl,
    ClassDecl,
    InterfaceDecl,
    StructDecl,
    EnumDecl,

    // Statements
    BlockStmt,
    ExprStmt,
    VarDeclStmt,
    IfStmt,
    WhileStmt,
    DoWhileStmt,
    ForStmt,
    ForInStmt,
    SwitchStmt,
    MatchStmt,
    ReturnStmt,
    BreakStmt,
    ContinueStmt,
    TryCatchStmt,
    ThrowStmt,

    // Expressions
    LiteralExpr,
    IdentifierExpr,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    MemberExpr,
    IndexExpr,
    ArrayExpr,
    MapExpr,
    LambdaExpr,
    TernaryExpr,
    AssignExpr,
    NewExpr,
    TryExpr,
    AwaitExpr,
    ComptimeExpr
};

class ASTNode {
public:
    explicit ASTNode(ASTNodeKind kind, diagnostics::SourceLocation loc)
        : m_kind(kind), m_location(std::move(loc)) {}

    virtual ~ASTNode() = default;

    [[nodiscard]] ASTNodeKind getKind() const noexcept { return m_kind; }
    [[nodiscard]] const diagnostics::SourceLocation& getLocation() const noexcept { return m_location; }

    virtual void accept(ASTVisitor& visitor) const = 0;

private:
    ASTNodeKind m_kind;
    diagnostics::SourceLocation m_location;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct Annotation {
    std::string name;
    std::vector<std::string> args;
    diagnostics::SourceLocation location;
};

} // namespace sp::ast
