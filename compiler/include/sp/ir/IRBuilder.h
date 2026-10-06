#pragma once

#include "sp/ast/ASTVisitor.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Statement.h"
#include "sp/ast/Expression.h"
#include "sp/ir/IRModule.h"
#include "sp/sema/SemanticAnalyzer.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace sp::codegen {
class VTableBuilder;
struct ClassLayout;
}

namespace sp::ir {

class IRBuilder : public ast::ASTVisitor {
public:
    explicit IRBuilder(const sema::SemanticAnalyzer& sema);
    ~IRBuilder() override;

    std::unique_ptr<IRModule> build(const ast::Program& program, const std::string& moduleName = "main");

    // Class and method compilation helpers
    void compileConstructor(const ast::ClassDecl& classDecl, const ast::FunctionDecl* ctorDecl, const codegen::ClassLayout& layout);
    void compileDefaultConstructor(const ast::ClassDecl& classDecl, const codegen::ClassLayout& layout);
    void compileMethod(const ast::ClassDecl& classDecl, const ast::FunctionDecl& methodDecl, const codegen::ClassLayout& layout);
    void compileSpecializedConstructor(const ast::FunctionDecl* ctorDecl, const std::vector<std::string>& typeParams, const std::vector<std::string>& typeArgs, const codegen::ClassLayout& layout);
    void compileSpecializedMethod(const ast::FunctionDecl& methodDecl, const std::vector<std::string>& typeParams, const std::vector<std::string>& typeArgs, const codegen::ClassLayout& layout);
    void compileSpecializedFunction(const ast::FunctionDecl& fnDecl, const std::vector<std::string>& typeArgs);

    IRValuePtr emitMemberPointer(const ast::MemberExpr& node, IRTypePtr& outFieldType);
    void setCurrentClass(const codegen::ClassLayout* layout) noexcept { m_currentClass = layout; }
    [[nodiscard]] const codegen::ClassLayout* getCurrentClass() const noexcept { return m_currentClass; }
    [[nodiscard]] codegen::VTableBuilder* getVTableBuilder() noexcept { return m_vtableBuilder.get(); }
    [[nodiscard]] const codegen::VTableBuilder* getVTableBuilder() const noexcept { return m_vtableBuilder.get(); }

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

    // Type mapping utility
    IRTypePtr mapType(const sema::Type* type) const;
    IRTypePtr mapType(const std::string& typeName) const;

private:
    void pushScope();
    void popScope();
    void declareVariable(const std::string& name, IRValuePtr allocaPtr, IRTypePtr valType);
    std::pair<IRValuePtr, IRTypePtr> lookupVariable(const std::string& name);

    IRBasicBlock* createBlock(const std::string& name);
    void setInsertPoint(IRBasicBlock* block);

    IRValuePtr emitCastIfNeeded(IRValuePtr val, IRTypePtr targetType);
    IRValuePtr convertToStringValue(IRValuePtr val, IRTypePtr valType, const ast::ExpressionNode* exprNode = nullptr);

    [[maybe_unused]] const sema::SemanticAnalyzer& m_sema;
    std::unique_ptr<IRModule> m_module;
    IRFunction* m_currentFunction{nullptr};
    IRBasicBlock* m_currentBlock{nullptr};

    // Scoped variable symbol tables: (alloca pointer, value type)
    std::vector<std::unordered_map<std::string, std::pair<IRValuePtr, IRTypePtr>>> m_scopes;

    // Loop control flow targets for break / continue
    std::vector<std::string> m_breakStack;
    std::vector<std::string> m_continueStack;

    // Evaluated expression result from the latest visited expression
    IRValuePtr m_lastExprValue{nullptr};
    IRTypePtr m_lastExprType{nullptr};

    size_t m_blockCounter{0};
    size_t m_varCounter{0};
    size_t m_lambdaCounter{0};

    std::unique_ptr<codegen::VTableBuilder> m_vtableBuilder;
    const codegen::ClassLayout* m_currentClass{nullptr};
    std::unordered_map<std::string, const ast::ClassDecl*> m_genericClasses;
    std::unordered_map<std::string, const ast::FunctionDecl*> m_genericFunctions;
    std::unordered_map<std::string, const ast::FunctionDecl*> m_astFunctions;
};

} // namespace sp::ir
