#pragma once

#include "sp/sema/Symbol.h"
#include "sp/sema/Type.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace sp::sema {

enum class ScopeKind {
    Global,
    Module,
    Class,
    Function,
    Block
};

class Scope : public std::enable_shared_from_this<Scope> {
public:
    Scope(ScopeKind kind, std::shared_ptr<Scope> parent = nullptr)
        : m_kind(kind), m_parent(std::move(parent)) {}

    [[nodiscard]] ScopeKind getKind() const noexcept { return m_kind; }
    [[nodiscard]] const std::shared_ptr<Scope>& getParent() const noexcept { return m_parent; }

    bool define(SymbolPtr symbol);
    [[nodiscard]] SymbolPtr lookupLocal(const std::string& name) const;
    [[nodiscard]] SymbolPtr lookup(const std::string& name) const;
    [[nodiscard]] std::vector<std::shared_ptr<FunctionSymbol>> lookupFunctions(const std::string& name) const;

    [[nodiscard]] const std::unordered_map<std::string, SymbolPtr>& getSymbols() const noexcept { return m_symbols; }

private:
    ScopeKind m_kind;
    std::shared_ptr<Scope> m_parent;
    std::unordered_map<std::string, SymbolPtr> m_symbols;
};

using ScopePtr = std::shared_ptr<Scope>;

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable() = default;

    void enterScope(ScopeKind kind);
    void exitScope();

    [[nodiscard]] const ScopePtr& getCurrentScope() const noexcept { return m_currentScope; }
    [[nodiscard]] const ScopePtr& getGlobalScope() const noexcept { return m_globalScope; }
    [[nodiscard]] size_t getDepth() const noexcept { return m_scopeStack.size(); }

    bool define(SymbolPtr symbol);
    [[nodiscard]] SymbolPtr lookup(const std::string& name) const;
    [[nodiscard]] SymbolPtr lookupCurrentScope(const std::string& name) const;
    [[nodiscard]] std::vector<std::shared_ptr<FunctionSymbol>> lookupFunctions(const std::string& name) const;

    void initBuiltins(TypeContext& typeCtx);

private:
    ScopePtr m_globalScope;
    ScopePtr m_currentScope;
    std::vector<ScopePtr> m_scopeStack;
};

} // namespace sp::sema
