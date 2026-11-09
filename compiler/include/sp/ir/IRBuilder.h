#pragma once

#include "sp/ast/ASTVisitor.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Statement.h"
#include "sp/ast/Expression.h"
#include "sp/diagnostics/SourceLocation.h"
#include "sp/ir/IRModule.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/sema/ComptimeEvaluator.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

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
    // Phase 6 slice B: evaluate every non-static field initializer, in
    // declaration order, into the fresh object. Runs at the start of
    // every constructor (after the vtable pointer is set, before the
    // ctor body).
    void emitFieldInitializers(const ast::ClassDecl& classDecl, const codegen::ClassLayout& layout, IRValuePtr thisVal,
                               const std::vector<std::string>* skipTypeParams = nullptr);
    // Phase 6 slice B: generate __sp_static_init_<Class>(), which stores
    // every static field initializer into its module global. Called
    // once per class, right after its methods are generated; the
    // function name is recorded for the `main` prologue.
    void compileStaticInitializer(const ast::ClassDecl& classDecl, const codegen::ClassLayout& layout);
    // Phase 6 slice B: emit calls to every recorded static-init function.
    void emitStaticInitCalls();
    // Phase 6 slice B: append evaluated default argument values for any
    // declared parameters the caller omitted. `declArgOffset` is how
    // many leading loweredArgs are not declared parameters (1 when
    // `this` was pushed first, 0 for a plain static call). Defaults
    // come from the resolved call target's AST decl (methods included);
    // a default that names another parameter fails loudly via the
    // existing E1000 path (the name is unresolved at the call site).
    void appendDefaultArgs(const ast::CallExpr& node, std::vector<IRValuePtr>& loweredArgs,
                           size_t declArgOffset,
                           const std::vector<std::shared_ptr<ParameterValue>>& irParams);
    void compileMethod(const ast::ClassDecl& classDecl, const ast::FunctionDecl& methodDecl, const codegen::ClassLayout& layout);
    void compileSpecializedConstructor(const ast::FunctionDecl* ctorDecl, const std::vector<std::string>& typeParams, const std::vector<std::string>& typeArgs, const codegen::ClassLayout& layout, const ast::ClassDecl* classDecl = nullptr);
    void compileSpecializedMethod(const ast::FunctionDecl& methodDecl, const std::vector<std::string>& typeParams, const std::vector<std::string>& typeArgs, const codegen::ClassLayout& layout);
    void compileSpecializedFunction(const ast::FunctionDecl& fnDecl, const std::vector<std::string>& typeArgs);

    IRValuePtr emitMemberPointer(const ast::MemberExpr& node, IRTypePtr& outFieldType, IRValuePtr preEvaluatedObject = nullptr);
    // The zero/null value of a type: null for pointers, 0 for integers
    // and enums, false for bool, 0.0 for floats, zeroinitializer for
    // structs. Used by safe navigation (?.) for the null-object case.
    static IRValuePtr nullValueOf(IRTypePtr type);
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

    // Phase 3 slice C (pro-4.0.0): async spawn / await helpers, as
    // reworked by Phase 5 slice A (state machine). Calling an `async
    // fn` returns a Task instead of running the body: the body runs as
    // a posted callback on the scheduler. The body itself is the entry
    // point — `void <name>(ptr %task, ptr %ctx)` — it allocates a heap
    // frame, stores task + args, and calls the resume function
    // `void __sp_async_resume_<name>(ptr %frame)`, which holds the
    // dispatcher + one segment block per await point. Awaiting suspends:
    // the current segment stores its state and registers the next
    // segment as the awaited task's continuation (posted to the
    // scheduler, never called synchronously); the stack unwinds, and
    // the resume re-enters at the recorded state. Payload kinds are
    // "void" | "int" | "bool" | "string" | "double" | "array" |
    // "struct" (the Task payload the runtime can carry); anything else
    // (e.g. a class return) fails loudly at the declaration.
    // Name of the resume function for an async body.
    std::string asyncResumeName(const std::string& compiledName) const;
    // Unwrap Task<T>/Future<T> to its payload kind; false when the type
    // is not a Task/Future or the payload cannot be carried.
    bool taskPayloadKind(const sema::TypePtr& taskType, std::string& outKind) const;
    // Payload kind of an async fn's declared return type (sema type).
    bool asyncReturnKind(const sema::TypePtr& returnType, std::string& outKind) const;
    IRValuePtr emitAsyncScheduler();
    // Spawn `thunk` (a body-entry function value: a FunctionAddress for
    // a direct call, or a vtable-loaded pointer for a virtual call) as
    // a task: allocate a heap ctx, store the lowered args in 8-byte
    // slots, post the body via sp_rt_async_spawn_sm. Returns the task
    // handle (ptr), or nullptr when an argument type cannot be carried.
    // Unlike the Phase 3 spawn, this does NOT complete the task — the
    // body completes it in its finish block.
    IRValuePtr emitAsyncSpawnSM(IRValuePtr thunk,
                                const std::vector<IRValuePtr>& argValues,
                                const std::vector<IRTypePtr>& argTypes);
    // Await `task` (a Task handle) on `sched` and return its payload
    // value (nullptr for void). Does NOT destroy the task. The pump
    // path, kept as the fallback when no state machine is active (all
    // async fns use the state machine; see asyncSMBegin). `structTy`
    // is the IR struct type for a "struct" payload.
    IRValuePtr emitAwaitTask(IRValuePtr sched, IRValuePtr task, const std::string& payloadKind,
                             IRTypePtr structTy = nullptr);
    // Load a struct payload value out of a task's heap box (`box` may
    // be null when the task was cancelled — then the zero value is
    // returned instead of loading through null). Uses an entry-block
    // temp alloca + two branches + a merge load (no Phi nodes exist).
    IRValuePtr emitStructValueFromBox(IRValuePtr box, IRTypePtr structTy);
    // Param types for an async call, derived from sema (the declared
    // param types of the target + `this` for methods). The body's IR
    // function params are (task, ctx) after the Phase 5 rework, so call
    // sites must never derive param types from the body's signature.
    // `thisType` is the sema type of `this` for method calls.
    std::vector<IRTypePtr> asyncSpawnParamTypes(
        const std::vector<sema::TypePtr>& semaParamTypes,
        const sema::TypePtr& thisType = nullptr) const;
    // Drain timers/queued tasks at the end of main (ignores sockets):
    // fire-and-forget async tasks finish before the process exits.
    void emitMainExitPump();

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

    // Phase 3 slice D: the compile-time evaluator, built once per build()
    // from the whole program (it registers the `comptime fn` functions).
    // visit(ComptimeExpr) interprets the inner expression with it and
    // lowers the computed value as an ordinary literal.
    std::unique_ptr<sema::ComptimeEvaluator> m_comptimeEval;

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

    // Phase 3 slice B (pro-4.0.0): try/catch/finally + throw lowering.
    // A TryFrame records the control-flow targets of one lexically
    // enclosing `try` statement. A throw branches to the innermost
    // frame's catch (if any), else its finally (if any), else outward
    // to the next frame or the uncaught-throw block. A return inside a
    // try with a finally is deferred the same way: the value is stashed,
    // a flag is set, the finally blocks run, and the deferred-return
    // block completes the return.
    struct TryFrame {
        IRBasicBlock* catchBlock{nullptr};
        IRBasicBlock* finallyBlock{nullptr};
        IRBasicBlock* mergeBlock{nullptr};
        // Loop-nesting depths at try entry: a break/continue is "inside"
        // this try only when the loop stack has grown past it (a loop
        // entered after the try). A break/continue that would exit the
        // try is rejected loudly when the try has a finally.
        size_t breakDepth{0};
        size_t continueDepth{0};
        // Phase 5 slice B: refcounted-scope depth at try entry. A throw
        // leaving this try releases the refcounted locals of scopes
        // [refScopeDepth, top) -- the try body's own scopes and any
        // scopes nested inside it. Scopes below it (locals declared
        // before the try) stay live: the catch/finally cannot see the
        // try body's locals, but code after the try/catch can still
        // use the outer ones.
        size_t refScopeDepth{0};
    };
    std::vector<TryFrame> m_tryStack;

    // Per-function slots for the throw/return protocol, created lazily
    // in the entry block. m_tryStateFunction tracks which function they
    // belong to; ensureTryState() (called from setInsertPoint) resets
    // them whenever lowering moves to a new function.
    IRFunction* m_tryStateFunction{nullptr};
    IRValuePtr m_throwPayloadSlot{nullptr}; // ptr: stringified thrown value
    IRValuePtr m_throwPendingSlot{nullptr}; // i1: a throw is in flight
    IRValuePtr m_returnPendingSlot{nullptr}; // i1: a return is deferred
    IRValuePtr m_returnValueSlot{nullptr}; // function return type (non-void)
    IRBasicBlock* m_deferredReturnBlock{nullptr};
    IRBasicBlock* m_uncaughtThrowBlock{nullptr};
    // Phase 5 slice B: typed + composite throws. The payload slot
    // above still carries the stringified value (the panic message and
    // the catch-all binding); these carry the rest:
    // - m_throwTypeSlot: i32 tag of the thrown value's static type
    //   (canonical Type::toString(), assigned on demand by
    //   throwTypeTagFor; tag 0 = unknown/error type, matches nothing).
    // - m_throwRawSlot: i64 bit-preserving raw value (scalars packed
    //   by emitPackThrowScalar, pointers via PtrToInt). The catch
    //   unpacks it back into the clause's type.
    // - m_throwOwnsSlot: i1, set when the payload holds the unique
    //   owning reference to a class object (a directly-thrown `new`
    //   transfers its count-1; any other class value is retained into
    //   the payload at the throw site). The consumer (matching catch
    //   binding, catch-all discard, or the uncaught handler) releases
    //   that share; a borrowed value is retained by its new owner
    //   instead.
    // - m_throwStructSlots: by-value struct throws (user structs) are
    //   copied into a per-type alloca, keyed by canonical type name.
    //   (Result/Option are deliberately NOT throwable: they are the
    //   return-value error channel, ADR-002 direction.)
    IRValuePtr m_throwTypeSlot{nullptr};
    IRValuePtr m_throwRawSlot{nullptr};
    IRValuePtr m_throwOwnsSlot{nullptr};
    std::unordered_map<std::string, IRValuePtr> m_throwStructSlots;
    std::unordered_map<std::string, int32_t> m_throwTypeTags;
    int32_t m_nextThrowTypeTag{1};
    void ensureTryState();

    IRValuePtr getOrCreateThrowPayloadSlot();
    IRValuePtr getOrCreateThrowPendingSlot();
    IRValuePtr getOrCreateReturnPendingSlot();
    IRValuePtr getOrCreateReturnValueSlot();
    IRValuePtr getOrCreateThrowTypeSlot();
    IRValuePtr getOrCreateThrowRawSlot();
    IRValuePtr getOrCreateThrowOwnsSlot();
    IRValuePtr getOrCreateThrowStructSlot(const std::string& canonicalType, IRTypePtr structType);
    // Tag for a canonical thrown-type name, assigned on first use and
    // stable for the whole compilation (one IRBuilder per program).
    int32_t throwTypeTagFor(const std::string& canonicalType);
    // Tags a catch clause matches: its own tag plus, for a
    // class/interface clause, the tags of every user class that is a
    // subtype of it (a thrown Dog matches `catch (Animal e)`). Empty
    // means the clause is a catch-all (unresolvable name, incl.
    // `Exception`, or `Any`).
    std::vector<int32_t> catchClauseTags(const ast::CatchClause& clause);
    // Innermost frame with a catch or a finally, or nullptr.
    const TryFrame* findThrowFrame();
    // Release refcounted locals in scopes [fromDepth, top): the
    // unwinding drop for a throw leaving the try at fromDepth.
    void emitThrowScopeDrops(size_t fromDepth);
    // Bit-preserving pack of a scalar IR value into i64 (ints: extend,
    // bool/char: zero-extend, float: bitcast via i32, double: bitcast).
    IRValuePtr emitPackThrowScalar(IRValuePtr val, IRTypePtr valType);
    // Unpack the raw i64 slot back into the clause's IR type
    // (pointer: IntToPtr, integer: Trunc, float: Trunc+BitCast,
    // double: BitCast). Struct clauses do not use this: their value
    // comes from the per-type struct slot.
    IRValuePtr emitUnpackThrowValue(IRValuePtr rawI64, IRTypePtr clauseType);
    IRBasicBlock* getOrCreateDeferredReturnBlock();
    IRBasicBlock* getOrCreateUncaughtThrowBlock();
    // Walk the try stack outward: first frame with a catch, else first
    // with a finally, else the uncaught-throw block.
    IRBasicBlock* findThrowTarget();
    // First frame (outward) with a finally, or nullptr.
    IRBasicBlock* findFinallyTarget();
    // Dispatch at the end of a finally body: pending throw propagates,
    // else pending return completes, else fall through to mergeBlock.
    void emitFinallyDispatch(IRBasicBlock* mergeBlock);
    // Set the throw slots for a thrown value and branch to its handler
    // (findThrowTarget), dropping the unwound scopes first. strVal is
    // the stringified value (payload slot); rawI64 the bit-packed raw
    // value (null for struct throws, whose value lives in the per-type
    // struct slot); typeTag the thrown type's tag (0 = unknown);
    // owns whether the payload holds an owning class reference.
    void emitThrowValue(IRValuePtr strVal, IRValuePtr rawI64, int32_t typeTag, bool owns);

    // Phase 2 slice B2: true while lowering the body of an `async fn`
    // (set in visit(FunctionDecl) / compileMethod). visit(AwaitExpr)
    // only lowers inside an async function; sema normally rejects await
    // elsewhere (E1802) first, but IRBuilder must not silently accept it
    // if sema is ever bypassed.
    bool m_inAsyncFunction{false};

    // Phase 5 slice A (pro-4.0.0): heap-frame state machine for async
    // functions. While lowering an async body, m_asyncSM holds the
    // frame layout (one 8-byte slot per reserved cell / param / local,
    // keyed by AST node or reserved index), the resume function, and
    // the per-await continuation blocks. Locals are addressed through
    // frame-slot GEPs instead of entry-block allocas, so they survive
    // across await points: each await stores the next state, registers
    // the resume as the awaited task's continuation, and returns; the
    // dispatcher (entry block of the resume function) re-enters at the
    // recorded state. Null outside an async body being lowered.
    struct AsyncSM {
        // Frame layout (8-byte slots, byte offsets):
        //   [0] state (i64) [1] task (ptr) [2] awaitTask (ptr)
        //   [3] retVal (payload type) [4] returnPending (i1)
        //   [5] throwPending (i1) [6] throwPayload (ptr)
        //   [7..] params (`this` first for methods), then locals.
        static constexpr int64_t kStateSlot = 0;
        static constexpr int64_t kTaskSlot = 1;
        static constexpr int64_t kAwaitTaskSlot = 2;
        static constexpr int64_t kRetValSlot = 3;
        static constexpr int64_t kReturnPendingSlot = 4;
        static constexpr int64_t kThrowPendingSlot = 5;
        static constexpr int64_t kThrowPayloadSlot = 6;
        static constexpr int64_t kFirstParamSlot = 7;
        IRFunction* bodyFn{nullptr};    // void (ptr task, ptr ctx)
        IRFunction* resumeFn{nullptr};  // void (ptr frame)
        IRBasicBlock* dispatcherBB{nullptr}; // entry block of resumeFn
        IRBasicBlock* segment0BB{nullptr};   // first segment (state 0)
        IRBasicBlock* finishBB{nullptr};     // completes the task, frees frame
        IRValuePtr frameParam{nullptr};      // resumeFn's %frame parameter
        int64_t frameSize{0};               // bytes; frameSize/8 slots
        int64_t nextSlot{kFirstParamSlot};
        // Param names (including "this" as [0] for methods) and their
        // IR types (for the body prologue's ctx loads), parallel.
        std::vector<std::string> paramNames;
        std::vector<IRTypePtr> paramIRTypes;
        // Slots keyed by AST node: params by their FunctionDecl param
        // index, locals by VarDeclStmt / ForStmt / ForInStmt node.
        std::unordered_map<const void*, int64_t> nodeSlots;
        // Param slots by index (0-based over the sema param list,
        // `this` first for methods).
        std::unordered_map<int, int64_t> paramSlots;
        // Continuation blocks, one per await (state = index + 1).
        std::vector<IRBasicBlock*> continuations;
        // Counter for hoisted await temps (__sp_await_N).
        int64_t nextHoistId{0};
        // Auxiliary frame slots for compiler-generated loop state
        // (the for-in induction variable), keyed by (AST node, tag).
        // The loop var itself uses nodeSlots (keyed by the ForInStmt
        // node); the induction variable uses auxSlots with tag 0.
        std::map<std::pair<const void*, int64_t>, int64_t> auxSlots;
        // The return payload kind ("void"|"int"|"bool"|"string"|
        // "double"|"array"|"struct") and, for struct returns, the IR
        // struct type (for the heap box size/alignment).
        std::string payloadKind;
        IRTypePtr structType{nullptr};
        // Frame-slot GEPs are appended to the entry block during
        // lowering (they land before the dispatcher terminator); this
        // caches them by slot so each slot gets exactly one GEP.
        std::unordered_map<int64_t, IRValuePtr> slotGEPs;
    };
    std::unique_ptr<AsyncSM> m_asyncSM;
    // True while lowering an async body with the state machine (i.e.
    // m_asyncSM is set AND the current function is its resume fn).
    bool inAsyncSM() const { return m_asyncSM && m_currentFunction == m_asyncSM->resumeFn; }
    // Byte offset of a frame slot.
    static int64_t asyncFrameOffset(int64_t slot) { return slot * 8; }
    // GEP for a frame slot, cached per slot (appended to the entry
    // block; lands before the dispatcher terminator).
    IRValuePtr asyncFrameSlotGEP(int64_t slot);
    // Load/store helpers for frame slots.
    IRValuePtr asyncFrameLoad(int64_t slot, IRTypePtr ty, const std::string& name);
    void asyncFrameStore(int64_t slot, IRValuePtr val);
    // Pre-scan an async body: assign frame slots to every local
    // (VarDeclStmt / ForStmt / ForInStmt) in the body, hoist extra
    // awaits out of statements that contain more than one, and return
    // the number of await expressions found (or -1 after reporting
    // E1000). Also validates that the declared return / param types
    // can be carried (E1000 otherwise). Creates m_asyncSM. Short-form
    // bodies are converted to block form first (a ReturnStmt).
    int64_t asyncPrepareBody(ast::FunctionDecl& decl, const std::string& compiledName,
                             const sema::TypePtr& returnType,
                             const std::vector<sema::TypePtr>& semaParamTypes,
                             const sema::TypePtr& thisType);
    // Pre-scan helpers (called by asyncPrepareBody). The scan walks an
    // async body BEFORE lowering: it assigns a frame slot to every
    // local (VarDeclStmt / ForStmt init / ForInStmt var, keyed by AST
    // node address) and rewrites statements that contain more than
    // one await (or one await that is not first-evaluated — see the
    // hoisting rule in the Phase 34 slice A doc) by hoisting the extra
    // awaits into `let __sp_await_N = await ...;` statements before
    // the enclosing statement. Awaits inside a lambda body are never
    // descended into (a lambda is a separate function).
    // A struct value occupies more than one 8-byte frame slot, so the
    // pre-scan must know each local's size. These helpers compute how
    // many slots a local needs (1 for everything except a struct
    // value, which needs ceil(size/8)).
    int64_t asyncAssignLocalSlot(const void* node, int64_t slotCount = 1);
    // Auxiliary slot for compiler-generated loop state (the for-in
    // induction variable), keyed by (AST node, tag).
    int64_t asyncAuxSlot(const void* node, int64_t tag, int64_t slotCount = 1);
    // Resolve a type string through sema. `m_sema` is held by a
    // const reference (sema is complete by codegen time), but
    // TypeContext::resolveType is non-const (it populates a cache);
    // the const_cast is confined to this helper.
    sema::TypePtr asyncResolveType(const std::string& typeStr);
    // The IR struct type for a `Task<StructName>`'s inner struct
    // (nullptr when the inner type is not a struct).
    IRTypePtr asyncStructTypeForTask(const sema::TypePtr& taskType);
    // The IR type of an awaited value, by payload kind.
    IRTypePtr asyncAwaitedIRType(const std::string& kind, IRTypePtr structTy);
    // Frame-slot GEP for a pre-scanned AST node (a local / loop var).
    // Reports E1000 and returns an entry-block alloca when the node
    // was never scanned (which cannot happen for nodes in the body —
    // the pre-scan covers the whole body before lowering).
    IRValuePtr asyncNodeSlotGEP(const void* node, IRTypePtr fallbackType,
                                const diagnostics::SourceLocation& loc);
    // IR struct type for a sema struct / tagged-union enum type
    // (nullptr when the type is not a struct value).
    IRTypePtr asyncStructTypeFor(const sema::TypePtr& semaType);
    int64_t asyncSlotCountForType(const sema::TypePtr& semaType);
    int64_t asyncVarDeclSlotCount(const ast::VarDeclStmt& node);
    int64_t asyncForInSlotCount(const ast::ForInStmt& node);
    void asyncCollectAwaits(const ast::ExpressionNode& e,
                            std::vector<const ast::AwaitExpr*>& out);
    const ast::AwaitExpr* asyncFirstAwait(const ast::ExpressionNode& e);
    bool asyncHoistAwaits(ast::ExprPtr& slot, const ast::AwaitExpr* keep,
                          std::vector<ast::StmtPtr>& out);
    int64_t asyncScanBlock(ast::BlockStmt& block);
    // Process a statement list (a block's statements, or a switch
    // case's): recurse into nested statements, assign local slots,
    // hoist extra awaits (inserting the hoisted lets before their
    // statement). Returns the await count, or -1 after E1000.
    int64_t asyncScanStmtList(std::vector<ast::StmtPtr>& stmts);
    // Recurse into a statement's nested statements/blocks (assigning
    // slots, scanning their blocks). Returns the await count found.
    int64_t asyncScanNested(ast::StatementNode& stmt);
    // Hoist awaits in a statement's OWN expressions (not nested
    // statements). `insert` receives the hoisted `let` statements to
    // place before this statement (nullptr = hoisting is not allowed
    // here — an await in such a position is E1000). Returns the await
    // count found, or -1 after reporting E1000.
    int64_t asyncScanStmtExprs(ast::StatementNode& stmt,
                               std::vector<ast::StmtPtr>* insert);
    // Begin/end the state machine for one async body (asyncPrepareBody
    // must have run first and created m_asyncSM): asyncSMBegin creates
    // the resume function + dispatcher + segment 0 + finish block,
    // declares the params as frame slots, and moves lowering into the
    // resume function; asyncSMEnd emits the dispatcher chain, the
    // finish block, and the body entry prologue, then clears the state.
    void asyncSMBegin(const std::string& compiledName, IRFunction* bodyFn);
    void asyncSMEnd();
    // Size in bytes of an IR struct type (x86-64 layout over its
    // element types), for heap-boxing a struct payload.
    static int64_t asyncStructSizeBytes(const IRTypePtr& structTy);
    // Suspend at an await: store the next state + the awaited task,
    // register the resume as the awaited task's continuation (posted,
    // never synchronous), and return from the current segment. On
    // resume, the continuation first checks our own task's
    // cancellation (branches to the finish block when cancelled —
    // the body stops at its next await point), then loads the awaited
    // value with the non-blocking getters (the awaited task is already
    // complete). Returns the value (nullptr for void). `structTy` is
    // the IR struct type for a "struct" payload. When `destroyTask`
    // is true, the (temporary) task is destroyed after its value has
    // been copied out — for an array the pointer outlives the task by
    // design, for a string the getter returns a fresh copy, and for a
    // struct the value was loaded by value.
    IRValuePtr asyncSuspendAndLoad(IRValuePtr task, const std::string& kind,
                                   IRTypePtr structTy = nullptr, bool destroyTask = false);

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
    // Phase 6 slice B: per-class static-init functions, in class
    // declaration order; called from `main` before any user code runs.
    std::vector<std::string> m_staticInitFunctions;
};

} // namespace sp::ir
