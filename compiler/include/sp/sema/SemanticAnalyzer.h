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

    bool analyze(const ast::Program& program);

    [[nodiscard]] TypeContext& getTypeContext() noexcept { return m_typeCtx; }
    [[nodiscard]] const TypeContext& getTypeContext() const noexcept { return m_typeCtx; }

    [[nodiscard]] SymbolTable& getSymbolTable() noexcept { return m_symbolTable; }
    [[nodiscard]] const SymbolTable& getSymbolTable() const noexcept { return m_symbolTable; }

    [[nodiscard]] TypeChecker& getTypeChecker() noexcept { return m_typeChecker; }
    [[nodiscard]] const TypeChecker& getTypeChecker() const noexcept { return m_typeChecker; }

    [[nodiscard]] TypePtr getNodeType(const ast::ExpressionNode* expr) const {
        return m_typeChecker.getNodeType(expr);
    }

private:
    diagnostics::DiagnosticEngine& m_diagnostics;
    TypeContext m_typeCtx;
    SymbolTable m_symbolTable;
    NameResolver m_nameResolver;
    TypeChecker m_typeChecker;
};

} // namespace sp::sema
