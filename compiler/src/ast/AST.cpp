#include "sp/ast/ASTVisitor.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/ast/Declaration.h"

namespace sp::ast {

// Expressions accept
void LiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IdentifierExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BinaryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void MemberExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IndexExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ArrayExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void MapExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void LambdaExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void TernaryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void AssignExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void NewExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void TryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// Statements accept
void BlockStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ExprStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void VarDeclStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IfStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void WhileStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void DoWhileStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ForStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ForInStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void SwitchStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void MatchStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BreakStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ContinueStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void TryCatchStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ThrowStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// Declarations accept
void FunctionDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ClassDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void InterfaceDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void StructDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void EnumDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ModuleDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ImportDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void Program::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

} // namespace sp::ast
