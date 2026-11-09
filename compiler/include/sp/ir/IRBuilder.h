#pragma once

#include "sp/ast/ASTVisitor.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Statement.h"
#include "sp/ast/Expression.h"
#include "sp/diagnostics/SourceLocation.h"
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

    // Phase 1 (pro-4.0.0): lowering failures are recorded, not thrown and
    // not silently dropped. build() returns nullptr when any error was
    // recorded (or the finished module fails IR verification); every
    // caller already treats a null module as a failed build.
    [[nodiscard]] bool hasErrors() const noexcept { return !m_errors.empty(); }
    [[nodiscard]] const std::vector<std::string>& getErrors() const noexcept { return m_errors; }

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
    void visit(const ast::TryExpr& node) override;
    void visit(const ast::AwaitExpr& node) override;
    void visit(const ast::ComptimeExpr& node) override;

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
    IRValuePtr emitCreateTaggedUnion(const std::string& typeName, int32_t tag, IRValuePtr payloadVal);
    void emitPopulateTaggedUnion(IRValuePtr slotPtr, const std::string& typeName, int32_t tag, IRValuePtr payloadVal);

    // Records a "not yet supported" error for a construct that has no
    // lowering. The message names the construct and its source location so
    // the failure is loud at compile time instead of silently changing
    // program semantics.
    void reportUnsupported(const diagnostics::SourceLocation& loc, const std::string& construct,
                           const std::string& detail);

    // Phase 3 slice A (pro-4.0.0): function pointers + overload mangling.
    // Canonical param-type strings for a free function decl, taken from
    // its FunctionSymbol (so `int32` and `int` spell the same signature).
    // Falls back to the decl's raw param strings when no symbol is found.
    std::vector<std::string> fnParamTypes(const ast::FunctionDecl& decl) const;
    // LLVM name for a free function: plain name, unless the name is
    // overloaded in the symbol table, in which case the signature suffix
    // (VTableBuilder::mangleOverloaded). `main` and extern "C" decls are
    // never mangled.
    std::string llvmFunctionName(const ast::FunctionDecl& decl) const;
    // LLVM name for a method: `Class_method`, suffixed when overloaded in
    // the declaring class.
    std::string llvmMethodName(const std::string& className, const ast::FunctionDecl& decl) const;
    // Param-type strings of a resolved call target (FunctionSymbol).
    static std::vector<std::string> targetParamTypes(const std::shared_ptr<sema::FunctionSymbol>& target);
    // LLVM name for a resolved direct free-function call target.
    std::string llvmCallName(const std::string& calleeName,
                             const std::shared_ptr<sema::FunctionSymbol>& target) const;

    // Lambda capture tracking: while lowering a lambda body, the names of
    // variables visible in the enclosing scopes (a capturing lambda would
    // reference one of these). An identifier that falls through to the
    // fallback while this stack is non-empty and names an outer variable
    // is a capture attempt and fails loudly (capturing lambdas are not
    // lowered). Nested lambdas push another frame.
    std::vector<std::unordered_set<std::string>> m_lambdaOuterVars;

    // Guards a C string-runtime call: every argument that must be a string
    // is checked against its sema type. A `char` (or other non-string,
    // non-pointer) value passed as `ptr` would segfault inside strstr etc.,
    // so such a call fails loudly with error[E1000] instead of being
    // emitted. Unknown (null) and error sema types are allowed through.
    // Returns false after reporting when any checked argument is unusable.
    bool checkStringArgs(const ast::CallExpr& node, const std::vector<size_t>& argIndices,
                         const std::string& methodName);

    [[maybe_unused]] const sema::SemanticAnalyzer& m_sema;
    std::unique_ptr<IRModule> m_module;
    IRFunction* m_currentFunction{nullptr};
    IRBasicBlock* m_currentBlock{nullptr};

    // "Not yet supported" errors recorded while lowering (see
    // reportUnsupported). build() fails the compilation when this is
    // non-empty instead of emitting a module with dropped semantics.
    std::vector<std::string> m_errors;

    // Scoped variable symbol tables: (alloca pointer, value type)
    std::vector<std::unordered_map<std::string, std::pair<IRValuePtr, IRTypePtr>>> m_scopes;
    // Scoped reference-counted variables: (name, allocaPtr) in declaration order for RAII drop emission
    std::vector<std::vector<std::pair<std::string, IRValuePtr>>> m_refCountedScopes;

    void emitScopeDrops(size_t scopeIndex);
    void emitAllFunctionDrops();

    // Loop control flow targets for break / continue
    std::vector<std::string> m_breakStack;
    std::vector<std::string> m_continueStack;

    // Phase 2 slice B2: true while lowering the body of an `async fn`
    // (set in visit(FunctionDecl) / compileMethod). visit(AwaitExpr)
    // only lowers inside an async function; sema normally rejects await
    // elsewhere (E1802) first, but IRBuilder must not silently accept it
    // if sema is ever bypassed.
    bool m_inAsyncFunction{false};

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
