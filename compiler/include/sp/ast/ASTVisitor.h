#pragma once

namespace sp::ast {

// Forward declarations
class LiteralExpr;
class IdentifierExpr;
class BinaryExpr;
class UnaryExpr;
class CallExpr;
class MemberExpr;
class IndexExpr;
class ArrayExpr;
class MapExpr;
class LambdaExpr;
class TernaryExpr;
class AssignExpr;
class NewExpr;
class TryExpr;

class BlockStmt;
class ExprStmt;
class VarDeclStmt;
class IfStmt;
class WhileStmt;
class DoWhileStmt;
class ForStmt;
class ForInStmt;
class SwitchStmt;
class MatchStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class TryCatchStmt;
class ThrowStmt;

class FunctionDecl;
class ClassDecl;
class InterfaceDecl;
class StructDecl;
class EnumDecl;
class ModuleDecl;
class ImportDecl;
class Program;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Expressions
    virtual void visit(const LiteralExpr& node) = 0;
    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const CallExpr& node) = 0;
    virtual void visit(const MemberExpr& node) = 0;
    virtual void visit(const IndexExpr& node) = 0;
    virtual void visit(const ArrayExpr& node) = 0;
    virtual void visit(const MapExpr& node) = 0;
    virtual void visit(const LambdaExpr& node) = 0;
    virtual void visit(const TernaryExpr& node) = 0;
    virtual void visit(const AssignExpr& node) = 0;
    virtual void visit(const NewExpr& node) = 0;
    virtual void visit(const TryExpr& node) = 0;

    // Statements
    virtual void visit(const BlockStmt& node) = 0;
    virtual void visit(const ExprStmt& node) = 0;
    virtual void visit(const VarDeclStmt& node) = 0;
    virtual void visit(const IfStmt& node) = 0;
    virtual void visit(const WhileStmt& node) = 0;
    virtual void visit(const DoWhileStmt& node) = 0;
    virtual void visit(const ForStmt& node) = 0;
    virtual void visit(const ForInStmt& node) = 0;
    virtual void visit(const SwitchStmt& node) = 0;
    virtual void visit(const MatchStmt& node) = 0;
    virtual void visit(const ReturnStmt& node) = 0;
    virtual void visit(const BreakStmt& node) = 0;
    virtual void visit(const ContinueStmt& node) = 0;
    virtual void visit(const TryCatchStmt& node) = 0;
    virtual void visit(const ThrowStmt& node) = 0;

    // Declarations
    virtual void visit(const FunctionDecl& node) = 0;
    virtual void visit(const ClassDecl& node) = 0;
    virtual void visit(const InterfaceDecl& node) = 0;
    virtual void visit(const StructDecl& node) = 0;
    virtual void visit(const EnumDecl& node) = 0;
    virtual void visit(const ModuleDecl& node) = 0;
    virtual void visit(const ImportDecl& node) = 0;
    virtual void visit(const Program& node) = 0;
};

} // namespace sp::ast
