#pragma once

#include "sp/ast/Declaration.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/sema/NameResolver.h"
#include "sp/sema/SymbolTable.h"
#include "sp/sema/Type.h"
#include "sp/sema/TypeChecker.h"
#include <memory>

namespace sp::sema {

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(diagnostics::DiagnosticEngine& diagnostics)
        : m_diagnostics(diagnostics),
          m_nameResolver(m_typeCtx, m_symbolTable, m_diagnostics),
          m_typeChecker(m_typeCtx, m_symbolTable, m_diagnostics) {
        m_symbolTable.initBuiltins(m_typeCtx);
    }

    // Runs name resolution + type checking on the program. Returns
    // true iff the program is clean: false when sema adds errors AND
    // when the diagnostics already held errors on entry (e.g. parse
    // errors) — Phase 6 slice C. The passes still run in the latter
    // case (their side effects are used by the LSP/SymbolIndex, which
    // ignore the return value); only the boolean changed. Callers must
    // not proceed to codegen when this returns false.
    bool analyze(const ast::Program& program);

    [[nodiscard]] TypeContext& getTypeContext() noexcept { return m_typeCtx; }
    [[nodiscard]] const TypeContext& getTypeContext() const noexcept { return m_typeCtx; }

    [[nodiscard]] SymbolTable& getSymbolTable() noexcept { return m_symbolTable; }
    [[nodiscard]] const SymbolTable& getSymbolTable() const noexcept { return m_symbolTable; }

    [[nodiscard]] TypeChecker& getTypeChecker() noexcept { return m_typeChecker; }
    [[nodiscard]] const TypeChecker& getTypeChecker() const noexcept { return m_typeChecker; }

    void setEdition(ast::Edition edition) noexcept { m_typeChecker.setEdition(edition); }
    [[nodiscard]] ast::Edition getEdition() const noexcept { return m_typeChecker.getEdition(); }

    [[nodiscard]] TypePtr getNodeType(const ast::ExpressionNode* expr) const {
        return m_typeChecker.getNodeType(expr);
    }

    [[nodiscard]] std::shared_ptr<FunctionSymbol> getCallTarget(const ast::CallExpr* expr) const {
        return m_typeChecker.getCallTarget(expr);
    }

private:
    diagnostics::DiagnosticEngine& m_diagnostics;
    TypeContext m_typeCtx;
    SymbolTable m_symbolTable;
    NameResolver m_nameResolver;
    TypeChecker m_typeChecker;
};

} // namespace sp::sema
