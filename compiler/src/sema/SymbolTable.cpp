#include "sp/sema/SymbolTable.h"

namespace sp::sema {

bool Scope::define(SymbolPtr symbol) {
    if (!symbol) return false;
    const std::string& name = symbol->getName();

    auto it = m_symbols.find(name);
    if (it != m_symbols.end()) {
        // Check if both are functions (overloading support)
        if (it->second->isFunction() && symbol->isFunction()) {
            auto existingFn = std::static_pointer_cast<FunctionSymbol>(it->second);
            auto newFn = std::static_pointer_cast<FunctionSymbol>(symbol);

            // Check if parameter signature matches existing function or any of its overloads
            auto checkSameParams = [](const FunctionSymbol& f1, const FunctionSymbol& f2) {
                const auto& p1 = f1.getFunctionType()->getParamTypes();
                const auto& p2 = f2.getFunctionType()->getParamTypes();
                if (p1.size() != p2.size()) return false;
                for (size_t i = 0; i < p1.size(); ++i) {
                    if (!p1[i]->equals(*p2[i])) return false;
                }
                return true;
            };

            if (checkSameParams(*existingFn, *newFn)) {
                if (existingFn->getLocation().filename == "<built-in>") {
                    // Concrete implementation overriding built-in stub
                    return true;
                }
                return false; // Duplicate function signature
            }

            for (const auto& overload : existingFn->getOverloads()) {
                if (checkSameParams(*overload, *newFn)) {
                    if (overload->getLocation().filename == "<built-in>") {
                        // Concrete implementation overriding built-in stub
                        return true;
                    }
                    return false; // Duplicate overload signature
                }
            }

            // Valid overload
            existingFn->addOverload(newFn);
            return true;
        }

        // Duplicate name in same scope
        return false;
    }

    m_symbols[name] = std::move(symbol);
    return true;
}

SymbolPtr Scope::lookupLocal(const std::string& name) const {
    auto it = m_symbols.find(name);
    if (it != m_symbols.end()) {
        return it->second;
    }
    return nullptr;
}

SymbolPtr Scope::lookup(const std::string& name) const {
    auto sym = lookupLocal(name);
    if (sym) return sym;

    if (m_parent) {
        return m_parent->lookup(name);
    }
    return nullptr;
}

std::vector<std::shared_ptr<FunctionSymbol>> Scope::lookupFunctions(const std::string& name) const {
    std::vector<std::shared_ptr<FunctionSymbol>> result;
    auto sym = lookup(name);
    if (sym && sym->isFunction()) {
        auto fn = std::static_pointer_cast<FunctionSymbol>(sym);
        result.push_back(fn);
        for (const auto& ov : fn->getOverloads()) {
            result.push_back(ov);
        }
    }
    return result;
}

// SymbolTable implementation
SymbolTable::SymbolTable() {
    m_globalScope = std::make_shared<Scope>(ScopeKind::Global, nullptr);
    m_currentScope = m_globalScope;
    m_scopeStack.push_back(m_currentScope);
}

void SymbolTable::enterScope(ScopeKind kind) {
    auto newScope = std::make_shared<Scope>(kind, m_currentScope);
    m_currentScope = newScope;
    m_scopeStack.push_back(m_currentScope);
}

void SymbolTable::exitScope() {
    if (m_scopeStack.size() > 1) {
        m_scopeStack.pop_back();
        m_currentScope = m_scopeStack.back();
    }
}

bool SymbolTable::define(SymbolPtr symbol) {
    if (!m_currentScope) return false;
    return m_currentScope->define(std::move(symbol));
}

SymbolPtr SymbolTable::lookup(const std::string& name) const {
    if (!m_currentScope) return nullptr;
    return m_currentScope->lookup(name);
}

SymbolPtr SymbolTable::lookupCurrentScope(const std::string& name) const {
    if (!m_currentScope) return nullptr;
    return m_currentScope->lookupLocal(name);
}

std::vector<std::shared_ptr<FunctionSymbol>> SymbolTable::lookupFunctions(const std::string& name) const {
    if (!m_currentScope) return {};
    return m_currentScope->lookupFunctions(name);
}

void SymbolTable::initBuiltins(TypeContext& typeCtx) {
    diagnostics::SourceLocation builtinLoc("<built-in>", 0, 0);

    // println overloads
    auto fnPrintLnStr = std::make_shared<FunctionSymbol>(
        "println",
        typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPrintLnStr);

    auto fnPrintLnInt = std::make_shared<FunctionSymbol>(
        "println",
        typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPrintLnInt);

    auto fnPrintLnDouble = std::make_shared<FunctionSymbol>(
        "println",
        typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPrintLnDouble);

    auto fnPrintLnBool = std::make_shared<FunctionSymbol>(
        "println",
        typeCtx.getFunctionType({typeCtx.getBool()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPrintLnBool);

    auto fnPrintLnAny = std::make_shared<FunctionSymbol>(
        "println",
        typeCtx.getFunctionType({typeCtx.getAny()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPrintLnAny);

    // print overloads
    auto fnPrintStr = std::make_shared<FunctionSymbol>(
        "print",
        typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPrintStr);

    auto fnPrintAny = std::make_shared<FunctionSymbol>(
        "print",
        typeCtx.getFunctionType({typeCtx.getAny()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPrintAny);

    // readln() -> string
    auto fnReadLn = std::make_shared<FunctionSymbol>(
        "readln",
        typeCtx.getFunctionType({}, typeCtx.getString()),
        builtinLoc
    );
    define(fnReadLn);

    // readLine() -> string
    auto fnReadLine = std::make_shared<FunctionSymbol>(
        "readLine",
        typeCtx.getFunctionType({}, typeCtx.getString()),
        builtinLoc
    );
    define(fnReadLine);

    // eprint(string) -> void, eprint(any) -> void
    auto fnEprintStr = std::make_shared<FunctionSymbol>(
        "eprint",
        typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnEprintStr);

    auto fnEprintAny = std::make_shared<FunctionSymbol>(
        "eprint",
        typeCtx.getFunctionType({typeCtx.getAny()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnEprintAny);

    // eprintln(string) -> void, eprintln(any) -> void
    auto fnEprintlnStr = std::make_shared<FunctionSymbol>(
        "eprintln",
        typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnEprintlnStr);

    auto fnEprintlnAny = std::make_shared<FunctionSymbol>(
        "eprintln",
        typeCtx.getFunctionType({typeCtx.getAny()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnEprintlnAny);

    // flush() -> void
    auto fnFlush = std::make_shared<FunctionSymbol>(
        "flush",
        typeCtx.getFunctionType({}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnFlush);

    // eflush() -> void
    auto fnEflush = std::make_shared<FunctionSymbol>(
        "eflush",
        typeCtx.getFunctionType({}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnEflush);

    // assert(bool) -> void
    auto fnAssert = std::make_shared<FunctionSymbol>(
        "assert",
        typeCtx.getFunctionType({typeCtx.getBool()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnAssert);

    // assert(bool, string) -> void
    auto fnAssertMsg = std::make_shared<FunctionSymbol>(
        "assert",
        typeCtx.getFunctionType({typeCtx.getBool(), typeCtx.getString()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnAssertMsg);

    // panic(string) -> void
    auto fnPanic = std::make_shared<FunctionSymbol>(
        "panic",
        typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getVoid()),
        builtinLoc
    );
    define(fnPanic);

    // len(any) -> int
    auto fnLen = std::make_shared<FunctionSymbol>(
        "len",
        typeCtx.getFunctionType({typeCtx.getAny()}, typeCtx.getInt()),
        builtinLoc
    );
    define(fnLen);
}

} // namespace sp::sema
