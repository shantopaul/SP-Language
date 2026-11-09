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

    // Built-in math functions and constants
    define(std::make_shared<VariableSymbol>("PI", typeCtx.getDouble(), builtinLoc, true));
    define(std::make_shared<VariableSymbol>("E", typeCtx.getDouble(), builtinLoc, true));
    define(std::make_shared<VariableSymbol>("TAU", typeCtx.getDouble(), builtinLoc, true));

    define(std::make_shared<FunctionSymbol>("sqrt", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("pow", typeCtx.getFunctionType({typeCtx.getDouble(), typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("sin", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("cos", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("tan", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("asin", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("acos", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("atan", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("atan2", typeCtx.getFunctionType({typeCtx.getDouble(), typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("floor", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("ceil", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("round", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));

    auto fnAbsInt = std::make_shared<FunctionSymbol>("abs", typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getInt()), builtinLoc);
    auto fnAbsDbl = std::make_shared<FunctionSymbol>("abs", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc);
    fnAbsInt->addOverload(fnAbsDbl);
    define(fnAbsInt);

    auto fnMinInt = std::make_shared<FunctionSymbol>("min", typeCtx.getFunctionType({typeCtx.getInt(), typeCtx.getInt()}, typeCtx.getInt()), builtinLoc);
    auto fnMinDbl = std::make_shared<FunctionSymbol>("min", typeCtx.getFunctionType({typeCtx.getDouble(), typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc);
    fnMinInt->addOverload(fnMinDbl);
    define(fnMinInt);

    auto fnMaxInt = std::make_shared<FunctionSymbol>("max", typeCtx.getFunctionType({typeCtx.getInt(), typeCtx.getInt()}, typeCtx.getInt()), builtinLoc);
    auto fnMaxDbl = std::make_shared<FunctionSymbol>("max", typeCtx.getFunctionType({typeCtx.getDouble(), typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc);
    fnMaxInt->addOverload(fnMaxDbl);
    define(fnMaxInt);

    define(std::make_shared<FunctionSymbol>("log", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("log10", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("log2", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("exp", typeCtx.getFunctionType({typeCtx.getDouble()}, typeCtx.getDouble()), builtinLoc));

    // Built-in filesystem functions (std.fs)
    define(std::make_shared<FunctionSymbol>("readText", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getString()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("writeText", typeCtx.getFunctionType({typeCtx.getString(), typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("appendText", typeCtx.getFunctionType({typeCtx.getString(), typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("exists", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("deleteFile", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("delete", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("createDir", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("listDir", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getArrayType(typeCtx.getString())), builtinLoc));
    define(std::make_shared<FunctionSymbol>("isFile", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("isDir", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getBool()), builtinLoc));

    auto resultStrIO = typeCtx.resolveType("Result<string, IOError>");
    auto resultBoolIO = typeCtx.resolveType("Result<bool, IOError>");
    if (resultStrIO) {
        define(std::make_shared<FunctionSymbol>("tryReadText", typeCtx.getFunctionType({typeCtx.getString()}, resultStrIO), builtinLoc));
        define(std::make_shared<FunctionSymbol>("tryReadLine", typeCtx.getFunctionType({}, resultStrIO), builtinLoc));
    }
    if (resultBoolIO) {
        define(std::make_shared<FunctionSymbol>("tryWriteText", typeCtx.getFunctionType({typeCtx.getString(), typeCtx.getString()}, resultBoolIO), builtinLoc));
        define(std::make_shared<FunctionSymbol>("tryAppendText", typeCtx.getFunctionType({typeCtx.getString(), typeCtx.getString()}, resultBoolIO), builtinLoc));
        define(std::make_shared<FunctionSymbol>("tryDelete", typeCtx.getFunctionType({typeCtx.getString()}, resultBoolIO), builtinLoc));
        define(std::make_shared<FunctionSymbol>("tryCreateDir", typeCtx.getFunctionType({typeCtx.getString()}, resultBoolIO), builtinLoc));
    }

    // Built-in time functions (std.time)
    define(std::make_shared<FunctionSymbol>("now", typeCtx.getFunctionType({}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("nowSeconds", typeCtx.getFunctionType({}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("sleep", typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getVoid()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("sleepSeconds", typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getVoid()), builtinLoc));

    // Built-in async functions (Phase 2 slice B2). These return Task<T>
    // and are the ONLY expressions `await` can lower (see
    // IRBuilder::visit(AwaitExpr)). sleep_async is the non-blocking
    // counterpart of sleep: it returns immediately and completes after
    // the delay on the event loop. delayed_value completes with `value`
    // after the delay, so `await` can yield a value.
    auto taskVoid = typeCtx.resolveType("Task<void>");
    auto taskInt = typeCtx.resolveType("Task<int>");
    auto taskString = typeCtx.resolveType("Task<string>");
    auto taskBool = typeCtx.resolveType("Task<bool>");
    if (taskVoid) {
        define(std::make_shared<FunctionSymbol>("sleep_async", typeCtx.getFunctionType({typeCtx.getInt()}, taskVoid), builtinLoc));
    }
    if (taskInt) {
        define(std::make_shared<FunctionSymbol>("delayed_value", typeCtx.getFunctionType({typeCtx.getInt(), typeCtx.getInt()}, taskInt), builtinLoc));
    }
    // Phase 3 slice C: delayed_value overloads for the other awaitable
    // payload kinds (string via sp_rt_async_delayed_str, bool via the
    // i64 task with a truncating cast at the await site).
    if (taskString) {
        define(std::make_shared<FunctionSymbol>("delayed_value", typeCtx.getFunctionType({typeCtx.getInt(), typeCtx.getString()}, taskString), builtinLoc));
    }
    if (taskBool) {
        define(std::make_shared<FunctionSymbol>("delayed_value", typeCtx.getFunctionType({typeCtx.getInt(), typeCtx.getBool()}, taskBool), builtinLoc));
    }
    define(std::make_shared<FunctionSymbol>("formatTime", typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getString()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("parseTime", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getInt()), builtinLoc));

    // Built-in OS functions (std.os)
    define(std::make_shared<FunctionSymbol>("getEnv", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getString()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("setEnv", typeCtx.getFunctionType({typeCtx.getString(), typeCtx.getString()}, typeCtx.getBool()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("platform", typeCtx.getFunctionType({}, typeCtx.getString()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("arch", typeCtx.getFunctionType({}, typeCtx.getString()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("args", typeCtx.getFunctionType({}, typeCtx.getArrayType(typeCtx.getString())), builtinLoc));
    define(std::make_shared<FunctionSymbol>("exit", typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getVoid()), builtinLoc));

    // Built-in process functions (std.process)
    define(std::make_shared<FunctionSymbol>("exec", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("spawn", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("runCommand", typeCtx.getFunctionType({typeCtx.getString()}, typeCtx.getString()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("pid", typeCtx.getFunctionType({}, typeCtx.getInt()), builtinLoc));

    // Built-in memory and arena functions (std.mem)
    define(std::make_shared<FunctionSymbol>("totalAllocatedBytes", typeCtx.getFunctionType({}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("totalLiveObjects", typeCtx.getFunctionType({}, typeCtx.getInt()), builtinLoc));

    // Built-in concurrency functions (std.sync)
    define(std::make_shared<FunctionSymbol>("threadSpawn", typeCtx.getFunctionType({typeCtx.getInt(), typeCtx.getInt()}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("threadJoin", typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getVoid()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("threadDetach", typeCtx.getFunctionType({typeCtx.getInt()}, typeCtx.getVoid()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("threadId", typeCtx.getFunctionType({}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("hardwareConcurrency", typeCtx.getFunctionType({}, typeCtx.getInt()), builtinLoc));
    define(std::make_shared<FunctionSymbol>("threadYield", typeCtx.getFunctionType({}, typeCtx.getVoid()), builtinLoc));
}

} // namespace sp::sema
