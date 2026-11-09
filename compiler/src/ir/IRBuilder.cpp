#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRVerifier.h"
#include "sp/codegen/VTableBuilder.h"
#include "sp/codegen/ClassGenerator.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace sp::ir {

static IRValuePtr makeDefaultValue(const IRTypePtr& type) {
    if (!type || type->isVoid()) return nullptr;
    if (type->isStruct() || type->isPointer()) {
        return std::make_shared<ConstantNull>(type);
    } else if (type->isFloatingPoint()) {
        return std::make_shared<ConstantFloat>(type, 0.0);
    } else {
        return std::make_shared<ConstantInt>(type, 0);
    }
}

void IRBuilder::emitPopulateTaggedUnion(IRValuePtr slotPtr, const std::string& typeName, int32_t tag, IRValuePtr payloadVal) {
    std::string structName = typeName;
    if (structName.rfind("Result", 0) == 0) structName = "Result";
    else if (structName.rfind("Option", 0) == 0) structName = "Option";
    auto structType = ir::IRType::getStruct("struct." + structName, {IRType::getInt32(), IRType::getInt64()});

    auto tagPtr = std::make_shared<GetElementPtrInst>(
        structType,
        slotPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
        },
        m_currentFunction->nextTempName("tag_ptr")
    );
    m_currentBlock->addInstruction(tagPtr);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantInt>(IRType::getInt32(), tag),
        tagPtr
    ));

    IRValuePtr payloadI64 = nullptr;
    if (payloadVal) {
        payloadI64 = emitCastIfNeeded(payloadVal, IRType::getInt64());
    } else {
        payloadI64 = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
    }
    auto payloadPtr = std::make_shared<GetElementPtrInst>(
        structType,
        slotPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 1)
        },
        m_currentFunction->nextTempName("payload_ptr")
    );
    m_currentBlock->addInstruction(payloadPtr);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(payloadI64, payloadPtr));
}

IRValuePtr IRBuilder::emitCreateTaggedUnion(const std::string& typeName, int32_t tag, IRValuePtr payloadVal) {
    std::string structName = typeName;
    if (structName.rfind("Result", 0) == 0) structName = "Result";
    else if (structName.rfind("Option", 0) == 0) structName = "Option";
    auto structType = ir::IRType::getStruct("struct." + structName, {IRType::getInt32(), IRType::getInt64()});

    std::string allocaReg = m_currentFunction->nextTempName("tu_alloca");
    auto allocaInst = std::make_shared<AllocaInst>(structType, allocaReg);
    m_currentFunction->getEntryBlock()->addInstruction(allocaInst);

    emitPopulateTaggedUnion(allocaInst, structName, tag, payloadVal);
    return allocaInst;
}

IRBuilder::IRBuilder(const sema::SemanticAnalyzer& sema)
    : m_sema(sema) {}

IRBuilder::~IRBuilder() = default;

std::unique_ptr<IRModule> IRBuilder::build(const ast::Program& program, const std::string& moduleName) {
    m_errors.clear();
    m_module = std::make_unique<IRModule>(moduleName);
    m_blockCounter = 0;
    m_varCounter = 0;
    m_scopes.clear();
    m_breakStack.clear();
    m_continueStack.clear();
    m_currentClass = nullptr;

    // Build VTables and Class layouts
    m_vtableBuilder = std::make_unique<codegen::VTableBuilder>(m_sema);
    m_vtableBuilder->buildProgramLayouts(program, *m_module);

    // Phase 3 slice D: build the compile-time evaluator registry from the
    // whole program before lowering, so visit(ComptimeExpr) can evaluate
    // comptime expressions and fold the computed constants into the IR.
    m_comptimeEval = std::make_unique<sema::ComptimeEvaluator>(nullptr);
    m_comptimeEval->processProgram(program);

    visit(program);

    // Phase 1 (pro-4.0.0): fail loudly instead of silently dropping
    // semantics. Constructs with no lowering record errors while visiting;
    // the finished module is then verified. Either failure aborts
    // compilation: every caller (spc, spm, TestRunner, --watch, the
    // setup smoke test) treats a null module as a failed build.
    if (!m_errors.empty()) {
        for (const auto& error : m_errors) {
            std::cerr << error << '\n';
        }
        return nullptr;
    }

    IRVerifier verifier;
    if (!verifier.verify(*m_module)) {
        for (const auto& error : verifier.getErrors()) {
            std::cerr << "error[E1000]: IR verification failed: " << error << '\n';
        }
        return nullptr;
    }

    return std::move(m_module);
}

void IRBuilder::reportUnsupported(const diagnostics::SourceLocation& loc, const std::string& construct,
                                  const std::string& detail) {
    m_errors.push_back("error[E1000]: '" + construct + "' is not yet supported in this phase of the SP compiler (" +
                       detail + ") --> " + loc.toString());
}

bool IRBuilder::checkStringArgs(const ast::CallExpr& node, const std::vector<size_t>& argIndices,
                                const std::string& methodName) {
    for (size_t idx : argIndices) {
        if (idx >= node.getArgs().size()) continue;
        auto argType = m_sema.getNodeType(node.getArgs()[idx].get());
        if (!argType || argType->isError()) continue;
        if (argType->isString() || argType->isPointer()) continue;
        reportUnsupported(node.getLocation(),
                          methodName + " with a '" + argType->toString() + "' argument",
                          "string methods take string arguments; a '" + argType->toString() +
                          "' value passed as a string pointer is not supported in this phase");
        return false;
    }
    return true;
}

// --- Phase 3 slice B: try/catch/finally + throw protocol -----------------
// All slots live in the entry block and are shared by every try in one
// function: only one throw (and one deferred return) can be in flight at
// a time, so a single slot each is enough.

IRValuePtr IRBuilder::getOrCreateThrowPayloadSlot() {
    // Phase 5 slice A: in the state machine the throw/return protocol
    // slots live in the heap frame (they must survive across awaits),
    // not in entry-block allocas.
    if (inAsyncSM()) {
        if (!m_throwPayloadSlot) m_throwPayloadSlot = asyncFrameSlotGEP(AsyncSM::kThrowPayloadSlot);
        return m_throwPayloadSlot;
    }
    if (!m_throwPayloadSlot) {
        auto alloca = std::make_shared<AllocaInst>(IRType::getPointer(IRType::getInt8()),
                                                   m_currentFunction->nextTempName("throw_payload"));
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        m_throwPayloadSlot = alloca;
    }
    return m_throwPayloadSlot;
}

IRValuePtr IRBuilder::getOrCreateThrowPendingSlot() {
    if (inAsyncSM()) {
        if (!m_throwPendingSlot) m_throwPendingSlot = asyncFrameSlotGEP(AsyncSM::kThrowPendingSlot);
        return m_throwPendingSlot;
    }
    if (!m_throwPendingSlot) {
        auto alloca = std::make_shared<AllocaInst>(IRType::getInt1(),
                                                   m_currentFunction->nextTempName("throw_pending"));
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        m_currentFunction->getEntryBlock()->addInstruction(std::make_shared<StoreInst>(
            std::make_shared<ConstantInt>(IRType::getInt1(), 0), alloca));
        m_throwPendingSlot = alloca;
    }
    return m_throwPendingSlot;
}

// --- Phase 5 slice B: typed + composite throw slots ----------------------
// (See IRBuilder.h for the design: type tag, bit-packed raw value,
// owning-reference flag, per-type struct slots.)

IRValuePtr IRBuilder::getOrCreateThrowTypeSlot() {
    if (!m_throwTypeSlot) {
        auto alloca = std::make_shared<AllocaInst>(IRType::getInt32(),
                                                   m_currentFunction->nextTempName("throw_type"));
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        m_throwTypeSlot = alloca;
    }
    return m_throwTypeSlot;
}

IRValuePtr IRBuilder::getOrCreateThrowRawSlot() {
    if (!m_throwRawSlot) {
        auto alloca = std::make_shared<AllocaInst>(IRType::getInt64(),
                                                   m_currentFunction->nextTempName("throw_raw"));
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        m_throwRawSlot = alloca;
    }
    return m_throwRawSlot;
}

IRValuePtr IRBuilder::getOrCreateThrowOwnsSlot() {
    if (!m_throwOwnsSlot) {
        auto alloca = std::make_shared<AllocaInst>(IRType::getInt1(),
                                                   m_currentFunction->nextTempName("throw_owns"));
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        m_currentFunction->getEntryBlock()->addInstruction(std::make_shared<StoreInst>(
            std::make_shared<ConstantInt>(IRType::getInt1(), 0), alloca));
        m_throwOwnsSlot = alloca;
    }
    return m_throwOwnsSlot;
}

IRValuePtr IRBuilder::getOrCreateThrowStructSlot(const std::string& canonicalType, IRTypePtr structType) {
    auto it = m_throwStructSlots.find(canonicalType);
    if (it != m_throwStructSlots.end()) return it->second;
    auto alloca = std::make_shared<AllocaInst>(structType,
                                               m_currentFunction->nextTempName("throw_struct"));
    m_currentFunction->getEntryBlock()->addInstruction(alloca);
    m_throwStructSlots.emplace(canonicalType, alloca);
    return alloca;
}

int32_t IRBuilder::throwTypeTagFor(const std::string& canonicalType) {
    auto it = m_throwTypeTags.find(canonicalType);
    if (it != m_throwTypeTags.end()) return it->second;
    int32_t tag = m_nextThrowTypeTag++;
    m_throwTypeTags.emplace(canonicalType, tag);
    return tag;
}

std::vector<int32_t> IRBuilder::catchClauseTags(const ast::CatchClause& clause) {
    const std::string& name = clause.getResolvedTypeName();
    // Catch-all: unresolvable name (incl. the conventional `Exception`
    // spelling, which has no type in the language) or `Any`.
    if (name.empty() || name == "any" || name == "Exception") return {};
    std::vector<int32_t> tags;
    tags.push_back(throwTypeTagFor(name));
    // Class/interface clause: also match every user class that is a
    // subtype of it (a thrown Dog matches `catch (Animal e)`). The
    // match is on the thrown expression's *static* type: a variable
    // statically typed as a base class never matches a derived-class
    // clause, even if it holds a derived instance at runtime.
    auto clauseType = m_sema.getTypeContext().findUserType(name);
    if (clauseType && (clauseType->isClass() || clauseType->isInterface())) {
        for (const auto& [userName, userType] : m_sema.getTypeContext().getUserTypes()) {
            if (!userType || !userType->isClass() || userName == name) continue;
            if (userType->isSubtypeOf(*clauseType)) {
                tags.push_back(throwTypeTagFor(userName));
            }
        }
    }
    return tags;
}

const IRBuilder::TryFrame* IRBuilder::findThrowFrame() {
    for (auto it = m_tryStack.rbegin(); it != m_tryStack.rend(); ++it) {
        if (it->catchBlock || it->finallyBlock) return &(*it);
    }
    return nullptr;
}

void IRBuilder::emitThrowScopeDrops(size_t fromDepth) {
    if (!m_currentBlock || m_currentBlock->hasTerminator()) return;
    for (int i = static_cast<int>(m_refCountedScopes.size()) - 1; i >= static_cast<int>(fromDepth); --i) {
        emitScopeDrops(static_cast<size_t>(i));
    }
}

IRValuePtr IRBuilder::emitPackThrowScalar(IRValuePtr val, IRTypePtr valType) {
    if (!val || !valType) return nullptr;
    if (valType->isFloat()) {
        // Bit-preserving: float -> i32 bits -> zero-extend to i64.
        auto bits = std::make_shared<CastInst>(IROpcode::BitCast, val, IRType::getInt32(),
                                               m_currentFunction->nextTempName("throw_fbits"));
        if (m_currentBlock) m_currentBlock->addInstruction(bits);
        auto wide = std::make_shared<CastInst>(IROpcode::ZExt, bits, IRType::getInt64(),
                                               m_currentFunction->nextTempName("throw_raw"));
        if (m_currentBlock) m_currentBlock->addInstruction(wide);
        return wide;
    }
    if (valType->isDouble()) {
        auto bits = std::make_shared<CastInst>(IROpcode::BitCast, val, IRType::getInt64(),
                                               m_currentFunction->nextTempName("throw_raw"));
        if (m_currentBlock) m_currentBlock->addInstruction(bits);
        return bits;
    }
    // Integers (incl. bool i1, char i8): extend to i64. Truncation on
    // unpack restores the exact bits.
    return emitCastIfNeeded(val, IRType::getInt64());
}

IRValuePtr IRBuilder::emitUnpackThrowValue(IRValuePtr rawI64, IRTypePtr clauseType) {
    if (!rawI64 || !clauseType) return rawI64;
    if (clauseType->isPointer()) {
        auto ptr = std::make_shared<CastInst>(IROpcode::IntToPtr, rawI64, clauseType,
                                              m_currentFunction->nextTempName("catch_val"));
        if (m_currentBlock) m_currentBlock->addInstruction(ptr);
        return ptr;
    }
    if (clauseType->isDouble()) {
        auto dbl = std::make_shared<CastInst>(IROpcode::BitCast, rawI64, IRType::getDouble(),
                                              m_currentFunction->nextTempName("catch_val"));
        if (m_currentBlock) m_currentBlock->addInstruction(dbl);
        return dbl;
    }
    if (clauseType->isFloat()) {
        auto bits = std::make_shared<CastInst>(IROpcode::Trunc, rawI64, IRType::getInt32(),
                                               m_currentFunction->nextTempName("catch_bits"));
        if (m_currentBlock) m_currentBlock->addInstruction(bits);
        auto flt = std::make_shared<CastInst>(IROpcode::BitCast, bits, IRType::getFloat(),
                                              m_currentFunction->nextTempName("catch_val"));
        if (m_currentBlock) m_currentBlock->addInstruction(flt);
        return flt;
    }
    if (clauseType->isInteger()) {
        return emitCastIfNeeded(rawI64, clauseType);
    }
    return rawI64;
}

IRValuePtr IRBuilder::getOrCreateReturnPendingSlot() {
    if (inAsyncSM()) {
        if (!m_returnPendingSlot) m_returnPendingSlot = asyncFrameSlotGEP(AsyncSM::kReturnPendingSlot);
        return m_returnPendingSlot;
    }
    if (!m_returnPendingSlot) {
        auto alloca = std::make_shared<AllocaInst>(IRType::getInt1(),
                                                   m_currentFunction->nextTempName("return_pending"));
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        m_currentFunction->getEntryBlock()->addInstruction(std::make_shared<StoreInst>(
            std::make_shared<ConstantInt>(IRType::getInt1(), 0), alloca));
        m_returnPendingSlot = alloca;
    }
    return m_returnPendingSlot;
}

IRValuePtr IRBuilder::getOrCreateReturnValueSlot() {
    if (inAsyncSM()) {
        // The resume function returns void; the "return value" is the
        // Task payload in the frame retVal slot (a struct return is
        // stored there as a heap-box pointer by visit(ReturnStmt)).
        if (!m_returnValueSlot) m_returnValueSlot = asyncFrameSlotGEP(AsyncSM::kRetValSlot);
        return m_returnValueSlot;
    }
    if (m_currentFunction->getReturnType()->isVoid()) return nullptr;
    if (!m_returnValueSlot) {
        auto alloca = std::make_shared<AllocaInst>(m_currentFunction->getReturnType(),
                                                   m_currentFunction->nextTempName("return_value"));
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        m_returnValueSlot = alloca;
    }
    return m_returnValueSlot;
}

IRBasicBlock* IRBuilder::getOrCreateDeferredReturnBlock() {
    if (m_deferredReturnBlock) return m_deferredReturnBlock;
    auto retBB = createBlock("deferred_return");
    m_deferredReturnBlock = retBB;
    // Phase 5 slice A: in the state machine a deferred return does
    // not emit a ReturnInst — the return value is already in the frame
    // retVal slot, so the block just branches to the finish block
    // (which completes the task and frees the frame).
    if (inAsyncSM()) {
        retBB->addInstruction(std::make_shared<BranchInst>(m_asyncSM->finishBB->getName()));
        return retBB;
    }
    // Emit the block body now. NOTE: this block deliberately emits NO
    // scope drops. It is shared by every deferred return in the
    // function, so it cannot know which scopes are live at a given
    // return site; dropping a fixed snapshot would release
    // uninitialized allocas on a shallower return path. Refcounted
    // locals on a deferred-return path therefore leak (bounded, one
    // function's worth of locals per return). The returned value
    // itself was retained at the return site, so it survives.
    IRBasicBlock* saved = m_currentBlock;
    setInsertPoint(retBB);
    IRTypePtr retType = m_currentFunction->getReturnType();
    if (retType->isVoid()) {
        retBB->addInstruction(std::make_shared<ReturnInst>());
    } else {
        auto slot = getOrCreateReturnValueSlot();
        auto loadInst = std::make_shared<LoadInst>(retType, slot, m_currentFunction->nextTempName("deferred_ret"));
        retBB->addInstruction(loadInst);
        retBB->addInstruction(std::make_shared<ReturnInst>(loadInst));
    }
    setInsertPoint(saved);
    return retBB;
}

IRBasicBlock* IRBuilder::getOrCreateUncaughtThrowBlock() {
    if (m_uncaughtThrowBlock) return m_uncaughtThrowBlock;
    auto uncaughtBB = createBlock("uncaught_throw");
    m_uncaughtThrowBlock = uncaughtBB;
    // Phase 5 slice A: in the state machine an uncaught throw panics
    // (an uncaught throw is unrecoverable, as in sync code) and then
    // branches to the finish block as a terminator. Result/Option
    // returns are already rejected for async fns by asyncReturnKind,
    // so no Err/None conversion is needed here.
    if (inAsyncSM()) {
        IRBasicBlock* saved = m_currentBlock;
        setInsertPoint(uncaughtBB);
        auto payloadSlot = getOrCreateThrowPayloadSlot();
        auto payload = std::make_shared<LoadInst>(IRType::getPointer(IRType::getInt8()), payloadSlot,
                                                  m_currentFunction->nextTempName("uncaught_msg"));
        uncaughtBB->addInstruction(payload);
        auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_panic",
                                               std::vector<IRValuePtr>{payload});
        uncaughtBB->addInstruction(call);
        // Unreachable at runtime (panic aborts); keep the block
        // terminated for the verifier.
        uncaughtBB->addInstruction(std::make_shared<BranchInst>(m_asyncSM->finishBB->getName()));
        setInsertPoint(saved);
        return uncaughtBB;
    }
    IRBasicBlock* saved = m_currentBlock;
    setInsertPoint(uncaughtBB);
    auto payloadSlot = getOrCreateThrowPayloadSlot();
    auto payload = std::make_shared<LoadInst>(IRType::getPointer(IRType::getInt8()), payloadSlot,
                                              m_currentFunction->nextTempName("uncaught_msg"));
    uncaughtBB->addInstruction(payload);
    IRTypePtr retType = m_currentFunction->getReturnType();
    auto structRet = std::dynamic_pointer_cast<IRStructType>(retType);
    bool isResult = structRet && structRet->getName() == "struct.Result";
    bool isOption = structRet && structRet->getName() == "struct.Option";
    if (retType->isStruct() && (isResult || isOption)) {
        // Uncaught throw in a Result/Option-returning function becomes
        // Err / None. The thrown value is discarded -- but if the
        // payload holds the unique owning reference to a class object
        // (throw-owns flag), release that share first, or the object
        // leaks once per uncaught throw.
        IRValuePtr retVal;
        if (isResult) {
            auto errVal = emitCreateTaggedUnion("Result", 1, payload);
            auto loadInst = std::make_shared<LoadInst>(retType, errVal, m_currentFunction->nextTempName("uncaught_err"));
            uncaughtBB->addInstruction(loadInst);
            retVal = loadInst;
        } else {
            auto noneVal = emitCreateTaggedUnion("Option", 1, nullptr);
            auto loadInst = std::make_shared<LoadInst>(retType, noneVal, m_currentFunction->nextTempName("uncaught_none"));
            uncaughtBB->addInstruction(loadInst);
            retVal = loadInst;
        }
        auto ownsSlot = getOrCreateThrowOwnsSlot();
        auto ownsLoad = std::make_shared<LoadInst>(IRType::getInt1(), ownsSlot,
                                                   m_currentFunction->nextTempName("uncaught_owns"));
        uncaughtBB->addInstruction(ownsLoad);
        auto releaseBB = createBlock("uncaught_release");
        auto retBB = createBlock("uncaught_return");
        uncaughtBB->addInstruction(std::make_shared<CondBranchInst>(
            ownsLoad, releaseBB->getName(), retBB->getName()));
        setInsertPoint(releaseBB);
        auto rawSlot = getOrCreateThrowRawSlot();
        auto rawLoad = std::make_shared<LoadInst>(IRType::getInt64(), rawSlot,
                                                  m_currentFunction->nextTempName("uncaught_raw"));
        releaseBB->addInstruction(rawLoad);
        auto rawPtr = std::make_shared<CastInst>(IROpcode::IntToPtr, rawLoad, IRType::getPointer(),
                                                 m_currentFunction->nextTempName("uncaught_obj"));
        releaseBB->addInstruction(rawPtr);
        releaseBB->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_release", std::vector<IRValuePtr>{rawPtr}));
        releaseBB->addInstruction(std::make_shared<BranchInst>(retBB->getName()));
        setInsertPoint(retBB);
        retBB->addInstruction(std::make_shared<ReturnInst>(retVal));
    } else {
        // Otherwise an uncaught throw is unrecoverable: panic with the
        // thrown message (ADR-002: unrecoverable errors use panic()).
        auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_panic",
                                               std::vector<IRValuePtr>{payload});
        uncaughtBB->addInstruction(call);
        // Unreachable at runtime (panic aborts); keep the block
        // terminated for the verifier.
        uncaughtBB->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(retType)));
    }
    setInsertPoint(saved);
    return uncaughtBB;
}

IRBasicBlock* IRBuilder::findThrowTarget() {
    for (auto it = m_tryStack.rbegin(); it != m_tryStack.rend(); ++it) {
        if (it->catchBlock) return it->catchBlock;
        if (it->finallyBlock) return it->finallyBlock;
    }
    return getOrCreateUncaughtThrowBlock();
}

IRBasicBlock* IRBuilder::findFinallyTarget() {
    for (auto it = m_tryStack.rbegin(); it != m_tryStack.rend(); ++it) {
        if (it->finallyBlock) return it->finallyBlock;
    }
    return nullptr;
}

void IRBuilder::emitThrowValue(IRValuePtr strVal, IRValuePtr rawI64, int32_t typeTag, bool owns) {
    auto flagSlot = getOrCreateThrowPendingSlot();
    auto payloadSlot = getOrCreateThrowPayloadSlot();
    auto typeSlot = getOrCreateThrowTypeSlot();
    auto rawSlot = getOrCreateThrowRawSlot();
    auto ownsSlot = getOrCreateThrowOwnsSlot();
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantInt>(IRType::getInt1(), 1), flagSlot));
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(strVal, payloadSlot));
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantInt>(IRType::getInt32(), typeTag), typeSlot));
    if (rawI64) {
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(rawI64, rawSlot));
    }
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantInt>(IRType::getInt1(), owns ? 1 : 0), ownsSlot));
    // Unwind: release the refcounted locals of the try being left (and
    // any scopes nested inside it). The throw site has already retained
    // the thrown object into the payload when needed, so these drops
    // cannot free it. Locals of enclosing scopes stay live.
    const TryFrame* frame = findThrowFrame();
    emitThrowScopeDrops(frame ? frame->refScopeDepth : 0);
    IRBasicBlock* target = findThrowTarget();
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(target->getName()));
}

void IRBuilder::emitFinallyDispatch(IRBasicBlock* mergeBlock) {
    auto throwFlag = getOrCreateThrowPendingSlot();
    auto loadThrow = std::make_shared<LoadInst>(IRType::getInt1(), throwFlag,
                                                m_currentFunction->nextTempName("throw_pending"));
    m_currentBlock->addInstruction(loadThrow);
    IRBasicBlock* checkReturnBB = createBlock("finally_check_return");
    IRBasicBlock* propagateBB = findThrowTarget();
    // Phase 5 slice B: a pending throw propagating through this
    // finally also leaves the enclosing try -- release that try's
    // refcounted locals (scopes at or above the propagate target
    // frame's entry depth). The throw site already released the inner
    // try's own scopes; the scopes still on the stack here are the
    // enclosing ones. When there is no enclosing frame (uncaught
    // throw), drop every scope still on the stack.
    const TryFrame* targetFrame = findThrowFrame();
    size_t dropDepth = targetFrame ? targetFrame->refScopeDepth : 0;
    IRBasicBlock* throwBB = propagateBB;
    if (dropDepth < m_refCountedScopes.size()) {
        throwBB = createBlock("finally_propagate_drops");
    }
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(
        loadThrow, throwBB->getName(), checkReturnBB->getName()));
    if (throwBB != propagateBB) {
        setInsertPoint(throwBB);
        emitThrowScopeDrops(dropDepth);
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(propagateBB->getName()));
    }

    setInsertPoint(checkReturnBB);
    auto retFlag = getOrCreateReturnPendingSlot();
    auto loadRet = std::make_shared<LoadInst>(IRType::getInt1(), retFlag,
                                              m_currentFunction->nextTempName("return_pending"));
    m_currentBlock->addInstruction(loadRet);
    IRBasicBlock* returnTarget = findFinallyTarget();
    if (!returnTarget) returnTarget = getOrCreateDeferredReturnBlock();
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(
        loadRet, returnTarget->getName(), mergeBlock->getName()));
}

void IRBuilder::pushScope() {
    m_scopes.emplace_back();
    m_refCountedScopes.emplace_back();
}

void IRBuilder::emitScopeDrops(size_t scopeIndex) {
    if (scopeIndex >= m_refCountedScopes.size()) return;
    if (!m_currentBlock || m_currentBlock->hasTerminator()) return;

    const auto& vars = m_refCountedScopes[scopeIndex];
    for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
        const auto& [name, allocaPtr] = *it;
        std::string dropReg = m_currentFunction->nextTempName("drop_" + name);
        auto loadInst = std::make_shared<LoadInst>(IRType::getPointer(), allocaPtr, dropReg);
        m_currentBlock->addInstruction(loadInst);

        auto releaseCall = std::make_shared<CallInst>(
            IRType::getVoid(),
            "sp_rt_release",
            std::vector<IRValuePtr>{loadInst}
        );
        m_currentBlock->addInstruction(releaseCall);
    }
}

void IRBuilder::emitAllFunctionDrops() {
    if (!m_currentBlock || m_currentBlock->hasTerminator()) return;
    for (int i = static_cast<int>(m_refCountedScopes.size()) - 1; i >= 0; --i) {
        emitScopeDrops(static_cast<size_t>(i));
    }
}

void IRBuilder::popScope() {
    if (!m_refCountedScopes.empty()) {
        emitScopeDrops(m_refCountedScopes.size() - 1);
        m_refCountedScopes.pop_back();
    }
    if (!m_scopes.empty()) {
        m_scopes.pop_back();
    }
}

void IRBuilder::declareVariable(const std::string& name, IRValuePtr allocaPtr, IRTypePtr valType) {
    if (m_scopes.empty()) {
        pushScope();
    }
    m_scopes.back()[name] = {std::move(allocaPtr), std::move(valType)};
}

std::pair<IRValuePtr, IRTypePtr> IRBuilder::lookupVariable(const std::string& name) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return {nullptr, nullptr};
}

IRBasicBlock* IRBuilder::createBlock(const std::string& name) {
    std::string blockName = name + "_" + std::to_string(m_blockCounter++);
    return m_currentFunction->createBasicBlock(blockName);
}

void IRBuilder::setInsertPoint(IRBasicBlock* block) {
    m_currentBlock = block;
    ensureTryState();
}

void IRBuilder::ensureTryState() {
    if (m_tryStateFunction == m_currentFunction) return;
    m_tryStateFunction = m_currentFunction;
    m_tryStack.clear();
    m_throwPayloadSlot = nullptr;
    m_throwPendingSlot = nullptr;
    m_returnPendingSlot = nullptr;
    m_returnValueSlot = nullptr;
    m_deferredReturnBlock = nullptr;
    m_uncaughtThrowBlock = nullptr;
    // Phase 5 slice B: per-function throw slots reset here. The tag
    // registry (m_throwTypeTags) deliberately does NOT reset: tags
    // must stay stable across functions for the whole compilation.
    m_throwTypeSlot = nullptr;
    m_throwRawSlot = nullptr;
    m_throwOwnsSlot = nullptr;
    m_throwStructSlots.clear();
}

IRValuePtr IRBuilder::emitCastIfNeeded(IRValuePtr val, IRTypePtr targetType) {
    if (!val || !targetType || val->getType()->getKind() == targetType->getKind()) {
        return val;
    }

    auto srcType = val->getType();
    std::string tempName = m_currentFunction ? m_currentFunction->nextTempName("cast") : "%cast";

    if (srcType->isInteger() && targetType->isInteger()) {
        if (auto constInt = std::dynamic_pointer_cast<ConstantInt>(val)) {
            return std::make_shared<ConstantInt>(targetType, constInt->getValue());
        }

        size_t srcBits = 32;
        if (srcType->isInt1()) srcBits = 1;
        else if (srcType->isInt8()) srcBits = 8;
        else if (srcType->isInt16()) srcBits = 16;
        else if (srcType->isInt32()) srcBits = 32;
        else if (srcType->isInt64()) srcBits = 64;

        size_t dstBits = 32;
        if (targetType->isInt1()) dstBits = 1;
        else if (targetType->isInt8()) dstBits = 8;
        else if (targetType->isInt16()) dstBits = 16;
        else if (targetType->isInt32()) dstBits = 32;
        else if (targetType->isInt64()) dstBits = 64;

        if (srcBits < dstBits) {
            auto cast = std::make_shared<CastInst>(IROpcode::SExt, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        } else if (srcBits > dstBits) {
            auto cast = std::make_shared<CastInst>(IROpcode::Trunc, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        }
    }

    if (srcType->isInteger() && targetType->isFloatingPoint()) {
        if (auto constInt = std::dynamic_pointer_cast<ConstantInt>(val)) {
            return std::make_shared<ConstantFloat>(targetType, static_cast<double>(constInt->getValue()));
        }
        auto cast = std::make_shared<CastInst>(IROpcode::SIToFP, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isFloatingPoint() && targetType->isInteger()) {
        if (auto constFloat = std::dynamic_pointer_cast<ConstantFloat>(val)) {
            return std::make_shared<ConstantInt>(targetType, static_cast<int64_t>(constFloat->getValue()));
        }
        auto cast = std::make_shared<CastInst>(IROpcode::FPToSI, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isPointer() && targetType->isInteger()) {
        auto cast = std::make_shared<CastInst>(IROpcode::PtrToInt, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isInteger() && targetType->isPointer()) {
        auto cast = std::make_shared<CastInst>(IROpcode::IntToPtr, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isFloatingPoint() && targetType->isFloatingPoint()) {
        if (auto constFloat = std::dynamic_pointer_cast<ConstantFloat>(val)) {
            return std::make_shared<ConstantFloat>(targetType, constFloat->getValue());
        }
        if (srcType->isDouble() && targetType->isFloat()) {
            auto cast = std::make_shared<CastInst>(IROpcode::FPTrunc, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        } else if (srcType->isFloat() && targetType->isDouble()) {
            auto cast = std::make_shared<CastInst>(IROpcode::FPExt, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        }
    }

    return val;
}

IRValuePtr IRBuilder::convertToStringValue(IRValuePtr val, IRTypePtr valType, const ast::ExpressionNode* exprNode) {
    if (!val || !valType) return m_module->getOrCreateStringLiteral("");
    if (valType->isPointer()) {
        return val;
    }
    std::string callReg = m_currentFunction ? m_currentFunction->nextTempName("str") : "%str";
    auto semaType = exprNode ? m_sema.getNodeType(exprNode) : nullptr;
    if (valType->isInt1() || (semaType && semaType->getKind() == sema::TypeKind::Bool)) {
        val = emitCastIfNeeded(val, IRType::getInt1());
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_bool",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }
    if (semaType && semaType->isChar()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_char",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (valType->isFloat()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_float",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (valType->isDouble()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_double",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (semaType && semaType->isUnsignedInteger()) {
        if (auto constInt = std::dynamic_pointer_cast<ConstantInt>(val)) {
            uint64_t uval = static_cast<uint64_t>(constInt->getValue());
            if (valType->isInt8()) uval &= 0xFFULL;
            else if (valType->isInt16()) uval &= 0xFFFFULL;
            else if (valType->isInt32()) uval &= 0xFFFFFFFFULL;
            val = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(uval));
        } else if (!valType->isInt64()) {
            std::string zextName = m_currentFunction ? m_currentFunction->nextTempName("zext") : "%zext";
            auto zext = std::make_shared<CastInst>(IROpcode::ZExt, val, IRType::getInt64(), zextName);
            if (m_currentBlock) m_currentBlock->addInstruction(zext);
            val = zext;
        }
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_uint",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (!semaType && valType->isInt8()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_char",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    val = emitCastIfNeeded(val, IRType::getInt64());
    auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                           "sp_rt_str_from_int",
                                           std::vector<IRValuePtr>{val},
                                           callReg);
    if (m_currentBlock) m_currentBlock->addInstruction(call);
    return call;
}

IRTypePtr IRBuilder::mapType(const sema::Type* type) const {
    if (!type) return IRType::getInt32();

    switch (type->getKind()) {
        case sema::TypeKind::Void: return IRType::getVoid();
        case sema::TypeKind::Bool: return IRType::getInt1();
        case sema::TypeKind::Int8:
        case sema::TypeKind::UInt8:
        case sema::TypeKind::Char: return IRType::getInt8();
        case sema::TypeKind::Int16:
        case sema::TypeKind::UInt16: return IRType::getInt16();
        case sema::TypeKind::Int32:
        case sema::TypeKind::UInt32: return IRType::getInt32();
        case sema::TypeKind::Int64:
        case sema::TypeKind::UInt64: return IRType::getInt64();
        case sema::TypeKind::Float: return IRType::getFloat();
        case sema::TypeKind::Double: return IRType::getDouble();
        case sema::TypeKind::String: return IRType::getPointer(IRType::getInt8());
        case sema::TypeKind::Enum: {
            const auto* en = static_cast<const sema::EnumType*>(type);
            if (en && en->isTaggedUnion()) {
                std::string name = en->getName();
                if (name.rfind("Result", 0) == 0) name = "Result";
                else if (name.rfind("Option", 0) == 0) name = "Option";
                return IRType::getStruct("struct." + name, {IRType::getInt32(), IRType::getInt64()});
            }
            return IRType::getInt32();
        }
        case sema::TypeKind::Array:
        case sema::TypeKind::Map:
        case sema::TypeKind::Class:
        case sema::TypeKind::Interface:
        case sema::TypeKind::Struct:
        case sema::TypeKind::Pointer:
            return IRType::getPointer();
        case sema::TypeKind::Nullable: {
            const auto* nullT = static_cast<const sema::NullableType*>(type);
            if (nullT && nullT->getUnderlyingType()) {
                return mapType(nullT->getUnderlyingType().get());
            }
            return IRType::getPointer();
        }
        default: return IRType::getInt32();
    }
}

IRTypePtr IRBuilder::mapType(const std::string& typeName) const {
    if (typeName.empty() || typeName == "void") return IRType::getVoid();
    if (typeName == "bool") return IRType::getInt1();
    if (typeName == "byte" || typeName == "char" || typeName == "int8" || typeName == "uint8") return IRType::getInt8();
    if (typeName == "short" || typeName == "int16" || typeName == "uint16") return IRType::getInt16();
    if (typeName == "int" || typeName == "int32" || typeName == "uint" || typeName == "uint32") return IRType::getInt32();
    if (typeName == "long" || typeName == "int64" || typeName == "uint64") return IRType::getInt64();
    if (typeName == "float") return IRType::getFloat();
    if (typeName == "double") return IRType::getDouble();
    if (typeName == "string") return IRType::getPointer(IRType::getInt8());
    if (typeName == "ptr" || typeName.rfind("ptr<", 0) == 0) return IRType::getPointer();

    if (typeName == "Result" || typeName.rfind("Result<", 0) == 0) {
        return IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
    }
    if (typeName == "Option" || typeName.rfind("Option<", 0) == 0) {
        return IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
    }

    auto userType = m_sema.getTypeContext().findUserType(typeName);
    if (userType && userType->isEnum()) {
        auto en = std::static_pointer_cast<sema::EnumType>(userType);
        if (en && en->isTaggedUnion()) {
            std::string name = en->getName();
            if (name.rfind("Result", 0) == 0) name = "Result";
            else if (name.rfind("Option", 0) == 0) name = "Option";
            return IRType::getStruct("struct." + name, {IRType::getInt32(), IRType::getInt64()});
        }
        return IRType::getInt32();
    }

    // Object/class/struct/interface reference
    return IRType::getPointer();
}

// --- AST Visitors ---

// Phase 3 slice A: function pointers + overload name mangling.
std::vector<std::string> IRBuilder::fnParamTypes(const ast::FunctionDecl& decl) const {
    // Canonical param types from the decl's FunctionSymbol, so `int32`
    // and `int` spell the same signature at definition and call sites.
    for (const auto& sym : m_sema.getSymbolTable().lookupFunctions(decl.getName())) {
        if (sym && sym->getASTDecl() == &decl && sym->getFunctionType()) {
            std::vector<std::string> out;
            for (const auto& p : sym->getFunctionType()->getParamTypes()) {
                out.push_back(p ? p->toString() : "any");
            }
            return out;
        }
    }
    // Fallback: raw strings (decl not found in the symbol table, e.g. a
    // stdlib function whose symbol carries no AST decl).
    std::vector<std::string> out;
    for (const auto& p : decl.getParams()) out.push_back(p.getType());
    return out;
}

std::string IRBuilder::llvmFunctionName(const ast::FunctionDecl& decl) const {
    // `main` is the entry point and extern "C" names are part of the C ABI:
    // neither is ever mangled. Generic fns are monomorphized separately.
    if (decl.getName() == "main" || (!decl.hasBody() && !decl.isShortForm()) || decl.isGeneric()) {
        return decl.getName();
    }
    auto overloads = m_sema.getSymbolTable().lookupFunctions(decl.getName());
    bool isOverloaded = overloads.size() > 1;
    return codegen::VTableBuilder::mangleOverloaded(decl.getName(), fnParamTypes(decl), isOverloaded);
}

std::string IRBuilder::llvmMethodName(const std::string& className, const ast::FunctionDecl& decl) const {
    std::string base = className + "_" + decl.getName();
    auto userType = m_sema.getTypeContext().findUserType(className);
    if (!userType || !userType->isClass()) return base;
    auto clsType = std::static_pointer_cast<const sema::ClassType>(userType);
    auto paramTypes = codegen::VTableBuilder::canonicalParamTypes(*clsType, decl);
    bool isOverloaded = codegen::VTableBuilder::isMethodOverloaded(*clsType, decl.getName());
    return codegen::VTableBuilder::mangleOverloaded(base, paramTypes, isOverloaded);
}

std::vector<std::string> IRBuilder::targetParamTypes(const std::shared_ptr<sema::FunctionSymbol>& target) {
    std::vector<std::string> out;
    if (target && target->getFunctionType()) {
        for (const auto& p : target->getFunctionType()->getParamTypes()) {
            out.push_back(p ? p->toString() : "any");
        }
    }
    return out;
}

std::string IRBuilder::llvmCallName(const std::string& calleeName,
                                    const std::shared_ptr<sema::FunctionSymbol>& target) const {
    if (!target || target->isMethod() || target->isGeneric()) return calleeName;
    auto overloads = m_sema.getSymbolTable().lookupFunctions(calleeName);
    bool isOverloaded = overloads.size() > 1;
    return codegen::VTableBuilder::mangleOverloaded(calleeName, targetParamTypes(target), isOverloaded);
}

void IRBuilder::visit(const ast::Program& node) {
    // Check if there is an explicit main function
    bool hasMainFunc = false;
    std::vector<const ast::StatementNode*> topLevelStmts;

    for (const auto& decl : node.getDeclarations()) {
        if (auto* fn = dynamic_cast<const ast::FunctionDecl*>(decl.get())) {
            if (fn->getName() == "main") {
                hasMainFunc = true;
            }
        }
    }

    // Collect top-level statements outside of function/class/struct declarations
    for (const auto& decl : node.getDeclarations()) {
        if (auto* stmt = dynamic_cast<const ast::StatementNode*>(decl.get())) {
            if (dynamic_cast<const ast::FunctionDecl*>(stmt) ||
                dynamic_cast<const ast::ClassDecl*>(stmt) ||
                dynamic_cast<const ast::InterfaceDecl*>(stmt) ||
                dynamic_cast<const ast::StructDecl*>(stmt) ||
                dynamic_cast<const ast::EnumDecl*>(stmt) ||
                dynamic_cast<const ast::ModuleDecl*>(stmt) ||
                dynamic_cast<const ast::ImportDecl*>(stmt)) {
                continue;
            }
            if (const auto* block = dynamic_cast<const ast::BlockStmt*>(stmt)) {
                bool hasOnlyDecls = true;
                for (const auto& inner : block->getStatements()) {
                    if (!dynamic_cast<const ast::FunctionDecl*>(inner.get()) &&
                        !dynamic_cast<const ast::StructDecl*>(inner.get()) &&
                        !dynamic_cast<const ast::EnumDecl*>(inner.get())) {
                        hasOnlyDecls = false;
                        break;
                    }
                }
                if (hasOnlyDecls && !block->getStatements().empty()) {
                    continue;
                }
            }
            topLevelStmts.push_back(stmt);
        }
    }

    // Index functions and generic classes/functions
    for (const auto& decl : node.getDeclarations()) {
        if (const auto* cls = dynamic_cast<const ast::ClassDecl*>(decl.get())) {
            for (const auto& member : cls->getMembers()) {
                if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
                    // Phase 3 slice A: key by the mangled LLVM name so
                    // same-name overloads coexist in the index.
                    m_astFunctions[llvmMethodName(cls->getName(), *fn)] = fn;
                }
            }
            if (cls->isGeneric()) {
                m_genericClasses[cls->getName()] = cls;
            }
        } else if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(decl.get())) {
            m_astFunctions[llvmFunctionName(*fn)] = fn;
            if (fn->isGeneric()) {
                m_genericFunctions[fn->getName()] = fn;
            }
            if (!fn->hasBody() && !fn->isShortForm()) {
                std::vector<std::shared_ptr<ParameterValue>> params;
                for (size_t i = 0; i < fn->getParams().size(); ++i) {
                    const auto& p = fn->getParams()[i];
                    params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i));
                }
                m_module->createFunction(fn->getName(), mapType(fn->getReturnType()), params, /*isExternal=*/true);
            }
        } else if (const auto* block = dynamic_cast<const ast::BlockStmt*>(decl.get())) {
            for (const auto& inner : block->getStatements()) {
                if (const auto* cls = dynamic_cast<const ast::ClassDecl*>(inner.get())) {
                    for (const auto& member : cls->getMembers()) {
                        if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
                            m_astFunctions[llvmMethodName(cls->getName(), *fn)] = fn;
                        }
                    }
                    if (cls->isGeneric()) {
                        m_genericClasses[cls->getName()] = cls;
                    }
                } else if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(inner.get())) {
                    m_astFunctions[llvmFunctionName(*fn)] = fn;
                    if (!fn->hasBody() && !fn->isShortForm()) {
                        std::vector<std::shared_ptr<ParameterValue>> params;
                        for (size_t i = 0; i < fn->getParams().size(); ++i) {
                            const auto& p = fn->getParams()[i];
                            params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i));
                        }
                        m_module->createFunction(fn->getName(), mapType(fn->getReturnType()), params, /*isExternal=*/true);
                    }
                }
            }
        }
    }

    // First visit class declarations (generates constructors & methods)
    for (const auto& decl : node.getDeclarations()) {
        if (const auto* cls = dynamic_cast<const ast::ClassDecl*>(decl.get())) {
            if (cls->isGeneric()) {
                // Compile all specializations of this generic class registered in sema
                for (const auto& [typeName, userType] : m_sema.getTypeContext().getUserTypes()) {
                    if (userType && userType->isClass()) {
                        auto clsType = std::static_pointer_cast<sema::ClassType>(userType);
                        if (clsType->getName().rfind(cls->getName() + "<", 0) == 0) {
                            std::string monoName = codegen::VTableBuilder::mangleGenericName(clsType->getName());
                            const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(monoName) : nullptr;
                            if (layout) {
                                size_t lt = clsType->getName().find('<');
                                size_t gt = clsType->getName().rfind('>');
                                std::vector<std::string> typeArgs = sema::TypeContext::splitTypeArgs(clsType->getName().substr(lt + 1, gt - lt - 1));

                                const ast::FunctionDecl* explicitCtor = nullptr;
                                for (const auto& member : cls->getMembers()) {
                                    if (const auto* mFn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
                                        if (mFn->getName() == cls->getName()) {
                                            explicitCtor = mFn;
                                            break;
                                        }
                                    }
                                }

                                if (explicitCtor) {
                                    compileSpecializedConstructor(explicitCtor, cls->getTypeParams(), typeArgs, *layout);
                                } else {
                                    compileDefaultConstructor(*cls, *layout);
                                }

                                for (const auto& member : cls->getMembers()) {
                                    if (const auto* mFn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
                                        if (mFn->getName() != cls->getName()) {
                                            compileSpecializedMethod(*mFn, cls->getTypeParams(), typeArgs, *layout);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                decl->accept(*this);
            }
        }
    }

    // Next visit standalone functions
    for (const auto& decl : node.getDeclarations()) {
        if (dynamic_cast<const ast::FunctionDecl*>(decl.get())) {
            decl->accept(*this);
        } else if (const auto* block = dynamic_cast<const ast::BlockStmt*>(decl.get())) {
            for (const auto& inner : block->getStatements()) {
                if (dynamic_cast<const ast::FunctionDecl*>(inner.get())) {
                    inner->accept(*this);
                }
            }
        }
    }

    // If there are top-level statements and no explicit main function, synthesize main
    if (!topLevelStmts.empty() && !hasMainFunc) {
        auto mainFunc = m_module->createFunction("main", IRType::getInt32(), {});
        m_currentFunction = mainFunc;
        auto entryBB = mainFunc->createBasicBlock("entry");
        setInsertPoint(entryBB);
        pushScope();

        for (const auto* stmt : topLevelStmts) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }

        if (!m_currentBlock->hasTerminator()) {
            // Phase 3 slice C: synthesized main also drains
            // fire-and-forget async tasks before the process exits.
            emitMainExitPump();
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)));
        }
        popScope();
    }
}

void IRBuilder::visit(const ast::FunctionDecl& node) {
    if (node.isGeneric()) {
        return; // Generic function templates are monomorphized on demand when called
    }

    // Phase 5 slice A: an `async fn` body is the state-machine entry
    // `void <name>(ptr %task, ptr %ctx)` — it allocates a heap frame,
    // stores task + args, and calls the resume function. The real body
    // is lowered into the resume function (see asyncSMBegin/End).
    const bool isAsync = node.isAsync();
    IRTypePtr returnType;
    std::vector<std::shared_ptr<ParameterValue>> params;
    if (isAsync) {
        returnType = IRType::getVoid();
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_task", 0));
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_ctx", 1));
    } else {
        returnType = (node.getName() == "main") ? IRType::getInt32() : mapType(node.getReturnType());
        for (size_t i = 0; i < node.getParams().size(); ++i) {
            const auto& p = node.getParams()[i];
            params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i));
        }
    }

    if (!node.hasBody() && !node.isShortForm()) {
        m_module->createFunction(llvmFunctionName(node), returnType, params, /*isExternal=*/true);
        return;
    }

    auto func = m_module->createFunction(llvmFunctionName(node), returnType, params);
    m_currentFunction = func;
    // Phase 2 slice B2: remember whether this is an `async fn` so
    // visit(AwaitExpr) knows whether await may lower here. Restored at
    // the end of this function; methods and generic specializations do
    // not go through here, so await inside them still fails loudly.
    bool prevAsync = m_inAsyncFunction;
    m_inAsyncFunction = isAsync;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();

    if (isAsync) {
        // Resolve the sema types (the pre-scan validates the return /
        // param types and rewrites the body: slot assignment + await
        // hoisting). The AST is mutated in place (see asyncPrepareBody).
        sema::TypePtr retSema = asyncResolveType(node.getReturnType());
        std::vector<sema::TypePtr> paramSema;
        for (const auto& p : node.getParams()) paramSema.push_back(asyncResolveType(p.getType()));
        auto& decl = const_cast<ast::FunctionDecl&>(node);
        if (asyncPrepareBody(decl, llvmFunctionName(node), retSema, paramSema, nullptr) < 0) {
            // E1000 already reported; skip body lowering (the build
            // fails on the recorded errors).
            popScope();
            m_inAsyncFunction = prevAsync;
            m_currentFunction = nullptr;
            m_currentBlock = nullptr;
            return;
        }
        asyncSMBegin(llvmFunctionName(node), func);
        for (const auto& stmt : decl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
        asyncSMEnd();
        popScope();
        m_inAsyncFunction = prevAsync;
        m_currentFunction = nullptr;
        m_currentBlock = nullptr;
        return;
    }

    // Allocate stack slots for parameters and store incoming values
    for (size_t i = 0; i < params.size(); ++i) {
        const auto& p = node.getParams()[i];
        const auto& paramVal = params[i];
        auto allocaInst = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaInst);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaInst));
        declareVariable(p.getName(), allocaInst, paramVal->getType());
    }

    if (node.hasBody()) {
        for (const auto& stmt : node.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (node.isShortForm()) {
        node.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    // Ensure terminator exists
    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else if (node.getName() == "main") {
            // Phase 3 slice C: main drains fire-and-forget async tasks
            // before the process exits (timers/queued tasks only;
            // registered sockets do not keep the drain going).
            emitMainExitPump();
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)));
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_inAsyncFunction = prevAsync;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::visit(const ast::BlockStmt& node) {
    pushScope();
    for (const auto& stmt : node.getStatements()) {
        stmt->accept(*this);
        if (m_currentBlock->hasTerminator()) break;
    }
    popScope();
}

void IRBuilder::visit(const ast::ExprStmt& node) {
    node.getExpr().accept(*this);
}

void IRBuilder::visit(const ast::VarDeclStmt& node) {
    IRTypePtr valType = nullptr;

    const codegen::ClassLayout* structLayout = nullptr;
    if (m_vtableBuilder && !node.getType().empty()) {
        structLayout = m_vtableBuilder->getClassLayout(node.getType());
        if (structLayout && structLayout->hasVTable) {
            structLayout = nullptr;
        }
    }

    if (structLayout) {
        valType = structLayout->structType;
    } else if (!node.getType().empty() && node.getType() != "var" && node.getType() != "let" && node.getType() != "const") {
        valType = mapType(node.getType());
    }

    IRValuePtr initVal = nullptr;
    if (node.hasInitializer()) {
        node.getInitializer()->accept(*this);
        initVal = m_lastExprValue;
        if (!valType && m_lastExprType) {
            valType = m_lastExprType;
        }
    }

    if (!valType) {
        valType = IRType::getInt32();
    }

    std::string varReg = "%" + node.getName() + "_" + std::to_string(m_varCounter++);
    IRValuePtr varAddr;
    if (inAsyncSM()) {
        // Phase 5 slice A: locals live in heap-frame slots so they
        // survive across await points (the pre-scan assigned this
        // VarDeclStmt a slot, sized for a struct value when needed).
        varAddr = asyncNodeSlotGEP(&node, valType, node.getLocation());
    } else {
        auto allocaInst = std::make_shared<AllocaInst>(valType, varReg);
        m_currentFunction->getEntryBlock()->addInstruction(allocaInst);
        varAddr = allocaInst;
    }
    declareVariable(node.getName(), varAddr, valType);

    bool isRefCounted = false;
    if (valType && valType->isPointer()) {
        if (m_vtableBuilder && !node.getType().empty()) {
            const auto* l = m_vtableBuilder->getClassLayout(node.getType());
            if (l && l->hasVTable) {
                isRefCounted = true;
            }
        }
        if (!isRefCounted && node.hasInitializer()) {
            auto semaType = m_sema.getNodeType(node.getInitializer());
            if (semaType && semaType->isReferenceType()) {
                isRefCounted = true;
            } else if (dynamic_cast<const ast::NewExpr*>(node.getInitializer())) {
                isRefCounted = true;
            }
        }
    }

    if (isRefCounted && !m_refCountedScopes.empty()) {
        m_refCountedScopes.back().push_back({node.getName(), varAddr});
        if (initVal && !dynamic_cast<const ast::NewExpr*>(node.getInitializer())) {
            auto retainCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_retain",
                std::vector<IRValuePtr>{initVal}
            );
            m_currentBlock->addInstruction(retainCall);
        }
    }

    if (initVal) {
        if (valType && valType->isStruct() && initVal->getType()->isPointer()) {
            auto loadInst = std::make_shared<LoadInst>(valType, initVal, m_currentFunction->nextTempName("tu_val"));
            m_currentBlock->addInstruction(loadInst);
            initVal = loadInst;
        }
        initVal = emitCastIfNeeded(initVal, valType);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(initVal, varAddr));
    }
}

void IRBuilder::visit(const ast::IfStmt& node) {
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    if (condVal && !condVal->getType()->isInt1()) {
        condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                m_currentFunction->nextTempName("bool"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
    }

    auto thenBB = createBlock("then");
    auto elseBB = node.hasElseBranch() ? createBlock("else") : nullptr;
    auto mergeBB = createBlock("merge");

    std::string falseLabel = elseBB ? elseBB->getName() : mergeBB->getName();
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, thenBB->getName(), falseLabel));

    // Then branch
    setInsertPoint(thenBB);
    node.getThenBranch().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
    }

    // Else branch
    if (elseBB) {
        setInsertPoint(elseBB);
        node.getElseBranch()->accept(*this);
        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
        }
    }

    setInsertPoint(mergeBB);
}

void IRBuilder::visit(const ast::WhileStmt& node) {
    auto condBB = createBlock("while_cond");
    auto bodyBB = createBlock("while_body");
    auto endBB = createBlock("while_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // Condition
    setInsertPoint(condBB);
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    if (condVal && !condVal->getType()->isInt1()) {
        condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                m_currentFunction->nextTempName("bool"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
    }
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBB->getName(), endBB->getName()));

    // Body
    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(condBB->getName());

    setInsertPoint(bodyBB);
    node.getBody().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();

    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::DoWhileStmt& node) {
    auto bodyBB = createBlock("dowhile_body");
    auto condBB = createBlock("dowhile_cond");
    auto endBB = createBlock("dowhile_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(bodyBB->getName()));

    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(condBB->getName());

    setInsertPoint(bodyBB);
    node.getBody().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();

    setInsertPoint(condBB);
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    if (condVal && !condVal->getType()->isInt1()) {
        condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                m_currentFunction->nextTempName("bool"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
    }
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBB->getName(), endBB->getName()));

    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::ForStmt& node) {
    pushScope();

    if (node.hasInit()) {
        node.getInit()->accept(*this);
    }

    auto condBB = createBlock("for_cond");
    auto bodyBB = createBlock("for_body");
    auto stepBB = createBlock("for_step");
    auto endBB = createBlock("for_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // Condition
    setInsertPoint(condBB);
    if (node.hasCondition()) {
        node.getCondition()->accept(*this);
        IRValuePtr condVal = m_lastExprValue;
        if (condVal && !condVal->getType()->isInt1()) {
            condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                    std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                    m_currentFunction->nextTempName("bool"));
            m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
        }
        m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBB->getName(), endBB->getName()));
    } else {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(bodyBB->getName()));
    }

    // Body
    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(stepBB->getName());

    setInsertPoint(bodyBB);
    node.getBody().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(stepBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();

    // Step
    setInsertPoint(stepBB);
    if (node.hasIncrement()) {
        node.getIncrement()->accept(*this);
    }
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    popScope();
    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::ForInStmt& node) {
    if (const auto* bin = dynamic_cast<const ast::BinaryExpr*>(&node.getIterable())) {
        if (bin->getOp() == lexer::TokenType::DotDot) {
            bin->getLeft().accept(*this);
            IRValuePtr startVal = m_lastExprValue;
            bin->getRight().accept(*this);
            IRValuePtr endVal = m_lastExprValue;
            if (!startVal || !endVal) return;

            startVal = emitCastIfNeeded(startVal, IRType::getInt32());
            endVal = emitCastIfNeeded(endVal, IRType::getInt32());

            std::string varReg = "%" + node.getVarName() + "_" + std::to_string(m_varCounter++);
            // Phase 5 slice A: in the state machine the loop var
            // lives in a heap-frame slot (an entry-block alloca would
            // not survive an await in the body).
            IRValuePtr varAlloca;
            if (inAsyncSM()) {
                varAlloca = asyncNodeSlotGEP(&node, IRType::getInt32(), node.getLocation());
            } else {
                auto alloca = std::make_shared<AllocaInst>(IRType::getInt32(), varReg);
                m_currentFunction->getEntryBlock()->addInstruction(alloca);
                varAlloca = alloca;
            }
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(startVal, varAlloca));

            auto condBB = createBlock("range_cond");
            auto bodyBB = createBlock("range_body");
            auto stepBB = createBlock("range_step");
            auto endBB = createBlock("range_end");

            m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

            // Condition: i < end
            setInsertPoint(condBB);
            auto curVal = std::make_shared<LoadInst>(IRType::getInt32(), varAlloca, m_currentFunction->nextTempName("range_cur"));
            condBB->addInstruction(curVal);
            auto cmpInst = std::make_shared<CompareInst>(IROpcode::ICmpSlt, curVal, endVal, m_currentFunction->nextTempName("range_cmp"));
            condBB->addInstruction(cmpInst);
            condBB->addInstruction(std::make_shared<CondBranchInst>(cmpInst, bodyBB->getName(), endBB->getName()));

            // Body
            setInsertPoint(bodyBB);
            pushScope();
            declareVariable(node.getVarName(), varAlloca, IRType::getInt32());

            m_breakStack.push_back(endBB->getName());
            m_continueStack.push_back(stepBB->getName());

            node.getBody().accept(*this);

            if (!m_currentBlock->hasTerminator()) {
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(stepBB->getName()));
            }

            m_breakStack.pop_back();
            m_continueStack.pop_back();
            popScope();

            // Step: i++
            setInsertPoint(stepBB);
            auto stepVal = std::make_shared<LoadInst>(IRType::getInt32(), varAlloca, m_currentFunction->nextTempName("range_step_load"));
            stepBB->addInstruction(stepVal);
            auto nextVal = std::make_shared<BinaryInst>(
                IROpcode::Add, stepVal, std::make_shared<ConstantInt>(IRType::getInt32(), 1),
                m_currentFunction->nextTempName("range_next"));
            stepBB->addInstruction(nextVal);
            stepBB->addInstruction(std::make_shared<StoreInst>(nextVal, varAlloca));
            stepBB->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

            setInsertPoint(endBB);
            return;
        }
    }

    node.getIterable().accept(*this);
    IRValuePtr collVal = m_lastExprValue;
    if (!collVal) return;

    auto semaIterType = m_sema.getNodeType(&node.getIterable());
    bool isList = semaIterType && semaIterType->isClass() && semaIterType->toString().rfind("List", 0) == 0;
    // Phase 6 slice A: maps iterate over their keys (see below).
    bool isMap = semaIterType && semaIterType->isMap();

    // 1. Query collection length/size
    std::string lenReg = m_currentFunction->nextTempName("for_len");
    auto lenCall = std::make_shared<CallInst>(
        IRType::getInt64(),
        (isMap ? "sp_rt_map_size" : (isList ? "sp_rt_list_size" : "sp_rt_array_length")),
        std::vector<IRValuePtr>{collVal},
        lenReg
    );
    m_currentBlock->addInstruction(lenCall);

    // 2. Allocate induction variable idx = 0 on stack
    std::string idxAllocaReg = "%for_idx_" + std::to_string(m_varCounter++);
    IRValuePtr idxAlloca;
    if (inAsyncSM()) {
        // Phase 5 slice A: the induction variable is loop-carried
        // state — it lives in a heap-frame slot (an entry-block
        // alloca would not survive an await in the body).
        auto it = m_asyncSM->auxSlots.find(std::make_pair(static_cast<const void*>(&node), int64_t{0}));
        if (it != m_asyncSM->auxSlots.end()) {
            idxAlloca = asyncFrameSlotGEP(it->second);
        } else {
            reportUnsupported(node.getLocation(), "loop variable",
                "internal error: an async loop induction variable was not assigned a frame slot by the pre-scan");
            auto alloca = std::make_shared<AllocaInst>(IRType::getInt64(), idxAllocaReg);
            m_currentFunction->getEntryBlock()->addInstruction(alloca);
            idxAlloca = alloca;
        }
    } else {
        auto alloca = std::make_shared<AllocaInst>(IRType::getInt64(), idxAllocaReg);
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        idxAlloca = alloca;
    }
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantInt>(IRType::getInt64(), 0), idxAlloca));

    // 3. Determine loop element variable IR type and allocate stack slot
    IRTypePtr itemIRType = IRType::getInt32();
    if (!node.getType().empty() && node.getType() != "var" && node.getType() != "let" && node.getType() != "auto") {
        itemIRType = mapType(node.getType());
    } else if (semaIterType && semaIterType->isArray()) {
        const auto& arr = static_cast<const sema::ArrayType&>(*semaIterType);
        itemIRType = mapType(arr.getElementType().get());
    } else if (isMap) {
        // Phase 6 slice A: a map iterates over its keys.
        const auto& mapTy = static_cast<const sema::MapType&>(*semaIterType);
        itemIRType = mapType(mapTy.getKeyType().get());
    } else if (isList) {
        std::string s = semaIterType->toString();
        size_t lt = s.find('<');
        size_t gt = s.rfind('>');
        if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
            itemIRType = mapType(s.substr(lt + 1, gt - lt - 1));
        }
    }
    if (!itemIRType) itemIRType = IRType::getInt32();

    std::string varReg = "%" + node.getVarName() + "_" + std::to_string(m_varCounter++);
    // Phase 5 slice A: in the state machine the loop var lives in a
    // heap-frame slot (an entry-block alloca would not survive an
    // await in the body).
    IRValuePtr varAlloca;
    if (inAsyncSM()) {
        varAlloca = asyncNodeSlotGEP(&node, itemIRType, node.getLocation());
    } else {
        auto alloca = std::make_shared<AllocaInst>(itemIRType, varReg);
        m_currentFunction->getEntryBlock()->addInstruction(alloca);
        varAlloca = alloca;
    }

    // 4. Create blocks
    auto condBB = createBlock("for_in_cond");
    auto bodyBB = createBlock("for_in_body");
    auto stepBB = createBlock("for_in_step");
    auto endBB = createBlock("for_in_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // 5. Condition block: check idx < len
    setInsertPoint(condBB);
    std::string curIdxReg = m_currentFunction->nextTempName("cur_idx");
    auto curIdx = std::make_shared<LoadInst>(IRType::getInt64(), idxAlloca, curIdxReg);
    condBB->addInstruction(curIdx);
    std::string cmpReg = m_currentFunction->nextTempName("for_cmp");
    auto cmpInst = std::make_shared<CompareInst>(IROpcode::ICmpSlt, curIdx, lenCall, cmpReg);
    condBB->addInstruction(cmpInst);
    condBB->addInstruction(std::make_shared<CondBranchInst>(cmpInst, bodyBB->getName(), endBB->getName()));

    // 6. Body block
    setInsertPoint(bodyBB);
    pushScope();
    declareVariable(node.getVarName(), varAlloca, itemIRType);

    // Fetch element at curIdx and store into varAlloca
    if (isMap) {
        // Phase 6 slice A: map keys, in table order (the runtime's
        // key_at accessors). Before this fix every non-List iterable
        // was treated as an array, so sp_rt_array_length read the
        // map's entry-table pointer as a length and the loop never
        // terminated, yielding garbage keys forever. The fetch mirrors
        // the map index-assignment path: string keys via the str table,
        // every other key via the i64 table. Iteration order is table
        // order (unspecified) — do not mutate the map while iterating.
        const auto& mapTy = static_cast<const sema::MapType&>(*semaIterType);
        auto keyType = mapTy.getKeyType();
        if (keyType && keyType->isString()) {
            auto getCall = std::make_shared<CallInst>(
                IRType::getPointer(IRType::getInt8()), "sp_rt_map_str_key_at",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("key"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        } else {
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(), "sp_rt_map_int_key_at",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("key"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        }
    } else if (isList) {
        if (itemIRType->isFloatingPoint()) {
            auto getCall = std::make_shared<CallInst>(
                IRType::getDouble(), "sp_rt_list_get_f64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        } else {
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(), "sp_rt_list_get_i64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        }
    } else {
        if (itemIRType->isFloatingPoint()) {
            auto getCall = std::make_shared<CallInst>(
                IRType::getDouble(), "sp_rt_array_get_f64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        } else {
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(), "sp_rt_array_get_i64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        }
    }

    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(stepBB->getName());

    node.getBody().accept(*this);

    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(stepBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();
    popScope();

    // 7. Step block: idx++
    setInsertPoint(stepBB);
    auto stepIdx = std::make_shared<LoadInst>(IRType::getInt64(), idxAlloca, m_currentFunction->nextTempName("step_idx"));
    stepBB->addInstruction(stepIdx);
    auto nextIdx = std::make_shared<BinaryInst>(
        IROpcode::Add, stepIdx, std::make_shared<ConstantInt>(IRType::getInt64(), 1),
        m_currentFunction->nextTempName("next_idx"));
    stepBB->addInstruction(nextIdx);
    stepBB->addInstruction(std::make_shared<StoreInst>(nextIdx, idxAlloca));
    stepBB->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // 8. End block
    setInsertPoint(endBB);
}
void IRBuilder::visit(const ast::SwitchStmt& node) {
    if (!m_currentFunction || !m_currentBlock) return;

    node.getSubject().accept(*this);
    IRValuePtr subjVal = m_lastExprValue;
    IRTypePtr subjType = m_lastExprType;
    if (!subjVal) return;

    auto endBB = createBlock("switch_end");
    m_breakStack.push_back(endBB->getName());

    const auto& cases = node.getCases();
    size_t numCases = cases.size();

    auto defaultBB = node.getDefaultStatements().empty() ? nullptr : createBlock("switch_default");
    std::string defaultTarget = defaultBB ? defaultBB->getName() : endBB->getName();

    if (numCases == 0) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(defaultTarget));
    } else {
        std::vector<IRBasicBlock*> testBBs;
        std::vector<IRBasicBlock*> bodyBBs;
        for (size_t i = 0; i < numCases; ++i) {
            testBBs.push_back(createBlock("switch_test"));
            bodyBBs.push_back(createBlock("switch_body"));
        }

        m_currentBlock->addInstruction(std::make_shared<BranchInst>(testBBs[0]->getName()));

        auto semaSubj = m_sema.getNodeType(&node.getSubject());
        bool isSubjStr = (semaSubj && semaSubj->isString()) || (subjType && subjType->isPointer());

        for (size_t i = 0; i < numCases; ++i) {
            setInsertPoint(testBBs[i]);
            cases[i].getValue().accept(*this);
            IRValuePtr caseVal = m_lastExprValue;
            IRTypePtr caseType = m_lastExprType;

            IRValuePtr condVal = nullptr;
            if (isSubjStr || (caseType && caseType->isPointer())) {
                auto call = std::make_shared<CallInst>(
                    IRType::getInt1(), "sp_rt_str_equals",
                    std::vector<IRValuePtr>{subjVal, caseVal},
                    m_currentFunction->nextTempName("str_eq"));
                testBBs[i]->addInstruction(call);
                condVal = call;
            } else {
                bool isFloat = (subjType && subjType->isFloatingPoint()) || (caseType && caseType->isFloatingPoint());
                IRTypePtr cmpType = isFloat ? IRType::getDouble()
                                            : ((subjType && subjType->isInt64()) || (caseType && caseType->isInt64())
                                                   ? IRType::getInt64()
                                                   : IRType::getInt32());
                auto l = emitCastIfNeeded(subjVal, cmpType);
                auto r = emitCastIfNeeded(caseVal, cmpType);
                auto cmp = std::make_shared<CompareInst>(
                    isFloat ? IROpcode::FCmpEq : IROpcode::ICmpEq,
                    l, r, m_currentFunction->nextTempName("cmp"));
                testBBs[i]->addInstruction(cmp);
                condVal = cmp;
            }

            std::string nextTest = (i + 1 < numCases) ? testBBs[i + 1]->getName() : defaultTarget;
            testBBs[i]->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBBs[i]->getName(), nextTest));

            // Case body
            setInsertPoint(bodyBBs[i]);
            pushScope();
            for (const auto& s : cases[i].getStatements()) {
                if (s) s->accept(*this);
            }
            popScope();

            if (!m_currentBlock->hasTerminator()) {
                std::string fallthrough = (i + 1 < numCases) ? bodyBBs[i + 1]->getName() : defaultTarget;
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(fallthrough));
            }
        }
    }

    if (defaultBB) {
        setInsertPoint(defaultBB);
        pushScope();
        for (const auto& s : node.getDefaultStatements()) {
            if (s) s->accept(*this);
        }
        popScope();
        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(endBB->getName()));
        }
    }

    m_breakStack.pop_back();
    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::MatchStmt& node) {
    if (!m_currentFunction || !m_currentBlock) return;

    node.getSubject().accept(*this);
    IRValuePtr subjVal = m_lastExprValue;
    IRTypePtr subjType = m_lastExprType;
    if (!subjVal) return;

    auto endBB = createBlock("match_end");

    const auto& arms = node.getArms();
    size_t numArms = arms.size();

    if (numArms == 0) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(endBB->getName()));
        setInsertPoint(endBB);
        return;
    }

    std::vector<IRBasicBlock*> testBBs;
    std::vector<IRBasicBlock*> bodyBBs;
    for (size_t i = 0; i < numArms; ++i) {
        testBBs.push_back(createBlock("match_test"));
        bodyBBs.push_back(createBlock("match_body"));
    }

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(testBBs[0]->getName()));

    auto semaSubj = m_sema.getNodeType(&node.getSubject());
    bool isSubjStr = (semaSubj && semaSubj->isString()) || (subjType && subjType->isPointer());

    for (size_t i = 0; i < numArms; ++i) {
        setInsertPoint(testBBs[i]);
        const auto& arm = arms[i];

        if (arm.isWildcard()) {
            testBBs[i]->addInstruction(std::make_shared<BranchInst>(bodyBBs[i]->getName()));
        } else {
            arm.getPattern()->accept(*this);
            IRValuePtr patVal = m_lastExprValue;
            IRTypePtr patType = m_lastExprType;

            IRValuePtr condVal = nullptr;
            if (isSubjStr || (patType && patType->isPointer())) {
                auto call = std::make_shared<CallInst>(
                    IRType::getInt1(), "sp_rt_str_equals",
                    std::vector<IRValuePtr>{subjVal, patVal},
                    m_currentFunction->nextTempName("str_eq"));
                testBBs[i]->addInstruction(call);
                condVal = call;
            } else {
                bool isFloat = (subjType && subjType->isFloatingPoint()) || (patType && patType->isFloatingPoint());
                IRTypePtr cmpType = isFloat ? IRType::getDouble()
                                            : ((subjType && subjType->isInt64()) || (patType && patType->isInt64())
                                                   ? IRType::getInt64()
                                                   : IRType::getInt32());
                auto l = emitCastIfNeeded(subjVal, cmpType);
                auto r = emitCastIfNeeded(patVal, cmpType);
                auto cmp = std::make_shared<CompareInst>(
                    isFloat ? IROpcode::FCmpEq : IROpcode::ICmpEq,
                    l, r, m_currentFunction->nextTempName("cmp"));
                testBBs[i]->addInstruction(cmp);
                condVal = cmp;
            }

            std::string nextTest = (i + 1 < numArms) ? testBBs[i + 1]->getName() : endBB->getName();
            testBBs[i]->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBBs[i]->getName(), nextTest));
        }

        // Arm body
        setInsertPoint(bodyBBs[i]);
        pushScope();
        arm.getBody().accept(*this);
        popScope();

        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(endBB->getName()));
        }
    }

    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::ReturnStmt& node) {
    // Phase 5 slice A: a return from an `async fn` stores the Task
    // payload in the frame retVal slot and branches to the finish
    // block (which completes the task and frees the frame). A struct
    // return is boxed (the retVal slot holds the box pointer); other
    // payloads are stored directly. A return inside a try with a
    // finally is deferred the same way as in sync code: stash the
    // value, set the flag, run the finally, then finish. A string
    // return is retained here and released in the finish block (after
    // the task copies it), so it survives the scope drops below.
    if (inAsyncSM()) {
        IRBasicBlock* finallyTarget = findFinallyTarget();
        if (node.hasValue()) {
            node.getValue()->accept(*this);
            IRValuePtr retVal = m_lastExprValue;
            const std::string& kind = m_asyncSM->payloadKind;
            if (kind == "struct") {
                if (retVal && retVal->getType()->isPointer()) {
                    auto loadInst = std::make_shared<LoadInst>(
                        m_asyncSM->structType, retVal,
                        m_currentFunction->nextTempName("tu_ret"));
                    m_currentBlock->addInstruction(loadInst);
                    retVal = loadInst;
                }
                int64_t boxSize = asyncStructSizeBytes(m_asyncSM->structType);
                auto box = std::make_shared<CallInst>(
                    IRType::getPointer(), "sp_rt_alloc",
                    std::vector<IRValuePtr>{std::make_shared<ConstantInt>(IRType::getInt64(), boxSize)},
                    m_currentFunction->nextTempName("async_ret_box"));
                m_currentBlock->addInstruction(box);
                if (retVal) {
                    m_currentBlock->addInstruction(std::make_shared<StoreInst>(retVal, box));
                }
                retVal = box;
            } else if (kind == "int" || kind == "bool") {
                if (retVal) retVal = emitCastIfNeeded(retVal, IRType::getInt64());
            } else if (kind == "string" || kind == "array") {
                if (retVal) retVal = emitCastIfNeeded(retVal, IRType::getPointer());
            } else if (kind == "double") {
                if (retVal) retVal = emitCastIfNeeded(retVal, IRType::getDouble());
            }
            if (kind == "string" && retVal) {
                m_currentBlock->addInstruction(std::make_shared<CallInst>(
                    IRType::getVoid(), "sp_rt_retain",
                    std::vector<IRValuePtr>{retVal}));
            }
            auto slot = getOrCreateReturnValueSlot();
            if (slot && retVal) {
                m_currentBlock->addInstruction(std::make_shared<StoreInst>(retVal, slot));
            }
            if (finallyTarget) {
                auto flagSlot = getOrCreateReturnPendingSlot();
                m_currentBlock->addInstruction(std::make_shared<StoreInst>(
                    std::make_shared<ConstantInt>(IRType::getInt1(), 1), flagSlot));
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(finallyTarget->getName()));
                return;
            }
            emitAllFunctionDrops();
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_asyncSM->finishBB->getName()));
        } else {
            if (finallyTarget) {
                auto flagSlot = getOrCreateReturnPendingSlot();
                m_currentBlock->addInstruction(std::make_shared<StoreInst>(
                    std::make_shared<ConstantInt>(IRType::getInt1(), 1), flagSlot));
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(finallyTarget->getName()));
                return;
            }
            emitAllFunctionDrops();
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_asyncSM->finishBB->getName()));
        }
        return;
    }
    // Phase 3 slice B: a return inside a try with a finally must run
    // the finally blocks first. Defer it: stash the value, set the
    // flag, and branch to the innermost finally; that finally's
    // dispatch completes the return (possibly via outer finallys).
    // A return inside a try WITHOUT a finally returns normally.
    IRBasicBlock* finallyTarget = findFinallyTarget();
    if (finallyTarget) {
        if (node.hasValue()) {
            node.getValue()->accept(*this);
            IRValuePtr retVal = m_lastExprValue;
            if (m_currentFunction->getReturnType()->isStruct() && retVal && retVal->getType()->isPointer()) {
                auto loadInst = std::make_shared<LoadInst>(m_currentFunction->getReturnType(), retVal, m_currentFunction->nextTempName("tu_ret"));
                m_currentBlock->addInstruction(loadInst);
                retVal = loadInst;
            }
            retVal = emitCastIfNeeded(retVal, m_currentFunction->getReturnType());

            // Retain a reference return value first: the deferred
            // return block releases the function's locals, and the
            // returned object must survive that.
            bool isRefRet = false;
            if (retVal && retVal->getType()->isPointer()) {
                auto semaType = m_sema.getNodeType(node.getValue());
                if (semaType && semaType->isReferenceType()) {
                    isRefRet = true;
                } else if (dynamic_cast<const ast::NewExpr*>(node.getValue())) {
                    isRefRet = true;
                }
            }
            if (isRefRet) {
                auto retainCall = std::make_shared<CallInst>(
                    IRType::getVoid(), "sp_rt_retain", std::vector<IRValuePtr>{retVal});
                m_currentBlock->addInstruction(retainCall);
            }

            auto slot = getOrCreateReturnValueSlot();
            if (slot && retVal) {
                m_currentBlock->addInstruction(std::make_shared<StoreInst>(retVal, slot));
            }
            auto flagSlot = getOrCreateReturnPendingSlot();
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(
                std::make_shared<ConstantInt>(IRType::getInt1(), 1), flagSlot));
        } else {
            // Bare return: for main the deferred value is 0.
            if (m_currentFunction->getName() == "main") {
                auto slot = getOrCreateReturnValueSlot();
                if (slot) {
                    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0), slot));
                }
            }
            auto flagSlot = getOrCreateReturnPendingSlot();
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(
                std::make_shared<ConstantInt>(IRType::getInt1(), 1), flagSlot));
        }
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(finallyTarget->getName()));
        return;
    }
    if (node.hasValue()) {
        node.getValue()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        if (m_currentFunction->getReturnType()->isStruct() && retVal && retVal->getType()->isPointer()) {
            auto loadInst = std::make_shared<LoadInst>(m_currentFunction->getReturnType(), retVal, m_currentFunction->nextTempName("tu_ret"));
            m_currentBlock->addInstruction(loadInst);
            retVal = loadInst;
        }
        retVal = emitCastIfNeeded(retVal, m_currentFunction->getReturnType());

        bool isRefRet = false;
        if (retVal && retVal->getType()->isPointer()) {
            auto semaType = m_sema.getNodeType(node.getValue());
            if (semaType && semaType->isReferenceType()) {
                isRefRet = true;
            } else if (dynamic_cast<const ast::NewExpr*>(node.getValue())) {
                isRefRet = true;
            }
        }

        if (isRefRet) {
            auto retainCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_retain",
                std::vector<IRValuePtr>{retVal}
            );
            m_currentBlock->addInstruction(retainCall);
        }

        // Phase 3 slice C: an explicit `return` from main drains
        // fire-and-forget async tasks first. (A return deferred by a
        // try/finally skips this drain — known limitation, see the
        // Phase 32 doc.)
        if (m_currentFunction->getName() == "main") emitMainExitPump();
        emitAllFunctionDrops();
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    } else {
        if (m_currentFunction->getName() == "main") emitMainExitPump();
        emitAllFunctionDrops();
        if (m_currentFunction->getName() == "main") {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)));
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        }
    }
}

void IRBuilder::visit(const ast::BreakStmt& node) {
    (void)node;
    if (!m_breakStack.empty()) {
        // Phase 3 slice B: a break that would leave a try with a
        // finally must run the finally first, which this lowering does
        // not support yet. Fail loudly instead of silently skipping
        // the finally. (A break of a loop entered *inside* the try
        // does not leave it, so it is allowed.)
        for (const auto& frame : m_tryStack) {
            if (frame.finallyBlock && m_breakStack.size() <= frame.breakDepth) {
                reportUnsupported(node.getLocation(), "break inside try with finally",
                                  "break out of a try block with a finally clause is not supported yet; restructure the loop or move the finally");
                return;
            }
        }
        if (!m_refCountedScopes.empty()) {
            emitScopeDrops(m_refCountedScopes.size() - 1);
        }
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_breakStack.back()));
    }
}

void IRBuilder::visit(const ast::ContinueStmt& node) {
    (void)node;
    if (!m_continueStack.empty()) {
        // Phase 3 slice B: same rule as break (see above).
        for (const auto& frame : m_tryStack) {
            if (frame.finallyBlock && m_continueStack.size() <= frame.continueDepth) {
                reportUnsupported(node.getLocation(), "continue inside try with finally",
                                  "continue out of a try block with a finally clause is not supported yet; restructure the loop or move the finally");
                return;
            }
        }
        if (!m_refCountedScopes.empty()) {
            emitScopeDrops(m_refCountedScopes.size() - 1);
        }
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_continueStack.back()));
    }
}

void IRBuilder::visit(const ast::TryCatchStmt& node) {
    // Phase 3 slice B (pro-4.0.0): real try/catch/finally lowering, in
    // the ADR-002 direction (Result/Option + `?` for cross-function
    // errors; throw/catch is local control flow inside one function).
    //
    // SEMANTICS (see docs/v2/PHASE-32-PRO-4.0.0.md for the full
    // contract, as amended by Phase 5 slice B in
    // docs/v2/PHASE-34-SLICE-B.md):
    // - The try body runs. A `throw` stores the stringified value in
    //   the shared payload slot, the thrown value's static-type tag in
    //   the type slot, and the bit-preserving raw value in the raw slot
    //   (pointers via PtrToInt, scalars bit-packed; by-value structs
    //   go to a per-type struct slot). It sets the pending flag,
    //   releases the refcounted locals of the try being left, and
    //   branches to the catch dispatch. Statements after the throw do
    //   not run.
    // - The catch dispatch tries each clause in order: a clause
    //   matches when the thrown tag is its own tag (or, for a
    //   class/interface clause, the tag of a subclass of it). A
    //   catch-all clause (unresolvable name, incl. the conventional
    //   `Exception` spelling, or `Any`) matches anything. The first
    //   match binds the catch variable to the value in its declared
    //   type and runs; no match leaves the pending flag set and
    //   propagates outward. The catch variable is bound to the payload
    //   string for catch-all clauses.
    // - The finally block runs on every exit path: normal completion,
    //   after the catch, and before a deferred return / propagated
    //   throw leaves the try.
    // - A `throw` with no enclosing catch propagates outward: to the
    //   next outer try's catch/finally, else to the uncaught block
    //   (Err in a Result function, None in an Option function, panic
    //   otherwise). A `return` inside a try with a finally is deferred
    //   the same way (value stashed, flag set, finally runs first).
    // - `break`/`continue` out of a try with a finally is a loud
    //   error[E1000] (see visit(BreakStmt)/visit(ContinueStmt)); a
    //   break/continue of a loop entered *inside* the try is fine.
    bool hasCatch = !node.getCatches().empty();
    bool hasFinally = node.hasFinally() && node.getFinallyBlock();
    IRBasicBlock* tryBB = createBlock("try");
    IRBasicBlock* catchBB = hasCatch ? createBlock("catch") : nullptr;
    IRBasicBlock* finallyBB = hasFinally ? createBlock("finally") : nullptr;
    IRBasicBlock* mergeBB = createBlock("try_merge");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(tryBB->getName()));

    TryFrame frame;
    frame.catchBlock = catchBB;
    frame.finallyBlock = finallyBB;
    frame.mergeBlock = mergeBB;
    frame.breakDepth = m_breakStack.size();
    frame.continueDepth = m_continueStack.size();
    frame.refScopeDepth = m_refCountedScopes.size();
    m_tryStack.push_back(frame);

    // --- try body ---
    setInsertPoint(tryBB);
    node.getTryBlock().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(
            finallyBB ? finallyBB->getName() : mergeBB->getName()));
    }
    m_tryStack.pop_back();

    // --- catch dispatch (Phase 5 slice B: typed catches) ---
    if (hasCatch) {
        // Load the thrown value's type tag once; each clause is tried
        // in order against it.
        setInsertPoint(catchBB);
        auto tagSlot = getOrCreateThrowTypeSlot();
        auto tagLoad = std::make_shared<LoadInst>(IRType::getInt32(), tagSlot,
                                                  m_currentFunction->nextTempName("catch_tag"));
        m_currentBlock->addInstruction(tagLoad);
        IRBasicBlock* noMatchBB = createBlock("catch_nomatch");
        IRBasicBlock* checkBB = catchBB;
        for (size_t ci = 0; ci < node.getCatches().size(); ++ci) {
            const auto& clause = node.getCatches()[ci];
            auto tags = catchClauseTags(clause);
            IRBasicBlock* clauseBB = createBlock("catch_clause");
            bool isLast = (ci + 1 == node.getCatches().size());
            if (tags.empty()) {
                // Catch-all: unconditional. Later clauses are dead
                // code (still built, with terminators).
                setInsertPoint(checkBB);
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(clauseBB->getName()));
            } else {
                // tag == t1 || tag == t2 || ...
                setInsertPoint(checkBB);
                IRValuePtr matchVal = nullptr;
                for (int32_t t : tags) {
                    auto cmp = std::make_shared<CompareInst>(IROpcode::ICmpEq, tagLoad,
                        std::make_shared<ConstantInt>(IRType::getInt32(), t),
                        m_currentFunction->nextTempName("catch_match"));
                    m_currentBlock->addInstruction(cmp);
                    if (!matchVal) {
                        matchVal = cmp;
                    } else {
                        auto orInst = std::make_shared<BinaryInst>(IROpcode::Or, matchVal, cmp,
                            m_currentFunction->nextTempName("catch_match_or"));
                        m_currentBlock->addInstruction(orInst);
                        matchVal = orInst;
                    }
                }
                IRBasicBlock* nextBB = isLast ? noMatchBB : createBlock("catch_check");
                m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(
                    matchVal, clauseBB->getName(), nextBB->getName()));
                checkBB = nextBB;
            }

            // --- clause body ---
            setInsertPoint(clauseBB);
            // The throw is handled now: clear the pending flag, or the
            // finally dispatch below would propagate it again.
            auto flagSlot = getOrCreateThrowPendingSlot();
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(
                std::make_shared<ConstantInt>(IRType::getInt1(), 0), flagSlot));
            pushScope();
            // Bind the catch variable to the thrown value in its
            // declared type.
            const std::string& canonName = clause.getResolvedTypeName();
            bool isCatchAll = canonName.empty() || canonName == "any" || canonName == "Exception";
            IRTypePtr clauseIRType = nullptr;
            IRValuePtr boundVal = nullptr;
            bool clauseIsClass = false;
            bool clauseIsStruct = false;
            if (isCatchAll) {
                clauseIRType = IRType::getPointer(IRType::getInt8());
                auto payloadSlot = getOrCreateThrowPayloadSlot();
                auto payloadLoad = std::make_shared<LoadInst>(clauseIRType, payloadSlot,
                    m_currentFunction->nextTempName("catch_str"));
                m_currentBlock->addInstruction(payloadLoad);
                boundVal = payloadLoad;
            } else {
                auto clauseSemaType = m_sema.getTypeContext().findUserType(canonName);
                clauseIsClass = clauseSemaType && (clauseSemaType->isClass() || clauseSemaType->isInterface());
                clauseIsStruct = clauseSemaType && clauseSemaType->isStruct();
                if (clauseIsStruct) {
                    // By-value struct: the value lives in the per-type
                    // struct slot (written by the throw site).
                    const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(canonName) : nullptr;
                    bool layoutOk = layout && layout->structType;
                    if (layoutOk) {
                        clauseIRType = layout->structType;
                    } else {
                        reportUnsupported(node.getLocation(),
                            "catch of struct '" + canonName + "'",
                            "no struct layout is available for this type; the catch variable cannot be bound");
                        clauseIRType = IRType::getPointer(IRType::getInt8());
                    }
                    auto slotIt = m_throwStructSlots.find(canonName);
                    if (layoutOk && slotIt != m_throwStructSlots.end()) {
                        auto structLoad = std::make_shared<LoadInst>(clauseIRType, slotIt->second,
                            m_currentFunction->nextTempName("catch_struct"));
                        m_currentBlock->addInstruction(structLoad);
                        boundVal = structLoad;
                    } else {
                        // No throw of this struct type exists in the
                        // try body, so this clause can never match
                        // here; bind a default value so lowering can
                        // continue (the clause body is dead code for
                        // this try).
                        boundVal = makeDefaultValue(clauseIRType);
                    }
                } else {
                    clauseIRType = mapType(canonName);
                    auto rawSlot = getOrCreateThrowRawSlot();
                    auto rawLoad = std::make_shared<LoadInst>(IRType::getInt64(), rawSlot,
                        m_currentFunction->nextTempName("catch_raw"));
                    m_currentBlock->addInstruction(rawLoad);
                    boundVal = emitUnpackThrowValue(rawLoad, clauseIRType);
                }
            }
            auto catchAlloca = std::make_shared<AllocaInst>(clauseIRType,
                m_currentFunction->nextTempName("catch_" + clause.getName()));
            m_currentFunction->getEntryBlock()->addInstruction(catchAlloca);
            declareVariable(clause.getName(), catchAlloca, clauseIRType);
            if (boundVal) {
                boundVal = emitCastIfNeeded(boundVal, clauseIRType);
                m_currentBlock->addInstruction(std::make_shared<StoreInst>(boundVal, catchAlloca));
            }
            // Ownership of a bound class object (Phase 5 slice B):
            // the payload holds one owning reference when the
            // throw-owns flag is set (directly-thrown `new`, or a
            // value retained into the payload at the throw site). The
            // catch variable takes that share without retaining; a
            // borrowed value is retained here, so the catch variable
            // is a new owner either way. The variable is registered
            // for release when the catch body ends.
            // A catch-all clause binds the string, not the object: a
            // discarded owned object is released here instead.
            IRBasicBlock* bodyBB = createBlock("catch_body");
            if (clauseIsClass || isCatchAll) {
                auto ownsSlot = getOrCreateThrowOwnsSlot();
                auto ownsLoad = std::make_shared<LoadInst>(IRType::getInt1(), ownsSlot,
                    m_currentFunction->nextTempName("catch_owns"));
                m_currentBlock->addInstruction(ownsLoad);
                auto ownedBB = createBlock(clauseIsClass ? "catch_take" : "catch_discard");
                auto borrowedBB = createBlock(clauseIsClass ? "catch_retain" : "catch_keep");
                m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(
                    ownsLoad, ownedBB->getName(), borrowedBB->getName()));
                setInsertPoint(ownedBB);
                if (isCatchAll) {
                    auto rawSlot = getOrCreateThrowRawSlot();
                    auto rawLoad = std::make_shared<LoadInst>(IRType::getInt64(), rawSlot,
                        m_currentFunction->nextTempName("catch_raw"));
                    ownedBB->addInstruction(rawLoad);
                    auto rawPtr = std::make_shared<CastInst>(IROpcode::IntToPtr, rawLoad,
                        IRType::getPointer(), m_currentFunction->nextTempName("catch_obj"));
                    ownedBB->addInstruction(rawPtr);
                    ownedBB->addInstruction(std::make_shared<CallInst>(
                        IRType::getVoid(), "sp_rt_release", std::vector<IRValuePtr>{rawPtr}));
                }
                ownedBB->addInstruction(std::make_shared<BranchInst>(bodyBB->getName()));
                setInsertPoint(borrowedBB);
                if (clauseIsClass) {
                    auto rawSlot = getOrCreateThrowRawSlot();
                    auto rawLoad = std::make_shared<LoadInst>(IRType::getInt64(), rawSlot,
                        m_currentFunction->nextTempName("catch_raw"));
                    borrowedBB->addInstruction(rawLoad);
                    auto rawPtr = std::make_shared<CastInst>(IROpcode::IntToPtr, rawLoad,
                        IRType::getPointer(), m_currentFunction->nextTempName("catch_obj"));
                    borrowedBB->addInstruction(rawPtr);
                    borrowedBB->addInstruction(std::make_shared<CallInst>(
                        IRType::getVoid(), "sp_rt_retain", std::vector<IRValuePtr>{rawPtr}));
                }
                borrowedBB->addInstruction(std::make_shared<BranchInst>(bodyBB->getName()));
            } else {
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(bodyBB->getName()));
            }
            if (clauseIsClass) {
                m_refCountedScopes.back().push_back({clause.getName(), catchAlloca});
            }
            // Throws inside the catch run the finally, then propagate
            // (this frame has no catch of its own).
            setInsertPoint(bodyBB);
            TryFrame catchFrame;
            catchFrame.catchBlock = nullptr;
            catchFrame.finallyBlock = finallyBB;
            catchFrame.mergeBlock = mergeBB;
            catchFrame.breakDepth = m_breakStack.size();
            catchFrame.continueDepth = m_continueStack.size();
            catchFrame.refScopeDepth = m_refCountedScopes.size() - 1;
            m_tryStack.push_back(catchFrame);
            clause.getBody().accept(*this);
            m_tryStack.pop_back();
            popScope();
            if (!m_currentBlock->hasTerminator()) {
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(
                    finallyBB ? finallyBB->getName() : mergeBB->getName()));
            }
        }

        // --- no clause matched: the throw is still in flight ---
        setInsertPoint(noMatchBB);
        if (finallyBB) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(finallyBB->getName()));
        } else {
            IRBasicBlock* target = findThrowTarget();
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(target->getName()));
        }
    }

    // --- finally body (no frame: returns/throws inside it execute
    // immediately against the outer frames) ---
    if (hasFinally) {
        setInsertPoint(finallyBB);
        node.getFinallyBlock()->accept(*this);
        if (!m_currentBlock->hasTerminator()) {
            emitFinallyDispatch(mergeBB);
        }
    }

    setInsertPoint(mergeBB);
}

void IRBuilder::visit(const ast::ThrowStmt& node) {
    // Phase 5 slice B: typed + composite `throw`. The thrown value is
    // routed to emitThrowValue, which stores it in the throw slots
    // (stringified payload, type tag, bit-packed raw value, owning
    // flag) and branches to the innermost catch dispatch, else the
    // finally, else outward.
    //
    // What can be thrown, by representation:
    // - Scalars (string, integer, bool, char, float, double): as
    //   before -- stringified for the message, bit-packed raw for a
    //   typed catch binding.
    // - Pointer values (arrays, maps, class objects, fn values):
    //   the raw pointer round-trips through the raw slot; the
    //   stringified payload is a "[<type>]" placeholder (a raw array/
    //   map/object pointer is not a C string and must never reach
    //   sp_rt_panic as one).
    // - By-value user structs: copied into a per-type struct slot.
    // - Result/Option values are NOT throwable: they are the
    //   return-value error channel (ADR-002 direction), not
    //   exceptions. Throwing one would discard its variant at the
    //   uncaught boundary (an uncaught Result throw in a Result
    //   function would have to become Err of a placeholder string).
    //   This is a deliberate semantic restriction, not a technical
    //   limitation -- the slot machinery could carry the struct.
    auto semaType = m_sema.getNodeType(&node.getExpr());
    if (semaType && semaType->isEnum()) {
        if (const auto* enumType = dynamic_cast<const sema::EnumType*>(semaType.get())) {
            if (enumType->isTaggedUnion()) {
                reportUnsupported(node.getLocation(), "throw of '" + semaType->toString() + "'",
                    "Result/Option values are not throwable: they are the return-value error channel, "
                    "so return Err/None (or use `?`) instead of throwing one");
                m_lastExprValue = nullptr;
                m_lastExprType = nullptr;
                return;
            }
        }
    }
    node.getExpr().accept(*this);
    IRValuePtr thrownVal = m_lastExprValue;
    IRTypePtr thrownType = m_lastExprType;
    if (!thrownVal) {
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }

    std::string canonical = semaType ? semaType->toString() : "";
    int32_t typeTag = canonical.empty() ? 0 : throwTypeTagFor(canonical);
    IRValuePtr strVal = nullptr;
    IRValuePtr rawI64 = nullptr;
    bool owns = false;
    if (semaType && semaType->isStruct()) {
        // By-value user struct: copy into the per-type struct slot. A
        // struct-typed expression yields either the struct value
        // itself (a struct variable/field) or a pointer to one (a
        // struct-returning call returns a pointer; cf. VarDecl's
        // struct load) -- load through the pointer in the latter case.
        const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(canonical) : nullptr;
        if (!layout || !layout->structType) {
            reportUnsupported(node.getLocation(), "throw of '" + canonical + "'",
                "no struct layout is available for this type; the value cannot be carried by a throw");
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }
        IRValuePtr structVal = thrownVal;
        if (thrownType && thrownType->isPointer()) {
            auto load = std::make_shared<LoadInst>(layout->structType, thrownVal,
                m_currentFunction->nextTempName("throw_struct"));
            m_currentBlock->addInstruction(load);
            structVal = load;
        }
        auto slot = getOrCreateThrowStructSlot(canonical, layout->structType);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(structVal, slot));
        strVal = m_module->getOrCreateStringLiteral("[" + canonical + "]");
    } else if (thrownType && thrownType->isPointer()) {
        // Pointer value: raw bits through the raw slot.
        rawI64 = emitCastIfNeeded(thrownVal, IRType::getInt64());
        if (semaType && semaType->isString()) {
            strVal = thrownVal;
        } else {
            strVal = m_module->getOrCreateStringLiteral("[" + canonical + "]");
        }
        if (semaType && semaType->isReferenceType()) {
            // Class object: the payload must hold an owning
            // reference, because the unwinding scope drops below
            // release the try body's locals (which may alias this
            // object). A directly-thrown `new` transfers its fresh
            // count-1 to the payload; any other class value is
            // retained into the payload here.
            owns = true;
            if (!dynamic_cast<const ast::NewExpr*>(&node.getExpr())) {
                auto retainCall = std::make_shared<CallInst>(
                    IRType::getVoid(), "sp_rt_retain", std::vector<IRValuePtr>{thrownVal});
                m_currentBlock->addInstruction(retainCall);
            }
        }
    } else {
        // Scalar: stringify for the message / catch-all binding, and
        // bit-pack the raw value for a typed catch binding.
        strVal = convertToStringValue(thrownVal, thrownType, &node.getExpr());
        rawI64 = emitPackThrowScalar(thrownVal, thrownType);
    }
    emitThrowValue(strVal, rawI64, typeTag, owns);
    m_lastExprValue = nullptr;
    m_lastExprType = nullptr;
}

void IRBuilder::visit(const ast::ClassDecl& node) {
    if (!m_vtableBuilder) return;
    codegen::ClassGenerator classGen(*this, *m_module, *m_vtableBuilder);
    classGen.generateClass(node);
}

void IRBuilder::compileDefaultConstructor(const ast::ClassDecl& classDecl, const codegen::ClassLayout& layout) {
    (void)classDecl;
    std::string mangledName = layout.name + "_" + layout.name;
    auto thisParam = std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0);
    auto func = m_module->createFunction(mangledName, IRType::getVoid(), {thisParam});

    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
    entryBB->addInstruction(allocaThis);
    entryBB->addInstruction(std::make_shared<StoreInst>(thisParam, allocaThis));
    declareVariable("this", allocaThis, IRType::getPointer());

    if (layout.hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout.vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout.structType,
            thisParam,
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            func->nextTempName("vptr")
        );
        entryBB->addInstruction(gep);
        entryBB->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    entryBB->addInstruction(std::make_shared<ReturnInst>());

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileConstructor(const ast::ClassDecl& classDecl, const ast::FunctionDecl* ctorDecl, const codegen::ClassLayout& layout) {
    (void)classDecl;
    std::string mangledName = layout.name + "_" + layout.name;
    std::vector<std::shared_ptr<ParameterValue>> params;
    params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i + 1));
        }
    }

    auto func = m_module->createFunction(mangledName, IRType::getVoid(), params);
    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
    entryBB->addInstruction(allocaThis);
    entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
    declareVariable("this", allocaThis, IRType::getPointer());

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            const auto& paramVal = params[i + 1];
            auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
            entryBB->addInstruction(allocaParam);
            entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
            declareVariable(p.getName(), allocaParam, paramVal->getType());
        }
    }

    if (layout.hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout.vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout.structType,
            params[0],
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            func->nextTempName("vptr")
        );
        entryBB->addInstruction(gep);
        entryBB->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    if (ctorDecl && ctorDecl->hasBody()) {
        for (const auto& stmt : ctorDecl->getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    }

    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
    }

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileMethod(const ast::ClassDecl& classDecl, const ast::FunctionDecl& methodDecl, const codegen::ClassLayout& layout) {
    (void)classDecl;
    // Phase 3 slice A: overloaded methods are mangled by signature.
    std::string mangledName = llvmMethodName(layout.name, methodDecl);
    // Phase 5 slice A: an `async` method body is the state-machine
    // entry `void <name>(ptr %task, ptr %ctx)` (`this` arrives through
    // the ctx, like any other argument).
    const bool isAsync = methodDecl.isAsync();
    IRTypePtr returnType = isAsync ? IRType::getVoid() : mapType(methodDecl.getReturnType());

    std::vector<std::shared_ptr<ParameterValue>> params;
    size_t paramOffset = 0;
    if (isAsync) {
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_task", 0));
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_ctx", 1));
    } else {
        if (!methodDecl.isStatic()) {
            params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));
            paramOffset = 1;
        }
        for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
            const auto& p = methodDecl.getParams()[i];
            params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i + paramOffset));
        }
    }

    auto func = m_module->createFunction(mangledName, returnType, params);
    m_currentFunction = func;
    // Phase 3 slice C: await inside an async method lowers like in a
    // free async fn. Restored at the end of this function.
    bool prevAsync = m_inAsyncFunction;
    m_inAsyncFunction = isAsync;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    if (isAsync) {
        // Phase 5 slice A: a nested async method (compiled lazily
        // while another async fn's body is being lowered, e.g. an
        // async method called from an async fn) must not clobber the
        // enclosing fn's state machine: asyncPrepareBody replaces
        // m_asyncSM and asyncSMEnd resets it. Save + restore around
        // this method's whole async lowering.
        auto savedAsyncSM = std::move(m_asyncSM);
        sema::TypePtr retSema = asyncResolveType(methodDecl.getReturnType());
        std::vector<sema::TypePtr> paramSema;
        for (const auto& p : methodDecl.getParams()) paramSema.push_back(asyncResolveType(p.getType()));
        sema::TypePtr thisType = nullptr;
        if (!methodDecl.isStatic()) thisType = m_sema.getTypeContext().findUserType(layout.name);
        auto& decl = const_cast<ast::FunctionDecl&>(methodDecl);
        if (asyncPrepareBody(decl, mangledName, retSema, paramSema, thisType) < 0) {
            popScope();
            m_inAsyncFunction = prevAsync;
            m_currentClass = nullptr;
            m_currentFunction = nullptr;
            m_currentBlock = nullptr;
            m_asyncSM = std::move(savedAsyncSM);
            return;
        }
        asyncSMBegin(mangledName, func);
        for (const auto& stmt : decl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
        asyncSMEnd();
        m_asyncSM = std::move(savedAsyncSM);
        popScope();
        m_inAsyncFunction = prevAsync;
        m_currentClass = nullptr;
        m_currentFunction = nullptr;
        m_currentBlock = nullptr;
        return;
    }

    if (!methodDecl.isStatic()) {
        auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
        entryBB->addInstruction(allocaThis);
        entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
        declareVariable("this", allocaThis, IRType::getPointer());
    }

    for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
        const auto& p = methodDecl.getParams()[i];
        const auto& paramVal = params[i + paramOffset];
        auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaParam);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
        declareVariable(p.getName(), allocaParam, paramVal->getType());
    }

    if (methodDecl.hasBody()) {
        for (const auto& stmt : methodDecl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (methodDecl.isShortForm()) {
        methodDecl.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_inAsyncFunction = prevAsync;
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileSpecializedConstructor(
    const ast::FunctionDecl* ctorDecl,
    const std::vector<std::string>& typeParams,
    const std::vector<std::string>& typeArgs,
    const codegen::ClassLayout& layout) {

    std::string mangledName = layout.name + "_" + layout.name;
    std::vector<std::shared_ptr<ParameterValue>> params;
    params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
            params.push_back(std::make_shared<ParameterValue>(mapType(subType), "%" + p.getName(), i + 1));
        }
    }

    auto func = m_module->createFunction(mangledName, IRType::getVoid(), params);
    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
    entryBB->addInstruction(allocaThis);
    entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
    declareVariable("this", allocaThis, IRType::getPointer());

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            const auto& paramVal = params[i + 1];
            auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
            entryBB->addInstruction(allocaParam);
            entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
            declareVariable(p.getName(), allocaParam, paramVal->getType());
        }
    }

    if (layout.hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout.vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout.structType,
            params[0],
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            func->nextTempName("vptr")
        );
        entryBB->addInstruction(gep);
        entryBB->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    if (ctorDecl && ctorDecl->hasBody()) {
        for (const auto& stmt : ctorDecl->getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    }

    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
    }

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileSpecializedMethod(
    const ast::FunctionDecl& methodDecl,
    const std::vector<std::string>& typeParams,
    const std::vector<std::string>& typeArgs,
    const codegen::ClassLayout& layout) {

    std::string mangledName = layout.name + "_" + methodDecl.getName();
    std::string subRet = sema::TypeContext::substituteType(methodDecl.getReturnType(), typeParams, typeArgs);
    // Phase 5 slice A: an `async` (specialized) method body is the
    // state-machine entry `void <name>(ptr %task, ptr %ctx)`.
    const bool isAsync = methodDecl.isAsync();
    IRTypePtr returnType = isAsync ? IRType::getVoid() : mapType(subRet);

    std::vector<std::shared_ptr<ParameterValue>> params;
    size_t paramOffset = 0;
    if (isAsync) {
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_task", 0));
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_ctx", 1));
    } else {
        if (!methodDecl.isStatic()) {
            params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));
            paramOffset = 1;
        }
        for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
            const auto& p = methodDecl.getParams()[i];
            std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
            params.push_back(std::make_shared<ParameterValue>(mapType(subType), "%" + p.getName(), i + paramOffset));
        }
    }

    auto func = m_module->createFunction(mangledName, returnType, params);
    m_currentFunction = func;
    // Phase 3 slice C: await inside an async (specialized) method
    // lowers like in a free async fn. Restored at the end.
    bool prevAsync = m_inAsyncFunction;
    m_inAsyncFunction = isAsync;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    if (isAsync) {
        sema::TypePtr retSema = asyncResolveType(subRet);
        std::vector<sema::TypePtr> paramSema;
        for (const auto& p : methodDecl.getParams()) {
            std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
            paramSema.push_back(asyncResolveType(subType));
        }
        sema::TypePtr thisType = nullptr;
        if (!methodDecl.isStatic()) thisType = m_sema.getTypeContext().findUserType(layout.name);
        auto& decl = const_cast<ast::FunctionDecl&>(methodDecl);
        // Phase 5 slice A: save/restore the enclosing async fn's state
        // machine (see compileMethod) — a generic async method
        // compiled lazily must not clobber it.
        auto savedAsyncSM = std::move(m_asyncSM);
        if (asyncPrepareBody(decl, mangledName, retSema, paramSema, thisType) < 0) {
            popScope();
            m_inAsyncFunction = prevAsync;
            m_currentClass = nullptr;
            m_currentFunction = nullptr;
            m_currentBlock = nullptr;
            m_asyncSM = std::move(savedAsyncSM);
            return;
        }
        asyncSMBegin(mangledName, func);
        for (const auto& stmt : decl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
        asyncSMEnd();
        m_asyncSM = std::move(savedAsyncSM);
        popScope();
        m_inAsyncFunction = prevAsync;
        m_currentClass = nullptr;
        m_currentFunction = nullptr;
        m_currentBlock = nullptr;
        return;
    }

    if (!methodDecl.isStatic()) {
        auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
        entryBB->addInstruction(allocaThis);
        entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
        declareVariable("this", allocaThis, IRType::getPointer());
    }

    for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
        const auto& p = methodDecl.getParams()[i];
        const auto& paramVal = params[i + paramOffset];
        auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaParam);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
        declareVariable(p.getName(), allocaParam, paramVal->getType());
    }

    if (methodDecl.hasBody()) {
        for (const auto& stmt : methodDecl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (methodDecl.isShortForm()) {
        methodDecl.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_inAsyncFunction = prevAsync;
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileSpecializedFunction(
    const ast::FunctionDecl& fnDecl,
    const std::vector<std::string>& typeArgs) {

    std::string mangledName = fnDecl.getName();
    for (const auto& arg : typeArgs) {
        mangledName += "_" + arg;
    }

    if (m_module->getFunction(mangledName)) {
        return; // Already compiled
    }

    const auto& typeParams = fnDecl.getTypeParams();
    std::string subRet = sema::TypeContext::substituteType(fnDecl.getReturnType(), typeParams, typeArgs);
    // Phase 5 slice A: an `async` generic specialization body is the
    // state-machine entry `void <name>(ptr %task, ptr %ctx)`.
    const bool isAsync = fnDecl.isAsync();
    IRTypePtr returnType = isAsync ? IRType::getVoid() : mapType(subRet);

    std::vector<std::shared_ptr<ParameterValue>> params;
    if (isAsync) {
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_task", 0));
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%__sp_ctx", 1));
    } else {
        for (size_t i = 0; i < fnDecl.getParams().size(); ++i) {
            const auto& p = fnDecl.getParams()[i];
            std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
            params.push_back(std::make_shared<ParameterValue>(mapType(subType), "%" + p.getName(), i));
        }
    }

    auto func = m_module->createFunction(mangledName, returnType, params);
    auto* prevFunc = m_currentFunction;
    auto* prevBlock = m_currentBlock;
    auto prevScopes = std::move(m_scopes);
    auto prevRefScopes = std::move(m_refCountedScopes);
    m_scopes.clear();
    m_refCountedScopes.clear();

    m_currentFunction = func;
    // Phase 3 slice C: await inside an async generic specialization
    // lowers like in a plain async fn. Restored at the end.
    bool prevAsync = m_inAsyncFunction;
    m_inAsyncFunction = isAsync;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();

    if (isAsync) {
        sema::TypePtr retSema = asyncResolveType(subRet);
        std::vector<sema::TypePtr> paramSema;
        for (const auto& p : fnDecl.getParams()) {
            std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
            paramSema.push_back(asyncResolveType(subType));
        }
        auto& decl = const_cast<ast::FunctionDecl&>(fnDecl);
        // Phase 5 slice A: save/restore the enclosing async fn's state
        // machine (see compileMethod) — a generic async fn compiled
        // lazily must not clobber it.
        auto savedAsyncSM = std::move(m_asyncSM);
        if (asyncPrepareBody(decl, mangledName, retSema, paramSema, nullptr) < 0) {
            popScope();
            m_inAsyncFunction = prevAsync;
            m_scopes = std::move(prevScopes);
            m_refCountedScopes = std::move(prevRefScopes);
            m_currentFunction = prevFunc;
            m_currentBlock = prevBlock;
            m_asyncSM = std::move(savedAsyncSM);
            return;
        }
        asyncSMBegin(mangledName, func);
        for (const auto& stmt : decl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
        asyncSMEnd();
        m_asyncSM = std::move(savedAsyncSM);
        popScope();
        m_inAsyncFunction = prevAsync;
        m_scopes = std::move(prevScopes);
        m_refCountedScopes = std::move(prevRefScopes);
        m_currentFunction = prevFunc;
        m_currentBlock = prevBlock;
        return;
    }

    for (size_t i = 0; i < fnDecl.getParams().size(); ++i) {
        const auto& p = fnDecl.getParams()[i];
        const auto& paramVal = params[i];
        auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaParam);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
        declareVariable(p.getName(), allocaParam, paramVal->getType());
    }

    if (fnDecl.hasBody()) {
        for (const auto& stmt : fnDecl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (fnDecl.isShortForm()) {
        fnDecl.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_inAsyncFunction = prevAsync;
    m_scopes = std::move(prevScopes);
    m_refCountedScopes = std::move(prevRefScopes);
    m_currentFunction = prevFunc;
    m_currentBlock = prevBlock;
}
void IRBuilder::visit(const ast::InterfaceDecl& node) { (void)node; }
void IRBuilder::visit(const ast::StructDecl& node) { (void)node; }
void IRBuilder::visit(const ast::EnumDecl& node) { (void)node; }
void IRBuilder::visit(const ast::ModuleDecl& node) { (void)node; }
void IRBuilder::visit(const ast::ImportDecl& node) { (void)node; }

// --- Expressions ---

void IRBuilder::visit(const ast::LiteralExpr& node) {
    switch (node.getLiteralKind()) {
        case ast::LiteralKind::Integer: {
            int64_t val = 0;
            std::string cleanVal = node.getValue();
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), '_'), cleanVal.end());
            try {
                if (cleanVal.rfind("0b", 0) == 0 || cleanVal.rfind("0B", 0) == 0) {
                    val = static_cast<int64_t>(std::stoull(cleanVal.substr(2), nullptr, 2));
                } else if (cleanVal.rfind("0o", 0) == 0 || cleanVal.rfind("0O", 0) == 0) {
                    val = static_cast<int64_t>(std::stoull(cleanVal.substr(2), nullptr, 8));
                } else {
                    val = std::stoll(cleanVal, nullptr, 0);
                }
            } catch (...) {
                try {
                    val = static_cast<int64_t>(std::stoull(cleanVal, nullptr, 0));
                } catch (...) {
                    val = 0;
                }
            }
            if (val > 2147483647LL || val < -2147483648LL) {
                m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt64(), val);
                m_lastExprType = IRType::getInt64();
            } else {
                m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
                m_lastExprType = IRType::getInt32();
            }
            break;
        }
        case ast::LiteralKind::Float: {
            double val = 0.0;
            std::string cleanVal = node.getValue();
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), '_'), cleanVal.end());
            bool isFloat32 = (!cleanVal.empty() && (cleanVal.back() == 'f' || cleanVal.back() == 'F'));
            if (isFloat32) {
                cleanVal.pop_back();
            }
            try {
                val = std::stod(cleanVal);
            } catch (...) {
                val = 0.0;
            }
            if (isFloat32) {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getFloat(), val);
                m_lastExprType = IRType::getFloat();
            } else {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), val);
                m_lastExprType = IRType::getDouble();
            }
            break;
        }
        case ast::LiteralKind::Boolean: {
            bool val = (node.getValue() == "true");
            m_lastExprValue = std::make_shared<ConstantBool>(val);
            m_lastExprType = IRType::getInt1();
            break;
        }
        case ast::LiteralKind::String: {
            auto strVal = m_module->getOrCreateStringLiteral(node.getValue());
            m_lastExprValue = strVal;
            m_lastExprType = IRType::getPointer(IRType::getInt8());
            break;
        }
        case ast::LiteralKind::Char: {
            char c = node.getValue().empty() ? '\0' : node.getValue()[0];
            m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt8(), static_cast<int64_t>(c));
            m_lastExprType = IRType::getInt8();
            break;
        }
        case ast::LiteralKind::Null: {
            m_lastExprValue = std::make_shared<ConstantNull>();
            m_lastExprType = IRType::getPointer();
            break;
        }
    }
}

void IRBuilder::visit(const ast::IdentifierExpr& node) {
    auto [allocaPtr, valType] = lookupVariable(node.getName());
    if (allocaPtr && valType) {
        if (valType->isStruct()) {
            m_lastExprValue = allocaPtr;
            m_lastExprType = IRType::getPointer();
            return;
        }
        std::string reg = m_currentFunction->nextTempName("v");
        auto loadInst = std::make_shared<LoadInst>(valType, allocaPtr, reg);
        m_currentBlock->addInstruction(loadInst);
        m_lastExprValue = loadInst;
        m_lastExprType = valType;
        return;
    }

    if (node.getName() == "None") {
        m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
        m_lastExprType = IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
        return;
    }

    if (node.getName() == "PI") {
        m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 3.141592653589793);
        m_lastExprType = IRType::getDouble();
        return;
    }
    if (node.getName() == "E") {
        m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 2.718281828459045);
        m_lastExprType = IRType::getDouble();
        return;
    }
    if (node.getName() == "TAU") {
        m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 6.283185307179586);
        m_lastExprType = IRType::getDouble();
        return;
    }

    if (m_currentClass && m_currentClass->fieldIndexMap.count(node.getName())) {
        auto [thisAlloca, thisType] = lookupVariable("this");
        if (thisAlloca) {
            auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
            m_currentBlock->addInstruction(thisVal);

            uint32_t fieldIndex = m_currentClass->fieldIndexMap.at(node.getName());
            size_t fVecIdx = m_currentClass->hasVTable ? (fieldIndex - 1) : fieldIndex;
            if (fVecIdx < m_currentClass->fields.size()) {
                valType = m_currentClass->fields[fVecIdx].irType;

                auto gep = std::make_shared<GetElementPtrInst>(
                    m_currentClass->structType,
                    thisVal,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), fieldIndex)
                    },
                    m_currentFunction->nextTempName("fld_ptr")
                );
                m_currentBlock->addInstruction(gep);

                auto loadInst = std::make_shared<LoadInst>(valType, gep, m_currentFunction->nextTempName("val"));
                m_currentBlock->addInstruction(loadInst);
                m_lastExprValue = loadInst;
                m_lastExprType = valType;
                return;
            }
        }
    }

    // Fallback for unresolved or global variables.
    // Phase 3 slice A: a bare (free) function name used as a value lowers
    // to the real function address (a GlobalVariable `@name`, exactly like
    // visit(LambdaExpr)), so it can be passed as a `ptr` callback to
    // extern "C" functions (queue handlers, cron tasks, ws routes, ...).
    // LLVM accepts forward references and the verifier does not check
    // bare `@name` operands, so the definition may come later in the file.
    // Still loud error[E1000] when the value cannot be a plain function
    // pointer: an overloaded bare name (ambiguous — SP has only untyped
    // `ptr`, so there is no way to pick an overload), a method (needs an
    // instance), or a generic function (no type args at a bare use).
    // Inside a lambda body, naming a variable from an enclosing scope is
    // a capture attempt — capturing lambdas are not lowered, so that
    // fails loudly too (previously it silently became 0).
    if (!m_lambdaOuterVars.empty() && m_lambdaOuterVars.back().count(node.getName())) {
        reportUnsupported(node.getLocation(), "lambda captures variable '" + node.getName() + "'",
                          "capturing lambdas are not supported; pass the value as a parameter");
    } else {
        auto functions = m_sema.getSymbolTable().lookupFunctions(node.getName());
        if (!functions.empty()) {
            bool anyMethod = false;
            bool anyGeneric = false;
            for (const auto& sym : functions) {
                if (sym && sym->isMethod()) anyMethod = true;
                if (sym && sym->isGeneric()) anyGeneric = true;
            }
            if (functions.size() > 1) {
                reportUnsupported(node.getLocation(), "function '" + node.getName() + "' used as a value",
                                  "the name is overloaded; a bare function reference is ambiguous — "
                                  "pass the function through a typed wrapper or call it instead");
            } else if (anyMethod) {
                reportUnsupported(node.getLocation(), "method '" + node.getName() + "' used as a value",
                                  "a bare method name has no instance; use a lambda or a free function as the callback");
            } else if (anyGeneric) {
                reportUnsupported(node.getLocation(), "function '" + node.getName() + "' used as a value",
                                  "a generic function cannot be used as a bare value; call it with explicit type arguments first");
            } else {
                // Exactly one non-generic free function: its address. Extern
                // "C" functions are declared by plain name; a defined function
                // by its (possibly mangled) LLVM name — a single-definition
                // function keeps its plain name.
                std::string llvmName = node.getName();
                if (const auto* decl = functions[0]->getASTDecl()) {
                    llvmName = llvmFunctionName(*decl);
                }
                m_lastExprValue = std::make_shared<GlobalVariable>(IRType::getInt8(), "@" + llvmName);
                m_lastExprType = IRType::getPointer();
                return;
            }
        }
    }
    // A bare method name inside its own class: the method needs an
    // instance, so it cannot be a plain function pointer either. (Methods
    // are not in the symbol table, so the check above cannot see them.)
    if (m_currentClass) {
        auto userType = m_sema.getTypeContext().findUserType(m_currentClass->name);
        if (userType && userType->isClass()) {
            auto clsType = std::static_pointer_cast<const sema::ClassType>(userType);
            if (!clsType->findMethods(node.getName()).empty()) {
                reportUnsupported(node.getLocation(), "method '" + node.getName() + "' used as a value",
                                  "a bare method name has no instance; use a lambda or a free function as the callback");
            }
        }
    }
    m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), 0);
    m_lastExprType = IRType::getInt32();
}

void IRBuilder::visit(const ast::AssignExpr& node) {
    if (const auto* idx = dynamic_cast<const ast::IndexExpr*>(&node.getTarget())) {
        idx->getObject().accept(*this);
        IRValuePtr objVal = m_lastExprValue;

        idx->getIndex().accept(*this);
        IRValuePtr idxVal = m_lastExprValue;

        node.getValue().accept(*this);
        IRValuePtr rhsVal = m_lastExprValue;

        if (!objVal || !idxVal || !rhsVal) {
            return;
        }

        auto semaObjType = m_sema.getNodeType(&idx->getObject());
        bool isMapType = semaObjType && (semaObjType->isMap() || (semaObjType->isClass() && semaObjType->toString().rfind("Map", 0) == 0));
        bool isListType = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("List", 0) == 0;
        bool isPtrType = semaObjType && semaObjType->isPointer();

        if (isPtrType) {
            auto semaPointee = static_cast<const sema::PointerType&>(*semaObjType).getPointeeType();
            IRTypePtr elemType = semaPointee ? mapType(semaPointee.get()) : IRType::getInt64();
            std::string gepName = m_currentFunction->nextTempName("ptr_gep");
            auto gep = std::make_shared<GetElementPtrInst>(
                elemType, objVal,
                std::vector<IRValuePtr>{idxVal},
                gepName
            );
            m_currentBlock->addInstruction(gep);
            rhsVal = emitCastIfNeeded(rhsVal, elemType);
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhsVal, gep));
            m_lastExprValue = rhsVal;
            m_lastExprType = elemType;
            return;
        }

        if (isMapType) {
            if (idxVal->getType()->isPointer()) {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_map_set_str",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            } else {
                idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_map_set_i64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            }
        } else if (isListType) {
            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
            if (rhsVal->getType()->isFloatingPoint()) {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getDouble());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_list_set_f64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            } else {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_list_set_i64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            }
        } else {
            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
            if (rhsVal->getType()->isFloatingPoint()) {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getDouble());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_array_set_f64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            } else {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_array_set_i64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            }
        }

        m_lastExprValue = rhsVal;
        return;
    }

    IRValuePtr targetPtr = nullptr;
    IRTypePtr valType = nullptr;

    if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&node.getTarget())) {
        targetPtr = emitMemberPointer(*mem, valType);
    } else if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getTarget())) {
        auto [allocaPtr, type] = lookupVariable(ident->getName());
        if (allocaPtr && type) {
            targetPtr = allocaPtr;
            valType = type;
        } else if (m_currentClass && m_currentClass->fieldIndexMap.count(ident->getName())) {
            auto [thisAlloca, thisType] = lookupVariable("this");
            if (thisAlloca) {
                auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
                m_currentBlock->addInstruction(thisVal);

                uint32_t fieldIndex = m_currentClass->fieldIndexMap.at(ident->getName());
                size_t fVecIdx = m_currentClass->hasVTable ? (fieldIndex - 1) : fieldIndex;
                if (fVecIdx < m_currentClass->fields.size()) {
                    valType = m_currentClass->fields[fVecIdx].irType;
                    auto gep = std::make_shared<GetElementPtrInst>(
                        m_currentClass->structType,
                        thisVal,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), fieldIndex)
                        },
                        m_currentFunction->nextTempName("fld_ptr")
                    );
                    m_currentBlock->addInstruction(gep);
                    targetPtr = gep;
                }
            }
        }
    } else if (const auto* un = dynamic_cast<const ast::UnaryExpr*>(&node.getTarget())) {
        if (un->getOp() == lexer::TokenType::Star) {
            un->getOperand().accept(*this);
            targetPtr = m_lastExprValue;
            auto semaType = m_sema.getNodeType(&node.getTarget());
            valType = semaType ? mapType(semaType.get()) : IRType::getInt64();
        }
    }

    if (!targetPtr || !valType) {
        return;
    }

    node.getValue().accept(*this);
    IRValuePtr rhsVal = m_lastExprValue;

    // Handle compound assignment (+=, -=, etc.)
    if (node.getOp() != lexer::TokenType::Equal) {
        std::string currentValReg = m_currentFunction->nextTempName("cur");
        auto loadInst = std::make_shared<LoadInst>(valType, targetPtr, currentValReg);
        m_currentBlock->addInstruction(loadInst);

        IROpcode op = IROpcode::Add;
        switch (node.getOp()) {
            case lexer::TokenType::PlusEqual:
                op = valType->isFloatingPoint() ? IROpcode::FAdd : IROpcode::Add;
                break;
            case lexer::TokenType::MinusEqual:
                op = valType->isFloatingPoint() ? IROpcode::FSub : IROpcode::Sub;
                break;
            case lexer::TokenType::StarEqual:
                op = valType->isFloatingPoint() ? IROpcode::FMul : IROpcode::Mul;
                break;
            case lexer::TokenType::SlashEqual:
                op = valType->isFloatingPoint() ? IROpcode::FDiv : IROpcode::SDiv;
                break;
            case lexer::TokenType::PercentEqual:
                op = valType->isFloatingPoint() ? IROpcode::FRem : IROpcode::SRem;
                break;
            default:
                op = IROpcode::Add;
                break;
        }

        rhsVal = emitCastIfNeeded(rhsVal, valType);
        auto binInst = std::make_shared<BinaryInst>(op, loadInst, rhsVal, m_currentFunction->nextTempName("comp"));
        m_currentBlock->addInstruction(binInst);
        rhsVal = binInst;
    }

    if (valType && valType->isStruct() && rhsVal && rhsVal->getType()->isPointer()) {
        auto loadInst = std::make_shared<LoadInst>(valType, rhsVal, m_currentFunction->nextTempName("tu_assign"));
        m_currentBlock->addInstruction(loadInst);
        rhsVal = loadInst;
    }
    rhsVal = emitCastIfNeeded(rhsVal, valType);

    bool isRefAssignment = false;
    if (valType && valType->isPointer()) {
        auto semaType = m_sema.getNodeType(&node.getValue());
        if (semaType && semaType->isReferenceType()) {
            isRefAssignment = true;
        } else if (dynamic_cast<const ast::NewExpr*>(&node.getValue())) {
            isRefAssignment = true;
        }
    }

    if (isRefAssignment) {
        if (!dynamic_cast<const ast::NewExpr*>(&node.getValue())) {
            auto retainCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_retain",
                std::vector<IRValuePtr>{rhsVal}
            );
            m_currentBlock->addInstruction(retainCall);
        }

        std::string oldValReg = m_currentFunction->nextTempName("old_obj");
        auto loadOld = std::make_shared<LoadInst>(IRType::getPointer(), targetPtr, oldValReg);
        m_currentBlock->addInstruction(loadOld);

        m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhsVal, targetPtr));

        auto releaseCall = std::make_shared<CallInst>(
            IRType::getVoid(),
            "sp_rt_release",
            std::vector<IRValuePtr>{loadOld}
        );
        m_currentBlock->addInstruction(releaseCall);
    } else {
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhsVal, targetPtr));
    }

    m_lastExprValue = rhsVal;
    m_lastExprType = valType;
}

void IRBuilder::visit(const ast::BinaryExpr& node) {
    node.getLeft().accept(*this);
    IRValuePtr lhs = m_lastExprValue;
    IRTypePtr lhsType = m_lastExprType;

    // Phase 1 (pro-4.0.0): && and || short-circuit. The RHS is evaluated
    // only when the LHS does not already decide the result, so RHS side
    // effects run exactly when the language semantics say they should.
    // The result is carried through a stack slot (same pattern as
    // TernaryExpr) so no Phi node is needed: Phi has no lowering in
    // this IR and would print as "unknown_inst".
    if ((node.getOp() == lexer::TokenType::AmpAmp || node.getOp() == lexer::TokenType::PipePipe) &&
        m_currentFunction && m_currentBlock && m_currentFunction->getEntryBlock() && lhs) {
        const bool isAnd = node.getOp() == lexer::TokenType::AmpAmp;
        IRValuePtr lhsBool = emitCastIfNeeded(lhs, IRType::getInt1());

        auto rhsBB = createBlock(isAnd ? "and_rhs" : "or_rhs");
        auto mergeBB = createBlock(isAnd ? "and_merge" : "or_merge");

        auto resAlloca = std::make_shared<AllocaInst>(IRType::getInt1(), m_currentFunction->nextTempName("logic_res"));
        m_currentFunction->getEntryBlock()->addInstruction(resAlloca);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(lhsBool, resAlloca));

        if (isAnd) {
            // false && _ == false: skip the RHS.
            m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(lhsBool, rhsBB->getName(), mergeBB->getName()));
        } else {
            // true || _ == true: skip the RHS.
            m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(lhsBool, mergeBB->getName(), rhsBB->getName()));
        }

        setInsertPoint(rhsBB);
        node.getRight().accept(*this);
        IRValuePtr rhs = m_lastExprValue;
        if (rhs) {
            rhs = emitCastIfNeeded(rhs, IRType::getInt1());
            // addInstruction inserts before a terminator if one is present;
            // expressions cannot terminate a block, so this is the RHS value.
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhs, resAlloca));
        }
        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
        }

        setInsertPoint(mergeBB);
        auto loadInst = std::make_shared<LoadInst>(IRType::getInt1(), resAlloca, m_currentFunction->nextTempName("logic_val"));
        mergeBB->addInstruction(loadInst);

        m_lastExprValue = loadInst;
        m_lastExprType = IRType::getInt1();
        return;
    }

    // Phase 6 slice A: null coalescing (??). Before this fix there was
    // no QuestionQuestion case at all, so the switch below fell through
    // to its default and emitted `Add` — invalid LLVM IR for pointer
    // operands (`add ptr %v0, @.str.0`). The RHS is evaluated lazily,
    // only when the LHS is null, and the result is carried through a
    // stack slot (same pattern as &&/|| and TernaryExpr — Phi has no
    // lowering in this IR).
    //
    // Null test mirrors what `== null` means for the LHS type in this
    // compiler: nullable value types use their zero value as null
    // (int? null is 0 — see the `== null` lowering), so the check is
    // `== null` for pointers, `== 0` for integers/bools/enums and
    // `oeq 0.0` for floats. A nullable STRUCT has no null
    // representation in the IR and is a loud E1000. A non-nullable LHS
    // can never be null, so ?? is a no-op there and the RHS is not
    // evaluated.
    if (node.getOp() == lexer::TokenType::QuestionQuestion &&
        m_currentFunction && m_currentBlock && m_currentFunction->getEntryBlock() && lhs) {
        auto semaNodeType = m_sema.getNodeType(&node);
        IRTypePtr resType = semaNodeType ? mapType(semaNodeType.get())
                                         : (lhsType ? lhsType : IRType::getInt32());
        auto semaLhsType = m_sema.getNodeType(&node.getLeft());
        if (!semaLhsType || !semaLhsType->isNullable()) {
            m_lastExprValue = emitCastIfNeeded(lhs, resType);
            m_lastExprType = resType;
            return;
        }
        if (!lhsType || lhsType->isStruct()) {
            reportUnsupported(node.getLocation(), "null coalescing \"??\"",
                              "the left-hand side has a struct type, which has no null value in the IR "
                              "(nullable structs are not supported as operands of \"??\")");
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }

        std::shared_ptr<CompareInst> isNull;
        if (lhsType->isPointer()) {
            isNull = std::make_shared<CompareInst>(
                IROpcode::ICmpEq, lhs, std::make_shared<ConstantNull>(lhsType),
                m_currentFunction->nextTempName("coalesce_null"));
        } else if (lhsType->isFloatingPoint()) {
            isNull = std::make_shared<CompareInst>(
                IROpcode::FCmpEq, lhs, std::make_shared<ConstantFloat>(lhsType, 0.0),
                m_currentFunction->nextTempName("coalesce_null"));
        } else {
            isNull = std::make_shared<CompareInst>(
                IROpcode::ICmpEq, lhs, std::make_shared<ConstantInt>(lhsType, 0),
                m_currentFunction->nextTempName("coalesce_null"));
        }
        m_currentBlock->addInstruction(isNull);

        auto rhsBB = createBlock("coalesce_rhs");
        auto mergeBB = createBlock("coalesce_merge");

        auto resAlloca = std::make_shared<AllocaInst>(resType, m_currentFunction->nextTempName("coalesce_res"));
        m_currentFunction->getEntryBlock()->addInstruction(resAlloca);
        // The LHS is the default result; the RHS overwrites it only
        // when the LHS is null.
        auto lhsCast = emitCastIfNeeded(lhs, resType);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(lhsCast, resAlloca));
        m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(isNull, rhsBB->getName(), mergeBB->getName()));

        setInsertPoint(rhsBB);
        node.getRight().accept(*this);
        IRValuePtr rhs = m_lastExprValue;
        if (rhs) {
            rhs = emitCastIfNeeded(rhs, resType);
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhs, resAlloca));
        }
        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
        }

        setInsertPoint(mergeBB);
        auto coalesceVal = std::make_shared<LoadInst>(resType, resAlloca, m_currentFunction->nextTempName("coalesce_val"));
        mergeBB->addInstruction(coalesceVal);

        m_lastExprValue = coalesceVal;
        m_lastExprType = resType;
        return;
    }

    node.getRight().accept(*this);
    IRValuePtr rhs = m_lastExprValue;
    IRTypePtr rhsType = m_lastExprType;

    if (!lhs || !rhs) {
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }

    // String concatenation if either operand is string/pointer
    if (node.getOp() == lexer::TokenType::Plus &&
        ((lhsType && lhsType->isPointer()) || (rhsType && rhsType->isPointer()))) {
        auto lhsStr = convertToStringValue(lhs, lhsType, &node.getLeft());
        auto rhsStr = convertToStringValue(rhs, rhsType, &node.getRight());
        std::string callReg = m_currentFunction ? m_currentFunction->nextTempName("str_cat") : "%str_cat";
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_concat",
                                               std::vector<IRValuePtr>{lhsStr, rhsStr},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        m_lastExprValue = call;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }

    // String equality comparison
    if (node.getOp() == lexer::TokenType::EqualEqual || node.getOp() == lexer::TokenType::BangEqual) {
        auto semaL = m_sema.getNodeType(&node.getLeft());
        auto semaR = m_sema.getNodeType(&node.getRight());
        bool isString = (semaL && semaL->isString()) || (semaR && semaR->isString());
        if (isString) {
            std::string resName = m_currentFunction->nextTempName("str_eq");
            auto call = std::make_shared<CallInst>(
                IRType::getInt1(), "sp_rt_str_equals",
                std::vector<IRValuePtr>{lhs, rhs},
                resName);
            m_currentBlock->addInstruction(call);
            if (node.getOp() == lexer::TokenType::BangEqual) {
                std::string notName = m_currentFunction->nextTempName("str_ne");
                auto notInst = std::make_shared<BinaryInst>(
                    IROpcode::Xor, call, std::make_shared<ConstantInt>(IRType::getInt1(), 1), notName);
                m_currentBlock->addInstruction(notInst);
                m_lastExprValue = notInst;
            } else {
                m_lastExprValue = call;
            }
            m_lastExprType = IRType::getInt1();
            return;
        }
    }

    // Logical operations on booleans stay as i1 without integer promotion.
    // NOTE (Phase 1): this eager path is now only a fallback for
    // expressions lowered without a current function/block (e.g. outside
    // any function body), where short-circuit control flow cannot be
    // built. All in-function && / || are handled by the short-circuit
    // path above and never reach here.
    if (node.getOp() == lexer::TokenType::AmpAmp || node.getOp() == lexer::TokenType::PipePipe) {
        lhs = emitCastIfNeeded(lhs, IRType::getInt1());
        rhs = emitCastIfNeeded(rhs, IRType::getInt1());
        std::string resName = m_currentFunction->nextTempName("b");
        auto op = (node.getOp() == lexer::TokenType::AmpAmp) ? IROpcode::And : IROpcode::Or;
        auto resultInst = std::make_shared<BinaryInst>(op, lhs, rhs, resName);
        if (m_currentBlock) m_currentBlock->addInstruction(resultInst);
        m_lastExprValue = resultInst;
        m_lastExprType = IRType::getInt1();
        return;
    }

    // Type promotion / widening
    bool isFloat = lhsType->isFloatingPoint() || rhsType->isFloatingPoint();
    bool isPointer = lhsType->isPointer() || rhsType->isPointer();
    IRTypePtr commonType = isFloat ? IRType::getDouble() : (isPointer ? IRType::getPointer() : (lhsType->isInt64() || rhsType->isInt64() ? IRType::getInt64() : IRType::getInt32()));

    lhs = emitCastIfNeeded(lhs, commonType);
    rhs = emitCastIfNeeded(rhs, commonType);

    std::string resName = m_currentFunction->nextTempName("b");
    IRInstructionPtr resultInst = nullptr;

    switch (node.getOp()) {
        case lexer::TokenType::Plus:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FAdd : IROpcode::Add, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Minus:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FSub : IROpcode::Sub, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Star:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FMul : IROpcode::Mul, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Slash:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FDiv : IROpcode::SDiv, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Percent:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FRem : IROpcode::SRem, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Amp:
            resultInst = std::make_shared<BinaryInst>(IROpcode::And, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Pipe:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Or, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Caret:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Xor, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::LessLess:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Shl, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::GreaterGreater:
            resultInst = std::make_shared<BinaryInst>(IROpcode::AShr, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::AmpAmp:
            resultInst = std::make_shared<BinaryInst>(IROpcode::And, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::PipePipe:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Or, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        // Comparisons
        case lexer::TokenType::EqualEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpEq : IROpcode::ICmpEq, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::BangEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpNe : IROpcode::ICmpNe, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::Less:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpLt : IROpcode::ICmpSlt, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::LessEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpLe : IROpcode::ICmpSle, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::Greater:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpGt : IROpcode::ICmpSgt, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::GreaterEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpGe : IROpcode::ICmpSge, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        default:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Add, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
    }

    m_currentBlock->addInstruction(resultInst);
    m_lastExprValue = resultInst;
}

void IRBuilder::visit(const ast::UnaryExpr& node) {
    // Handle prefix / postfix ++ and --
    if (node.getOp() == lexer::TokenType::PlusPlus || node.getOp() == lexer::TokenType::MinusMinus) {
        const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getOperand());
        if (ident) {
            auto [allocaPtr, valType] = lookupVariable(ident->getName());
            if (allocaPtr && valType) {
                std::string oldValReg = m_currentFunction->nextTempName("old");
                auto oldVal = std::make_shared<LoadInst>(valType, allocaPtr, oldValReg);
                m_currentBlock->addInstruction(oldVal);

                IROpcode op = (node.getOp() == lexer::TokenType::PlusPlus) ? IROpcode::Add : IROpcode::Sub;
                auto one = std::make_shared<ConstantInt>(valType, 1);
                std::string newValReg = m_currentFunction->nextTempName("inc");
                auto newVal = std::make_shared<BinaryInst>(op, oldVal, one, newValReg);
                m_currentBlock->addInstruction(newVal);

                m_currentBlock->addInstruction(std::make_shared<StoreInst>(newVal, allocaPtr));

                if (node.isPrefix()) {
                    m_lastExprValue = newVal;
                } else {
                    m_lastExprValue = oldVal;
                }
                m_lastExprType = valType;
                return;
            }
        }
    }

    // Raw pointer dereference *ptr
    if (node.getOp() == lexer::TokenType::Star) {
        node.getOperand().accept(*this);
        IRValuePtr ptrVal = m_lastExprValue;
        auto semaType = m_sema.getNodeType(&node);
        IRTypePtr targetType = semaType ? mapType(semaType.get()) : IRType::getInt64();
        std::string derefName = m_currentFunction->nextTempName("deref");
        auto loadInst = std::make_shared<LoadInst>(targetType, ptrVal, derefName);
        m_currentBlock->addInstruction(loadInst);
        m_lastExprValue = loadInst;
        m_lastExprType = targetType;
        return;
    }

    // Address-of &var
    if (node.getOp() == lexer::TokenType::Amp) {
        if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getOperand())) {
            auto [allocaPtr, valType] = lookupVariable(ident->getName());
            if (allocaPtr) {
                m_lastExprValue = allocaPtr;
                m_lastExprType = IRType::getPointer();
                return;
            }
        }
        if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&node.getOperand())) {
            mem->getObject().accept(*this);
            IRValuePtr objVal = m_lastExprValue;
            auto objSemaType = m_sema.getNodeType(&mem->getObject());
            std::string className = objSemaType ? objSemaType->toString() : "";
            int fieldIdx = m_vtableBuilder ? m_vtableBuilder->getFieldIndex(className, mem->getMember()) : -1;
            if (fieldIdx >= 0) {
                const auto* layout = m_vtableBuilder->getClassLayout(className);
                IRTypePtr structType = layout ? layout->structType : IRType::getPointer();
                std::string gepName = m_currentFunction->nextTempName("addr_gep");
                auto gep = std::make_shared<GetElementPtrInst>(
                    structType, objVal,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), fieldIdx)
                    },
                    gepName
                );
                m_currentBlock->addInstruction(gep);
                m_lastExprValue = gep;
                m_lastExprType = IRType::getPointer();
                return;
            }
        }
        node.getOperand().accept(*this);
        auto temp = std::make_shared<AllocaInst>(m_lastExprType, m_currentFunction->nextTempName("addr_tmp"));
        m_currentFunction->getEntryBlock()->addInstruction(temp);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(m_lastExprValue, temp));
        m_lastExprValue = temp;
        m_lastExprType = IRType::getPointer();
        return;
    }

    node.getOperand().accept(*this);
    IRValuePtr opVal = m_lastExprValue;
    IRTypePtr opType = m_lastExprType;

    std::string resName = m_currentFunction->nextTempName("u");
    IRInstructionPtr resultInst = nullptr;

    switch (node.getOp()) {
        case lexer::TokenType::Minus:
            if (opType->isFloatingPoint()) {
                resultInst = std::make_shared<BinaryInst>(IROpcode::FSub, std::make_shared<ConstantFloat>(opType, 0.0), opVal, resName);
            } else {
                resultInst = std::make_shared<BinaryInst>(IROpcode::Sub, std::make_shared<ConstantInt>(opType, 0), opVal, resName);
            }
            m_lastExprType = opType;
            break;
        case lexer::TokenType::Bang:
            resultInst = std::make_shared<CompareInst>(IROpcode::ICmpEq, opVal, std::make_shared<ConstantInt>(opType, 0), resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::Tilde:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Xor, opVal, std::make_shared<ConstantInt>(opType, -1), resName);
            m_lastExprType = opType;
            break;
        default:
            m_lastExprValue = opVal;
            return;
    }

    m_currentBlock->addInstruction(resultInst);
    m_lastExprValue = resultInst;
}

void IRBuilder::visit(const ast::CallExpr& node) {
    // 1. Check if callee is MemberExpr: obj.method(...) or super.method(...)
    if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&node.getCallee())) {
        std::string methodName = mem->getMember();

        // 1a. super.method(...)
        if (const auto* idObj = dynamic_cast<const ast::IdentifierExpr*>(&mem->getObject())) {
            if (idObj->getName() == "super") {
                if (m_currentClass && !m_currentClass->baseClass.empty()) {
                    std::string mangledName = m_currentClass->baseClass + "_" + methodName;
                    // Phase 3 slice A: an overloaded base method is mangled
                    // by signature (the overload sema resolved).
                    auto superTarget = m_sema.getCallTarget(&node);
                    if (superTarget && !superTarget->isGeneric()) {
                        auto userType = m_sema.getTypeContext().findUserType(m_currentClass->baseClass);
                        if (userType && userType->isClass()) {
                            auto clsType = std::static_pointer_cast<const sema::ClassType>(userType);
                            if (codegen::VTableBuilder::isMethodOverloaded(*clsType, methodName)) {
                                mangledName += codegen::VTableBuilder::overloadSuffix(targetParamTypes(superTarget));
                            }
                        }
                    }
                    auto [thisAlloca, thisType] = lookupVariable("this");
                    if (thisAlloca) {
                        auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
                        m_currentBlock->addInstruction(thisVal);

                        std::vector<IRValuePtr> loweredArgs;
                        loweredArgs.push_back(thisVal);
                        for (const auto& arg : node.getArgs()) {
                            arg->accept(*this);
                            loweredArgs.push_back(m_lastExprValue);
                        }

                        auto* targetFunc = m_module->getFunction(mangledName);
                        IRTypePtr returnType = targetFunc ? targetFunc->getReturnType() : IRType::getVoid();
                        std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("scall");

                        // Phase 3 slice C: `super.asyncMethod(...)` also
                        // returns a Task — spawn it, like any async call.
                        if (auto target = m_sema.getCallTarget(&node); target && target->isAsync()) {
                            std::string kind;
                            if (!taskPayloadKind(m_sema.getNodeType(&node), kind)) {
                                reportUnsupported(node.getLocation(), "call to async method",
                                                  "the return type of this 'async' method cannot be carried by a Task "
                                                  "(only void, int, bool, string, double, array and struct payloads are supported)");
                                m_lastExprValue = nullptr;
                                m_lastExprType = nullptr;
                                return;
                            }
                            // Phase 5 slice A: param types come from
                            // sema (the body's IR params are now
                            // (task, ctx)), and the body itself is
                            // the thunk — no separate thunk function.
                            auto baseType = m_sema.getTypeContext().findUserType(m_currentClass->baseClass);
                            std::vector<IRTypePtr> paramTypes = asyncSpawnParamTypes(
                                target->getFunctionType()->getParamTypes(), baseType);
                            IRValuePtr task = emitAsyncSpawnSM(
                                std::make_shared<FunctionAddress>(mangledName), loweredArgs, paramTypes);
                            if (!task) {
                                reportUnsupported(node.getLocation(), "call to async method",
                                                  "an argument of this 'async' method cannot be passed through a Task "
                                                  "(only int, bool, string and double arguments are supported)");
                                m_lastExprValue = nullptr;
                                m_lastExprType = nullptr;
                                return;
                            }
                            m_lastExprValue = task;
                            m_lastExprType = IRType::getPointer();
                            return;
                        }

                        auto callInst = std::make_shared<CallInst>(returnType, mangledName, loweredArgs, callReg);
                        m_currentBlock->addInstruction(callInst);

                        m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
                        m_lastExprType = returnType;
                        return;
                    }
                }
            }
        }

        // 1a2. Static type methods: int.parse(...), double.parse(...)
        if (const auto* idObj = dynamic_cast<const ast::IdentifierExpr*>(&mem->getObject())) {
            const std::string& targetName = idObj->getName();
            if (methodName == "parse") {
                if (targetName == "int" || targetName == "int32" || targetName == "int64") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_int") : "%parsed_int";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            "sp_rt_str_to_int",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
                if (targetName == "double") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_double") : "%parsed_double";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getDouble(),
                            "sp_rt_str_to_double",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getDouble();
                        return;
                    }
                }
                if (targetName == "float") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_float") : "%parsed_float";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getFloat(),
                            "sp_rt_str_to_float",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getFloat();
                        return;
                    }
                }
                if (targetName == "bool") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_bool") : "%parsed_bool";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt1(),
                            "sp_rt_str_to_bool",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt1();
                        return;
                    }
                }
            }

            if (targetName == "IO" || targetName == "Console") {
                if (methodName == "println") {
                    if (node.getArgs().empty()) {
                        auto emptyStr = m_module->getOrCreateStringLiteral("");
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{emptyStr});
                        m_currentBlock->addInstruction(callInst);
                    } else {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "print") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "eprintln") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    } else {
                        auto emptyStr = m_module->getOrCreateStringLiteral("");
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{emptyStr});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "eprint") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprint_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "readLine" || methodName == "readln") {
                    std::string resName = m_currentFunction ? m_currentFunction->nextTempName("readline") : "%readline";
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_read_line",
                        std::vector<IRValuePtr>{},
                        resName
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "tryReadLine") {
                    std::string resName = m_currentFunction ? m_currentFunction->nextTempName("readline") : "%readline";
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_read_line",
                        std::vector<IRValuePtr>{},
                        resName
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = emitCreateTaggedUnion("Result", 0, callInst);
                    m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
                if (methodName == "flush") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_flush", std::vector<IRValuePtr>{});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "eflush") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eflush", std::vector<IRValuePtr>{});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (targetName == "String") {
                if (methodName == "upper" || methodName == "lower" || methodName == "trim" || methodName == "reverse") {
                    if (!checkStringArgs(node, {0}, "String." + methodName)) {
                        m_lastExprValue = nullptr;
                        m_lastExprType = nullptr;
                        return;
                    }
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        std::string fnName = (methodName == "upper") ? "sp_rt_str_upper" :
                                             (methodName == "lower") ? "sp_rt_str_lower" :
                                             (methodName == "trim") ? "sp_rt_str_trim" : "sp_rt_str_reverse";
                        std::string resReg = m_currentFunction->nextTempName("str_res");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(IRType::getInt8()),
                            fnName,
                            std::vector<IRValuePtr>{m_lastExprValue},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer(IRType::getInt8());
                        return;
                    }
                }
                if (methodName == "length") {
                    if (!checkStringArgs(node, {0}, "String.length")) {
                        m_lastExprValue = nullptr;
                        m_lastExprType = nullptr;
                        return;
                    }
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        std::string resReg = m_currentFunction->nextTempName("str_len");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            "sp_rt_str_len",
                            std::vector<IRValuePtr>{m_lastExprValue},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
                if (methodName == "contains") {
                    if (!checkStringArgs(node, {0, 1}, "String.contains")) {
                        m_lastExprValue = nullptr;
                        m_lastExprType = nullptr;
                        return;
                    }
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto subVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_has");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt1(),
                            "sp_rt_str_contains",
                            std::vector<IRValuePtr>{sVal, subVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt1();
                        return;
                    }
                }
                if (methodName == "indexOf") {
                    if (!checkStringArgs(node, {0, 1}, "String.indexOf")) {
                        m_lastExprValue = nullptr;
                        m_lastExprType = nullptr;
                        return;
                    }
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto subVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_idx");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            "sp_rt_str_index_of",
                            std::vector<IRValuePtr>{sVal, subVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
                if (methodName == "replace") {
                    if (!checkStringArgs(node, {0, 1, 2}, "String.replace")) {
                        m_lastExprValue = nullptr;
                        m_lastExprType = nullptr;
                        return;
                    }
                    if (node.getArgs().size() >= 3) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto fromVal = m_lastExprValue;
                        node.getArgs()[2]->accept(*this);
                        auto toVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_rep");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(IRType::getInt8()),
                            "sp_rt_str_replace",
                            std::vector<IRValuePtr>{sVal, fromVal, toVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer(IRType::getInt8());
                        return;
                    }
                }
                if (methodName == "substring") {
                    if (!checkStringArgs(node, {0}, "String.substring")) {
                        m_lastExprValue = nullptr;
                        m_lastExprType = nullptr;
                        return;
                    }
                    if (node.getArgs().size() >= 3) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto startVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        node.getArgs()[2]->accept(*this);
                        auto endVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        std::string resReg = m_currentFunction->nextTempName("str_sub");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(IRType::getInt8()),
                            "sp_rt_str_substring",
                            std::vector<IRValuePtr>{sVal, startVal, endVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer(IRType::getInt8());
                        return;
                    }
                }
                if (methodName == "split") {
                    if (!checkStringArgs(node, {0, 1}, "String.split")) {
                        m_lastExprValue = nullptr;
                        m_lastExprType = nullptr;
                        return;
                    }
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto delimVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_split");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(),
                            "sp_rt_str_split",
                            std::vector<IRValuePtr>{sVal, delimVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer();
                        return;
                    }
                }
            }

            if (targetName == "Math") {
                if (methodName == "sqrt" || methodName == "sin" || methodName == "cos" || methodName == "tan" ||
                    methodName == "asin" || methodName == "acos" || methodName == "atan" ||
                    methodName == "floor" || methodName == "ceil" || methodName == "round" ||
                    methodName == "log" || methodName == "log10" || methodName == "log2" || methodName == "exp") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto val = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
                        std::string fn = "sp_rt_math_" + methodName;
                        std::string reg = m_currentFunction->nextTempName("math_res");
                        auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{val}, reg);
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getDouble();
                        return;
                    }
                }
                if (methodName == "pow" || methodName == "atan2") {
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto v1 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
                        node.getArgs()[1]->accept(*this);
                        auto v2 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
                        std::string fn = "sp_rt_math_" + methodName;
                        std::string reg = m_currentFunction->nextTempName("math_res");
                        auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getDouble();
                        return;
                    }
                }
                if (methodName == "abs") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto val = m_lastExprValue;
                        auto type = m_lastExprType;
                        auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
                        bool isFlt = (type && type->isFloatingPoint()) || (semaType && (semaType->isFloat() || semaType->isDouble()));
                        if (isFlt) {
                            val = emitCastIfNeeded(val, IRType::getDouble());
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), "sp_rt_math_abs_double", std::vector<IRValuePtr>{val}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getDouble();
                        } else {
                            val = emitCastIfNeeded(val, IRType::getInt64());
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_math_abs_int", std::vector<IRValuePtr>{val}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        }
                        return;
                    }
                }
                if (methodName == "min" || methodName == "max") {
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto v1 = m_lastExprValue;
                        auto t1 = m_lastExprType;
                        auto semaT1 = m_sema.getNodeType(node.getArgs()[0].get());
                        node.getArgs()[1]->accept(*this);
                        auto v2 = m_lastExprValue;
                        auto t2 = m_lastExprType;
                        auto semaT2 = m_sema.getNodeType(node.getArgs()[1].get());
                        bool isFlt = (t1 && t1->isFloatingPoint()) || (t2 && t2->isFloatingPoint()) ||
                                     (semaT1 && (semaT1->isFloat() || semaT1->isDouble())) ||
                                     (semaT2 && (semaT2->isFloat() || semaT2->isDouble()));
                        if (isFlt) {
                            v1 = emitCastIfNeeded(v1, IRType::getDouble());
                            v2 = emitCastIfNeeded(v2, IRType::getDouble());
                            std::string fn = (methodName == "min") ? "sp_rt_math_min_double" : "sp_rt_math_max_double";
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getDouble();
                        } else {
                            v1 = emitCastIfNeeded(v1, IRType::getInt64());
                            v2 = emitCastIfNeeded(v2, IRType::getInt64());
                            std::string fn = (methodName == "min") ? "sp_rt_math_min_int" : "sp_rt_math_max_int";
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getInt64(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        }
                        return;
                    }
                }
            }

            if (targetName == "FS" || targetName == "File") {
                if (methodName == "readText" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_fs_read_text", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "tryReadText" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string existsReg = m_currentFunction->nextTempName("fs_exists");
                    auto existsCall = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_exists", std::vector<IRValuePtr>{p}, existsReg);
                    m_currentBlock->addInstruction(existsCall);

                    auto thenBB = m_currentFunction->createBasicBlock("try_read_ok");
                    auto elseBB = m_currentFunction->createBasicBlock("try_read_err");
                    auto mergeBB = m_currentFunction->createBasicBlock("try_read_end");

                    auto branchInst = std::make_shared<CondBranchInst>(existsCall, thenBB->getName(), elseBB->getName());
                    m_currentBlock->addInstruction(branchInst);

                    auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_read_slot"));
                    m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

                    // Success block
                    setInsertPoint(thenBB);
                    std::string readReg = m_currentFunction->nextTempName("fs_res");
                    auto readCall = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_fs_read_text", std::vector<IRValuePtr>{p}, readReg);
                    thenBB->addInstruction(readCall);
                    emitPopulateTaggedUnion(resultSlot, "Result", 0, readCall);
                    thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Err block (IOError.NotFound = 0)
                    setInsertPoint(elseBB);
                    auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
                    emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
                    elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Merge block
                    setInsertPoint(mergeBB);
                    m_lastExprValue = resultSlot;
                    m_lastExprType = structType;
                    return;
                }
                if ((methodName == "tryWriteText" || methodName == "tryAppendText") && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto c = m_lastExprValue;
                    std::string fn = (methodName == "tryWriteText") ? "sp_rt_fs_write_text" : "sp_rt_fs_append_text";
                    std::string writeReg = m_currentFunction->nextTempName("fs_res");
                    auto writeCall = std::make_shared<CallInst>(IRType::getInt1(), fn, std::vector<IRValuePtr>{p, c}, writeReg);
                    m_currentBlock->addInstruction(writeCall);

                    auto thenBB = m_currentFunction->createBasicBlock("try_write_ok");
                    auto elseBB = m_currentFunction->createBasicBlock("try_write_err");
                    auto mergeBB = m_currentFunction->createBasicBlock("try_write_end");

                    auto branchInst = std::make_shared<CondBranchInst>(writeCall, thenBB->getName(), elseBB->getName());
                    m_currentBlock->addInstruction(branchInst);

                    auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_write_slot"));
                    m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

                    // Success block
                    setInsertPoint(thenBB);
                    auto trueVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
                    emitPopulateTaggedUnion(resultSlot, "Result", 0, trueVal);
                    thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Err block (IOError.PermissionDenied = 1)
                    setInsertPoint(elseBB);
                    auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
                    emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
                    elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Merge block
                    setInsertPoint(mergeBB);
                    m_lastExprValue = resultSlot;
                    m_lastExprType = structType;
                    return;
                }
                if ((methodName == "writeText" || methodName == "appendText") && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto c = m_lastExprValue;
                    std::string fn = (methodName == "writeText") ? "sp_rt_fs_write_text" : "sp_rt_fs_append_text";
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), fn, std::vector<IRValuePtr>{p, c}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "exists" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_exists", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if ((methodName == "delete" || methodName == "deleteFile") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_delete", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if ((methodName == "tryDelete" || methodName == "tryDeleteFile") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string delReg = m_currentFunction->nextTempName("fs_res");
                    auto delCall = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_delete", std::vector<IRValuePtr>{p}, delReg);
                    m_currentBlock->addInstruction(delCall);

                    auto thenBB = m_currentFunction->createBasicBlock("try_del_ok");
                    auto elseBB = m_currentFunction->createBasicBlock("try_del_err");
                    auto mergeBB = m_currentFunction->createBasicBlock("try_del_end");

                    auto branchInst = std::make_shared<CondBranchInst>(delCall, thenBB->getName(), elseBB->getName());
                    m_currentBlock->addInstruction(branchInst);

                    auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_del_slot"));
                    m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

                    // Success block
                    setInsertPoint(thenBB);
                    auto trueVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
                    emitPopulateTaggedUnion(resultSlot, "Result", 0, trueVal);
                    thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Err block (IOError.NotFound = 0)
                    setInsertPoint(elseBB);
                    auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
                    emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
                    elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Merge block
                    setInsertPoint(mergeBB);
                    m_lastExprValue = resultSlot;
                    m_lastExprType = structType;
                    return;
                }
                if (methodName == "createDir" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_create_dir", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "tryCreateDir" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string mkdirReg = m_currentFunction->nextTempName("fs_res");
                    auto mkdirCall = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_create_dir", std::vector<IRValuePtr>{p}, mkdirReg);
                    m_currentBlock->addInstruction(mkdirCall);

                    auto thenBB = m_currentFunction->createBasicBlock("try_mkdir_ok");
                    auto elseBB = m_currentFunction->createBasicBlock("try_mkdir_err");
                    auto mergeBB = m_currentFunction->createBasicBlock("try_mkdir_end");

                    auto branchInst = std::make_shared<CondBranchInst>(mkdirCall, thenBB->getName(), elseBB->getName());
                    m_currentBlock->addInstruction(branchInst);

                    auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_mkdir_slot"));
                    m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

                    // Success block
                    setInsertPoint(thenBB);
                    auto trueVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
                    emitPopulateTaggedUnion(resultSlot, "Result", 0, trueVal);
                    thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Err block (IOError.PermissionDenied = 1)
                    setInsertPoint(elseBB);
                    auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
                    emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
                    elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    // Merge block
                    setInsertPoint(mergeBB);
                    m_lastExprValue = resultSlot;
                    m_lastExprType = structType;
                    return;
                }
                if (methodName == "listDir" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_fs_list_dir", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
                if (methodName == "isFile" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_file", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "isDir" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_dir", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }

            if (targetName == "Time") {
                if (methodName == "now") {
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ms", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "nowSeconds") {
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_secs", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "nowNanos") {
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ns", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if ((methodName == "sleep" || methodName == "sleepSeconds") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string fn = (methodName == "sleep") ? "sp_rt_time_sleep_ms" : "sp_rt_time_sleep_secs";
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), fn, std::vector<IRValuePtr>{ms});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "format" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_time_format_iso", std::vector<IRValuePtr>{ms}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "parse" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto s = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_parse_iso", std::vector<IRValuePtr>{s}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }

            if (targetName == "Duration") {
                if ((methodName == "fromMillis" || methodName == "fromSeconds" ||
                     methodName == "fromMinutes" || methodName == "fromHours") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string fn = "sp_rt_duration_from_millis";
                    if (methodName == "fromSeconds") fn = "sp_rt_duration_from_seconds";
                    else if (methodName == "fromMinutes") fn = "sp_rt_duration_from_minutes";
                    else if (methodName == "fromHours") fn = "sp_rt_duration_from_hours";
                    std::string reg = m_currentFunction->nextTempName("dur");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(), fn, std::vector<IRValuePtr>{val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
            }

            if (targetName == "OS") {
                if (methodName == "getEnv" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto n = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_get_env", std::vector<IRValuePtr>{n}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "setEnv" && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto n = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto v = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_os_set_env", std::vector<IRValuePtr>{n, v}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "platform") {
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_platform", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "arch") {
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_arch", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "args") {
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_os_args", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
                if (methodName == "exit" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto code = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_os_exit", std::vector<IRValuePtr>{code});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (targetName == "Process") {
                if (methodName == "exec" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto cmd = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_exec", std::vector<IRValuePtr>{cmd}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "spawn" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto cmd = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_spawn", std::vector<IRValuePtr>{cmd}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if ((methodName == "run" || methodName == "execCapture") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto cmd = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_process_run", std::vector<IRValuePtr>{cmd}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "pid") {
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_pid", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }

            if (targetName == "Thread") {
                if (methodName == "id") {
                    std::string reg = m_currentFunction->nextTempName("tid");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_thread_id", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "hardwareConcurrency") {
                    std::string reg = m_currentFunction->nextTempName("hwc");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_thread_hardware_concurrency", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "yield") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_thread_yield", std::vector<IRValuePtr>{});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "sleep" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_thread_sleep_ms", std::vector<IRValuePtr>{ms});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "spawn" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto fnPtr = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    IRValuePtr argVal = nullptr;
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[1]->accept(*this);
                        argVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    } else {
                        argVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
                    }
                    std::string reg = m_currentFunction->nextTempName("th_handle");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_thread_spawn", std::vector<IRValuePtr>{fnPtr, argVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }

            if (targetName == "Result" || targetName.rfind("Result<", 0) == 0) {
                if (methodName == "Ok" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    m_lastExprValue = emitCreateTaggedUnion("Result", 0, m_lastExprValue);
                    m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
                if (methodName == "Err" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    m_lastExprValue = emitCreateTaggedUnion("Result", 1, m_lastExprValue);
                    m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            }
            if (targetName == "Option" || targetName.rfind("Option<", 0) == 0) {
                if (methodName == "Some" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    m_lastExprValue = emitCreateTaggedUnion("Option", 0, m_lastExprValue);
                    m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
                if (methodName == "None") {
                    m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
                    m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            }

            auto enumUserType = m_sema.getTypeContext().findUserType(targetName);
            if (enumUserType && enumUserType->isEnum()) {
                auto en = std::static_pointer_cast<sema::EnumType>(enumUserType);
                if (en && en->isTaggedUnion() && en->hasVariant(methodName)) {
                    int64_t variantVal = en->getVariantValue(methodName);
                    IRValuePtr payloadVal = nullptr;
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        payloadVal = m_lastExprValue;
                    }
                    m_lastExprValue = emitCreateTaggedUnion(targetName, static_cast<int32_t>(variantVal), payloadVal);
                    m_lastExprType = ir::IRType::getStruct("struct." + targetName, {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            }
        }

        // 1b. obj.method(...)
        mem->getObject().accept(*this);
        IRValuePtr objVal = m_lastExprValue;
        if (!objVal) {
            m_lastExprValue = nullptr;
            m_lastExprType = IRType::getVoid();
            return;
        }

        auto semaObjType = m_sema.getNodeType(&mem->getObject());
        std::string objTypeName = semaObjType ? semaObjType->toString() : "";
        bool isTaggedUnionObj = false;
        std::string tuBaseName;
        if (objTypeName == "Result" || objTypeName.rfind("Result<", 0) == 0) {
            isTaggedUnionObj = true;
            tuBaseName = "Result";
        } else if (objTypeName == "Option" || objTypeName.rfind("Option<", 0) == 0) {
            isTaggedUnionObj = true;
            tuBaseName = "Option";
        } else if (semaObjType && semaObjType->isEnum()) {
            auto en = std::static_pointer_cast<sema::EnumType>(semaObjType);
            if (en && en->isTaggedUnion()) {
                isTaggedUnionObj = true;
                tuBaseName = en->getName();
            }
        }

        // Phase 5 slice A: Task instance methods (cancel / isReady /
        // wasCancelled) lower directly to the async runtime C ABI —
        // the Task class has no IR methods of its own. The task
        // handle is the object value (a pointer).
        if (objTypeName == "Task" || objTypeName.rfind("Task<", 0) == 0) {
            if (methodName == "cancel" || methodName == "isReady" || methodName == "wasCancelled") {
                std::string rtFn = methodName == "cancel" ? "sp_rt_async_task_cancel"
                                 : methodName == "isReady" ? "sp_rt_async_task_is_ready"
                                 : "sp_rt_async_task_was_cancelled";
                auto callInst = std::make_shared<CallInst>(IRType::getInt1(), rtFn,
                    std::vector<IRValuePtr>{objVal}, m_currentFunction->nextTempName("task_flag"));
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt1();
                return;
            }
        }

        if (isTaggedUnionObj) {
            IRValuePtr objPtr = objVal;
            auto structType = ir::IRType::getStruct("struct." + tuBaseName, {IRType::getInt32(), IRType::getInt64()});
            if (objVal->getType()->isStruct()) {
                auto tempAlloca = std::make_shared<AllocaInst>(objVal->getType(), m_currentFunction->nextTempName("tu_tmp"));
                m_currentFunction->getEntryBlock()->addInstruction(tempAlloca);
                m_currentBlock->addInstruction(std::make_shared<StoreInst>(objVal, tempAlloca));
                objPtr = tempAlloca;
            }

            if (methodName == "isOk" || methodName == "isSome") {
                auto gepTag = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                    },
                    m_currentFunction->nextTempName("tag_ptr")
                );
                m_currentBlock->addInstruction(gepTag);
                auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("tag"));
                m_currentBlock->addInstruction(loadTag);

                auto cmp = std::make_shared<CompareInst>(IROpcode::ICmpEq, loadTag,
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    m_currentFunction->nextTempName("is_ok"));
                m_currentBlock->addInstruction(cmp);
                m_lastExprValue = cmp;
                m_lastExprType = IRType::getInt1();
                return;
            }

            if (methodName == "isErr" || methodName == "isNone") {
                auto gepTag = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                    },
                    m_currentFunction->nextTempName("tag_ptr")
                );
                m_currentBlock->addInstruction(gepTag);
                auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("tag"));
                m_currentBlock->addInstruction(loadTag);

                auto cmp = std::make_shared<CompareInst>(IROpcode::ICmpNe, loadTag,
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    m_currentFunction->nextTempName("is_err"));
                m_currentBlock->addInstruction(cmp);
                m_lastExprValue = cmp;
                m_lastExprType = IRType::getInt1();
                return;
            }

            if (methodName == "unwrap") {
                auto gepPayload = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 1)
                    },
                    m_currentFunction->nextTempName("payload_ptr")
                );
                m_currentBlock->addInstruction(gepPayload);
                auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("payload"));
                m_currentBlock->addInstruction(loadPayload);

                auto callSemaType = m_sema.getNodeType(&node);
                IRTypePtr targetType = callSemaType ? mapType(callSemaType.get()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(loadPayload, targetType);
                m_lastExprType = targetType;
                return;
            }

            if (methodName == "unwrapErr") {
                auto gepPayload = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 1)
                    },
                    m_currentFunction->nextTempName("payload_ptr")
                );
                m_currentBlock->addInstruction(gepPayload);
                auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("payload"));
                m_currentBlock->addInstruction(loadPayload);

                auto callSemaType = m_sema.getNodeType(&node);
                IRTypePtr targetType = callSemaType ? mapType(callSemaType.get()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(loadPayload, targetType);
                m_lastExprType = targetType;
                return;
            }

            if (methodName == "unwrapOr") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    IRValuePtr defVal = m_lastExprValue;

                    auto gepTag = std::make_shared<GetElementPtrInst>(
                        structType,
                        objPtr,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                        },
                        m_currentFunction->nextTempName("tag_ptr")
                    );
                    m_currentBlock->addInstruction(gepTag);
                    auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("tag"));
                    m_currentBlock->addInstruction(loadTag);

                    auto gepPayload = std::make_shared<GetElementPtrInst>(
                        structType,
                        objPtr,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), 1)
                        },
                        m_currentFunction->nextTempName("payload_ptr")
                    );
                    m_currentBlock->addInstruction(gepPayload);
                    auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("payload"));
                    m_currentBlock->addInstruction(loadPayload);

                    auto callSemaType = m_sema.getNodeType(&node);
                    IRTypePtr targetType = callSemaType ? mapType(callSemaType.get()) : (defVal ? defVal->getType() : IRType::getInt64());
                    auto unwrapped = emitCastIfNeeded(loadPayload, targetType);
                    defVal = emitCastIfNeeded(defVal, targetType);

                    auto okBB = createBlock("unwrap_ok");
                    auto defBB = createBlock("unwrap_def");
                    auto mergeBB = createBlock("unwrap_merge");
                    auto resAlloca = std::make_shared<AllocaInst>(targetType, m_currentFunction->nextTempName("unwrapped_res"));
                    m_currentFunction->getEntryBlock()->addInstruction(resAlloca);

                    auto cmp = std::make_shared<CompareInst>(IROpcode::ICmpEq, loadTag,
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        m_currentFunction->nextTempName("is_ok"));
                    m_currentBlock->addInstruction(cmp);
                    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(cmp, okBB->getName(), defBB->getName()));

                    setInsertPoint(okBB);
                    okBB->addInstruction(std::make_shared<StoreInst>(unwrapped, resAlloca));
                    okBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    setInsertPoint(defBB);
                    defBB->addInstruction(std::make_shared<StoreInst>(defVal, resAlloca));
                    defBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    setInsertPoint(mergeBB);
                    auto loadRes = std::make_shared<LoadInst>(targetType, resAlloca, m_currentFunction->nextTempName("unwrapped_val"));
                    mergeBB->addInstruction(loadRes);
                    m_lastExprValue = loadRes;
                    m_lastExprType = targetType;
                    return;
                }
            }
        }
        if (semaObjType && semaObjType->isClass() && semaObjType->toString() == "Duration") {
            if (methodName == "toMillis") {
                std::string reg = m_currentFunction->nextTempName("dur_ms");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_duration_to_millis",
                    std::vector<IRValuePtr>{objVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "toSeconds") {
                std::string reg = m_currentFunction->nextTempName("dur_s");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_duration_to_seconds",
                    std::vector<IRValuePtr>{objVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
        }

        if (semaObjType && semaObjType->isClass() && semaObjType->toString() == "Arena") {
            if (methodName == "alloc" && !node.getArgs().empty()) {
                node.getArgs()[0]->accept(*this);
                auto szVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                auto alignVal = std::make_shared<ConstantInt>(IRType::getInt64(), 8);
                std::string reg = m_currentFunction->nextTempName("arena_ptr");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_arena_alloc",
                    std::vector<IRValuePtr>{objVal, szVal, alignVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "allocAligned" && node.getArgs().size() >= 2) {
                node.getArgs()[0]->accept(*this);
                auto szVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                node.getArgs()[1]->accept(*this);
                auto alignVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                std::string reg = m_currentFunction->nextTempName("arena_ptr");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_arena_alloc",
                    std::vector<IRValuePtr>{objVal, szVal, alignVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "reset") {
                auto callInst = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_arena_reset",
                    std::vector<IRValuePtr>{objVal}
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = nullptr;
                m_lastExprType = IRType::getVoid();
                return;
            }
            if (methodName == "destroy") {
                auto callInst = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_arena_destroy",
                    std::vector<IRValuePtr>{objVal}
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = nullptr;
                m_lastExprType = IRType::getVoid();
                return;
            }
            if (methodName == "allocatedBytes") {
                std::string reg = m_currentFunction->nextTempName("arena_bytes");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_arena_allocated_bytes",
                    std::vector<IRValuePtr>{objVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
        }

        if (semaObjType && semaObjType->isClass()) {
            std::string clsName = semaObjType->toString();

            if (clsName == "Thread") {
                if (methodName == "join") {
                    auto hVal = emitCastIfNeeded(objVal, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_thread_join", std::vector<IRValuePtr>{hVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "detach") {
                    auto hVal = emitCastIfNeeded(objVal, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_thread_detach", std::vector<IRValuePtr>{hVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "Mutex") {
                if (methodName == "lock") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_mutex_lock", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "tryLock") {
                    std::string reg = m_currentFunction->nextTempName("mtx_try");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_mutex_try_lock", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "unlock") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_mutex_unlock", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_mutex_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "RwLock") {
                if (methodName == "readLock") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_rwlock_read_lock", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "tryReadLock") {
                    std::string reg = m_currentFunction->nextTempName("rw_try");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_rwlock_try_read_lock", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "readUnlock") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_rwlock_read_unlock", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "writeLock") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_rwlock_write_lock", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "tryWriteLock") {
                    std::string reg = m_currentFunction->nextTempName("rw_try");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_rwlock_try_write_lock", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "writeUnlock") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_rwlock_write_unlock", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_rwlock_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "Semaphore") {
                if (methodName == "acquire") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_semaphore_acquire", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "tryAcquire") {
                    std::string reg = m_currentFunction->nextTempName("sem_try");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_semaphore_try_acquire", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "release") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_semaphore_release", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_semaphore_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "ConditionVariable") {
                if (methodName == "wait" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto mtxVal = m_lastExprValue;
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_condvar_wait", std::vector<IRValuePtr>{objVal, mtxVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "waitTimeout" && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto mtxVal = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto msVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("cv_timeout");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_condvar_wait_timeout", std::vector<IRValuePtr>{objVal, mtxVal, msVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "notifyOne") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_condvar_notify_one", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "notifyAll") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_condvar_notify_all", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_condvar_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "AtomicInt") {
                if (methodName == "get" || methodName == "load") {
                    std::string reg = m_currentFunction->nextTempName("atom_val");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_atomic_int_load", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if ((methodName == "set" || methodName == "store") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_atomic_int_store", std::vector<IRValuePtr>{objVal, val});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "add" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("atom_add");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_atomic_int_add", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "sub" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("atom_sub");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_atomic_int_sub", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "increment") {
                    auto val = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
                    std::string reg = m_currentFunction->nextTempName("atom_inc");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_atomic_int_add", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "decrement") {
                    auto val = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
                    std::string reg = m_currentFunction->nextTempName("atom_dec");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_atomic_int_sub", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "exchange" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("atom_xchg");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_atomic_int_exchange", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "compareAndSwap" && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto expVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    node.getArgs()[1]->accept(*this);
                    auto desVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("atom_cas");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_atomic_int_cas", std::vector<IRValuePtr>{objVal, expVal, desVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_atomic_int_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "AtomicBool") {
                if (methodName == "get" || methodName == "load") {
                    std::string reg = m_currentFunction->nextTempName("atom_bval");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_atomic_bool_load", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if ((methodName == "set" || methodName == "store") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt1());
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_atomic_bool_store", std::vector<IRValuePtr>{objVal, val});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "exchange" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt1());
                    std::string reg = m_currentFunction->nextTempName("atom_bxchg");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_atomic_bool_exchange", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "compareAndSwap" && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto expVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt1());
                    node.getArgs()[1]->accept(*this);
                    auto desVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt1());
                    std::string reg = m_currentFunction->nextTempName("atom_bcas");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_atomic_bool_cas", std::vector<IRValuePtr>{objVal, expVal, desVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_atomic_bool_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "Channel" || clsName.rfind("Channel", 0) == 0) {
                if (methodName == "send" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("chan_send");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_channel_send_i64", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "recv") {
                    std::string reg = m_currentFunction->nextTempName("chan_recv");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_channel_recv_i64", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "trySend" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("chan_tsend");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_channel_try_send_i64", std::vector<IRValuePtr>{objVal, val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "close") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_channel_close", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "isClosed") {
                    std::string reg = m_currentFunction->nextTempName("chan_closed");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_channel_is_closed", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "size") {
                    std::string reg = m_currentFunction->nextTempName("chan_sz");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_channel_size", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "capacity") {
                    std::string reg = m_currentFunction->nextTempName("chan_cap");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_channel_capacity", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_channel_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (clsName == "ThreadPool") {
                if (methodName == "submit" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    IRValuePtr argVal = nullptr;
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[1]->accept(*this);
                        argVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    } else {
                        argVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
                    }
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_threadpool_submit", std::vector<IRValuePtr>{objVal, fnVal, argVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "waitIdle") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_threadpool_wait_idle", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "threadCount") {
                    std::string reg = m_currentFunction->nextTempName("tpool_cnt");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_threadpool_thread_count", std::vector<IRValuePtr>{objVal}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "destroy") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_threadpool_destroy", std::vector<IRValuePtr>{objVal});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }
        }

        if (semaObjType && semaObjType->isString()) {
            if (methodName == "length" || methodName == "size") {
                std::string lenReg = m_currentFunction->nextTempName("slen");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_str_len",
                    std::vector<IRValuePtr>{objVal},
                    lenReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "upper") {
                std::string upReg = m_currentFunction->nextTempName("supper");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_upper",
                    std::vector<IRValuePtr>{objVal},
                    upReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "lower") {
                std::string lowReg = m_currentFunction->nextTempName("slower");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_lower",
                    std::vector<IRValuePtr>{objVal},
                    lowReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "trim") {
                std::string trimReg = m_currentFunction->nextTempName("strim");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_trim",
                    std::vector<IRValuePtr>{objVal},
                    trimReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "contains") {
                if (!checkStringArgs(node, {0}, "String.contains")) {
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string contReg = m_currentFunction->nextTempName("scont");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt1(),
                        "sp_rt_str_contains",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        contReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }
            if (methodName == "startsWith") {
                if (!checkStringArgs(node, {0}, "String.startsWith")) {
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string swReg = m_currentFunction->nextTempName("ssw");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt1(),
                        "sp_rt_str_starts_with",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        swReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }
            if (methodName == "endsWith") {
                if (!checkStringArgs(node, {0}, "String.endsWith")) {
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string ewReg = m_currentFunction->nextTempName("sew");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt1(),
                        "sp_rt_str_ends_with",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        ewReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }
            if (methodName == "indexOf") {
                if (!checkStringArgs(node, {0}, "String.indexOf")) {
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string idxReg = m_currentFunction->nextTempName("sidx");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt64(),
                        "sp_rt_str_index_of",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        idxReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }
            if (methodName == "replace") {
                if (!checkStringArgs(node, {0, 1}, "String.replace")) {
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                if (node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto fromVal = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto toVal = m_lastExprValue;
                    std::string repReg = m_currentFunction->nextTempName("srep");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_str_replace",
                        std::vector<IRValuePtr>{objVal, fromVal, toVal},
                        repReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
            }
            if (methodName == "substring") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto startVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    IRValuePtr endVal = nullptr;
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[1]->accept(*this);
                        endVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    } else {
                        endVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1000000000LL);
                    }
                    std::string subReg = m_currentFunction->nextTempName("ssub");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_str_substring",
                        std::vector<IRValuePtr>{objVal, startVal, endVal},
                        subReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
            }
            if (methodName == "charAt") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string charReg = m_currentFunction->nextTempName("schar");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt8(),
                        "sp_rt_str_char_at",
                        std::vector<IRValuePtr>{objVal, idxVal},
                        charReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt8();
                    return;
                }
            }
            if (methodName == "split") {
                if (!checkStringArgs(node, {0}, "String.split")) {
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                IRValuePtr delimVal = nullptr;
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    delimVal = m_lastExprValue;
                } else {
                    delimVal = m_module->getOrCreateStringLiteral("");
                }
                std::string splitReg = m_currentFunction->nextTempName("ssplit");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(),
                    "sp_rt_str_split",
                    std::vector<IRValuePtr>{objVal, delimVal},
                    splitReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer();
                return;
            }
            if (methodName == "reverse") {
                std::string revReg = m_currentFunction->nextTempName("srev");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_reverse",
                    std::vector<IRValuePtr>{objVal},
                    revReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "repeat") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto countVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string repReg = m_currentFunction->nextTempName("srepeat");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_str_repeat",
                        std::vector<IRValuePtr>{objVal, countVal},
                        repReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
            }
            if (methodName == "isEmpty") {
                std::string emptyReg = m_currentFunction->nextTempName("sempty");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt1(),
                    "sp_rt_str_is_empty",
                    std::vector<IRValuePtr>{objVal},
                    emptyReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt1();
                return;
            }
        }

        bool isList = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("List", 0) == 0;
        bool isMap = semaObjType && (semaObjType->isMap() || (semaObjType->isClass() && semaObjType->toString().rfind("Map", 0) == 0));
        bool isSet = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("Set", 0) == 0;
        bool isStack = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("Stack", 0) == 0;
        bool isQueue = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("Queue", 0) == 0;
        bool isPQ = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("PriorityQueue", 0) == 0;

        if (semaObjType && (semaObjType->isArray() || isList || isMap || isSet || isStack || isQueue || isPQ)) {
            if (methodName == "length" || methodName == "size") {
                std::string fnName = isSet ? "sp_rt_set_size"
                                     : isStack ? "sp_rt_stack_size"
                                     : isQueue ? "sp_rt_queue_size"
                                     : isPQ ? "sp_rt_pq_size"
                                     : semaObjType->isArray() ? "sp_rt_array_length"
                                     : isMap ? "sp_rt_map_size"
                                     : "sp_rt_list_size";
                std::string lenReg = m_currentFunction->nextTempName("len");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    lenReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = emitCastIfNeeded(callInst, IRType::getInt32());
                m_lastExprType = IRType::getInt32();
                return;
            } else if (methodName == "isEmpty") {
                std::string fnName = isSet ? "sp_rt_set_is_empty"
                                     : isStack ? "sp_rt_stack_is_empty"
                                     : isQueue ? "sp_rt_queue_is_empty"
                                     : isPQ ? "sp_rt_pq_is_empty"
                                     : semaObjType->isArray() ? "sp_rt_array_is_empty"
                                     : isMap ? "sp_rt_map_is_empty"
                                     : "sp_rt_list_is_empty";
                std::string emptyReg = m_currentFunction->nextTempName("is_empty");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt1(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    emptyReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt1();
                return;
            } else if (methodName == "clear") {
                std::string fnName = isSet ? "sp_rt_set_clear"
                                     : isStack ? "sp_rt_stack_clear"
                                     : isQueue ? "sp_rt_queue_clear"
                                     : isPQ ? "sp_rt_pq_clear"
                                     : isMap ? "sp_rt_map_clear"
                                     : "sp_rt_list_clear";
                auto callInst = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    fnName,
                    std::vector<IRValuePtr>{objVal}
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = nullptr;
                m_lastExprType = IRType::getVoid();
                return;
            } else if (methodName == "removeAt") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getVoid(),
                        "sp_rt_list_remove_at",
                        std::vector<IRValuePtr>{objVal, idxVal}
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "push" || methodName == "add" || methodName == "enqueue") {
                if (isPQ && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto itemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    node.getArgs()[1]->accept(*this);
                    auto prioVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getVoid(),
                        "sp_rt_pq_push",
                        std::vector<IRValuePtr>{objVal, itemVal, prioVal}
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    if (isSet) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_add_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_add_i64",
                                std::vector<IRValuePtr>{objVal, elemVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else if (isStack) {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            "sp_rt_stack_push",
                            std::vector<IRValuePtr>{objVal, elemVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    } else if (isQueue) {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            "sp_rt_queue_enqueue",
                            std::vector<IRValuePtr>{objVal, elemVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    } else {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            "sp_rt_list_push_i64",
                            std::vector<IRValuePtr>{objVal, elemVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "contains" || methodName == "containsKey") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string hasReg = m_currentFunction->nextTempName("has_item");
                    if (isSet) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_set_contains_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_set_contains_i64",
                                std::vector<IRValuePtr>{objVal, elemVal},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_map_contains_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_map_contains_i64",
                                std::vector<IRValuePtr>{objVal, elemVal},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt1(),
                            "sp_rt_list_contains_i64",
                            std::vector<IRValuePtr>{objVal, elemVal},
                            hasReg
                        );
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = m_currentBlock->getInstructions().back();
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            } else if (methodName == "remove") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    if (isSet) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_remove_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_remove_i64",
                                std::vector<IRValuePtr>{objVal, elemVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_remove_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_remove_i64",
                                std::vector<IRValuePtr>{objVal, elemVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "pop" || methodName == "dequeue") {
                std::string popReg = m_currentFunction->nextTempName("pop_val");
                std::string fnName = isPQ ? "sp_rt_pq_pop"
                                     : isQueue ? "sp_rt_queue_dequeue"
                                     : isStack ? "sp_rt_stack_pop"
                                     : "sp_rt_list_pop_i64";
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    popReg
                );
                m_currentBlock->addInstruction(callInst);
                auto semaCallType = m_sema.getNodeType(&node);
                IRTypePtr retIRType = semaCallType ? mapType(semaCallType->toString()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(callInst, retIRType);
                m_lastExprType = retIRType;
                return;
            } else if (methodName == "peek") {
                std::string peekReg = m_currentFunction->nextTempName("peek_val");
                std::string fnName = isPQ ? "sp_rt_pq_peek"
                                     : isQueue ? "sp_rt_queue_peek"
                                     : "sp_rt_stack_peek";
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    peekReg
                );
                m_currentBlock->addInstruction(callInst);
                auto semaCallType = m_sema.getNodeType(&node);
                IRTypePtr retIRType = semaCallType ? mapType(semaCallType->toString()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(callInst, retIRType);
                m_lastExprType = retIRType;
                return;
            } else if (methodName == "peekPriority") {
                std::string prioReg = m_currentFunction->nextTempName("prio_val");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_pq_peek_priority",
                    std::vector<IRValuePtr>{objVal},
                    prioReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = emitCastIfNeeded(callInst, IRType::getInt32());
                m_lastExprType = IRType::getInt32();
                return;
            } else if (methodName == "get") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = m_lastExprValue;
                    if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        std::string getReg = m_currentFunction->nextTempName("map_get");
                        if (idxVal->getType()->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt64(),
                                "sp_rt_map_get_str",
                                std::vector<IRValuePtr>{objVal, idxVal},
                                getReg
                            );
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        } else {
                            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt64(),
                                "sp_rt_map_get_i64",
                                std::vector<IRValuePtr>{objVal, idxVal},
                                getReg
                            );
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        }
                        return;
                    } else {
                        idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                        std::string getReg = m_currentFunction->nextTempName("list_get");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            semaObjType->isArray() ? "sp_rt_array_get_i64" : "sp_rt_list_get_i64",
                            std::vector<IRValuePtr>{objVal, idxVal},
                            getReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
            } else if (methodName == "set") {
                if (node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto valVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        if (idxVal->getType()->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_set_str",
                                std::vector<IRValuePtr>{objVal, idxVal, valVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_set_i64",
                                std::vector<IRValuePtr>{objVal, idxVal, valVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else {
                        idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            semaObjType->isArray() ? "sp_rt_array_set_i64" : "sp_rt_list_set_i64",
                            std::vector<IRValuePtr>{objVal, idxVal, valVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "isEmpty") {
                std::string fnName = isSet ? "sp_rt_set_size"
                                     : semaObjType->isArray() ? "sp_rt_array_length"
                                     : (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) ? "sp_rt_map_size"
                                     : "sp_rt_list_size";
                std::string lenReg = m_currentFunction->nextTempName("len");
                auto lenCall = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    lenReg
                );
                m_currentBlock->addInstruction(lenCall);
                std::string cmpReg = m_currentFunction->nextTempName("is_empty");
                auto cmpInst = std::make_shared<CompareInst>(
                    IROpcode::ICmpEq,
                    lenCall,
                    std::make_shared<ConstantInt>(IRType::getInt64(), 0),
                    cmpReg
                );
                m_currentBlock->addInstruction(cmpInst);
                m_lastExprValue = cmpInst;
                m_lastExprType = IRType::getInt1();
                return;
            } else if (methodName == "map") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    std::string mapReg = m_currentFunction->nextTempName("map_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(),
                        semaObjType->isArray() ? "sp_rt_array_map" : "sp_rt_list_map",
                        std::vector<IRValuePtr>{objVal, fnVal},
                        mapReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
            } else if (methodName == "filter") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    std::string fltReg = m_currentFunction->nextTempName("flt_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(),
                        semaObjType->isArray() ? "sp_rt_array_filter" : "sp_rt_list_filter",
                        std::vector<IRValuePtr>{objVal, fnVal},
                        fltReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
            } else if (methodName == "reduce") {
                if (node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto initVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string redReg = m_currentFunction->nextTempName("red_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt64(),
                        semaObjType->isArray() ? "sp_rt_array_reduce" : "sp_rt_list_reduce",
                        std::vector<IRValuePtr>{objVal, fnVal, initVal},
                        redReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                } else if (node.getArgs().size() == 1) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    auto initVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
                    std::string redReg = m_currentFunction->nextTempName("red_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt64(),
                        semaObjType->isArray() ? "sp_rt_array_reduce" : "sp_rt_list_reduce",
                        std::vector<IRValuePtr>{objVal, fnVal, initVal},
                        redReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }
        }

        std::string className;
        // Phase 6 slice A: an interface-typed receiver dispatches
        // through the interface layout's global slot table (the slot
        // index is the same for every implementing class), so the slot
        // key is qualified with the interface name.
        bool receiverIsInterface = false;
        if (const auto* identObj = dynamic_cast<const ast::IdentifierExpr*>(&mem->getObject())) {
            if (identObj->getName() == "this" && m_currentClass) {
                className = m_currentClass->name;
            }
        }
        if (className.empty()) {
            auto semaType = m_sema.getNodeType(&mem->getObject());
            if (semaType && semaType->isNullable()) {
                const auto* nullT = static_cast<const sema::NullableType*>(semaType.get());
                if (nullT && nullT->getUnderlyingType()) {
                    semaType = nullT->getUnderlyingType();
                }
            }
            if (semaType && semaType->isClass()) {
                className = codegen::VTableBuilder::mangleGenericName(semaType->toString());
            } else if (semaType && semaType->isInterface()) {
                className = codegen::VTableBuilder::mangleGenericName(semaType->toString());
                receiverIsInterface = true;
            }
        }
        if (className.empty() && m_currentClass) {
            className = m_currentClass->name;
        }

        const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(className) : nullptr;
        // Phase 3 slice A: vtable slots are keyed by methodName + signature
        // suffix; use the overload sema resolved. Fall back to the legacy
        // name-only lookup (generic-class layouts are still keyed by plain
        // name, and unresolved calls keep the old behaviour).
        int slotIdx = -1;
        if (m_vtableBuilder && !className.empty()) {
            auto target = m_sema.getCallTarget(&node);
            if (target) {
                std::string key = methodName + codegen::VTableBuilder::overloadSuffix(targetParamTypes(target));
                if (receiverIsInterface) {
                    key = className + "::" + key;
                }
                slotIdx = m_vtableBuilder->getMethodVTableSlotByKey(className, key);
            }
            if (slotIdx < 0) {
                slotIdx = m_vtableBuilder->getMethodVTableSlot(className, methodName);
            }
        }

        if (layout && slotIdx >= 0) {
            // Polymorphic virtual dispatch via vtable!
            auto gepVTable = std::make_shared<GetElementPtrInst>(
                layout->structType,
                objVal,
                std::vector<IRValuePtr>{
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                },
                m_currentFunction->nextTempName("vt_slot")
            );
            m_currentBlock->addInstruction(gepVTable);

            auto loadVTable = std::make_shared<LoadInst>(
                IRType::getPointer(),
                gepVTable,
                m_currentFunction->nextTempName("vtable")
            );
            m_currentBlock->addInstruction(loadVTable);

            auto gepSlot = std::make_shared<GetElementPtrInst>(
                IRType::getPointer(),
                loadVTable,
                std::vector<IRValuePtr>{
                    std::make_shared<ConstantInt>(IRType::getInt32(), slotIdx)
                },
                m_currentFunction->nextTempName("slot_ptr")
            );
            m_currentBlock->addInstruction(gepSlot);

            auto loadFnPtr = std::make_shared<LoadInst>(
                IRType::getPointer(),
                gepSlot,
                m_currentFunction->nextTempName("fn_ptr")
            );
            m_currentBlock->addInstruction(loadFnPtr);

            IRTypePtr returnType = IRType::getVoid();
            const auto& slotInfo = layout->vtableSlots[slotIdx];
            if (slotInfo.functionType && slotInfo.functionType->isFunction()) {
                auto fnType = std::dynamic_pointer_cast<IRFunctionType>(slotInfo.functionType);
                if (fnType) returnType = fnType->getReturnType();
            } else {
                auto* existing = m_module->getFunction(slotInfo.mangledName);
                if (existing) returnType = existing->getReturnType();
            }

            std::vector<IRValuePtr> loweredArgs;
            loweredArgs.push_back(objVal);
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }

            // Phase 5 slice A: an async method called through a
            // virtual dispatch is spawned by passing the vtable-loaded
            // body address directly as the thunk to
            // sp_rt_async_spawn_sm (Design 2 — no indirect thunk, no
            // vtable change; the body itself is the thunk and
            // allocates its own frame, so the concrete override's
            // frame size is never needed at the call site).
            if (auto target = m_sema.getCallTarget(&node); target && target->isAsync()) {
                std::string kind;
                if (!taskPayloadKind(m_sema.getNodeType(&node), kind)) {
                    reportUnsupported(node.getLocation(), "call to async method",
                                      "the return type of this 'async' method cannot be carried by a Task "
                                      "(only void, int, bool, string, double, array and struct payloads are supported)");
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                // Param types come from sema (the vtable slot's
                // function type is the body's (task, ctx) signature
                // now, not the declared signature).
                auto thisType = m_sema.getTypeContext().findUserType(className);
                std::vector<IRTypePtr> paramTypes = asyncSpawnParamTypes(
                    target->getFunctionType()->getParamTypes(), thisType);
                IRValuePtr task = emitAsyncSpawnSM(loadFnPtr, loweredArgs, paramTypes);
                if (!task) {
                    reportUnsupported(node.getLocation(), "call to async method",
                                      "an argument of this 'async' method cannot be passed through a Task "
                                      "(only int, bool, string and double arguments are supported)");
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                m_lastExprValue = task;
                m_lastExprType = IRType::getPointer();
                return;
            }

            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("vcall");
            auto callInst = std::make_shared<CallInst>(returnType, loadFnPtr, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        } else {
            // Direct/static call
            std::string mangledName = (className.empty() ? "" : className + "_") + methodName;
            // Phase 3 slice A: an overloaded static method is mangled by
            // signature (the overload sema resolved).
            if (!className.empty()) {
                auto target = m_sema.getCallTarget(&node);
                if (target && !target->isGeneric()) {
                    auto userType = m_sema.getTypeContext().findUserType(className);
                    if (userType && userType->isClass()) {
                        auto clsType = std::static_pointer_cast<const sema::ClassType>(userType);
                        if (codegen::VTableBuilder::isMethodOverloaded(*clsType, methodName)) {
                            mangledName = className + "_" + methodName +
                                codegen::VTableBuilder::overloadSuffix(targetParamTypes(target));
                        }
                    }
                }
            }
            auto* targetFunc = m_module->getFunction(mangledName);
            IRTypePtr returnType = nullptr;
            if (targetFunc) {
                returnType = targetFunc->getReturnType();
            } else if (m_astFunctions.count(mangledName)) {
                returnType = mapType(m_astFunctions.at(mangledName)->getReturnType());
            } else if (auto nodeType = m_sema.getNodeType(&node)) {
                returnType = mapType(nodeType.get());
            } else {
                returnType = IRType::getInt32();
            }

            std::vector<IRValuePtr> loweredArgs;
            if (targetFunc && !targetFunc->getParams().empty() && targetFunc->getParams()[0]->getName() == "%this") {
                loweredArgs.push_back(objVal);
            }
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }

            // Phase 5 slice A: calling an `async` method returns a
            // Task instead of running the body — spawn it (body as
            // the thunk + heap ctx) and hand the task to the caller.
            if (auto target = m_sema.getCallTarget(&node); target && target->isAsync()) {
                std::string kind;
                if (!taskPayloadKind(m_sema.getNodeType(&node), kind)) {
                    reportUnsupported(node.getLocation(), "call to async method",
                                      "the return type of this 'async' method cannot be carried by a Task "
                                      "(only void, int, bool, string, double, array and struct payloads are supported)");
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                // Param types come from sema (the body's IR params
                // are now (task, ctx)); `this` is present iff
                // loweredArgs has one more value than the declared
                // params (a compiled instance method).
                size_t semaParamCount = target->getFunctionType()->getParamTypes().size();
                bool hasThis = (loweredArgs.size() == semaParamCount + 1);
                sema::TypePtr thisType = nullptr;
                if (hasThis) {
                    thisType = m_sema.getNodeType(&mem->getObject());
                    if (!thisType) thisType = m_sema.getTypeContext().findUserType(className);
                }
                std::vector<IRTypePtr> paramTypes = asyncSpawnParamTypes(
                    target->getFunctionType()->getParamTypes(), thisType);
                IRValuePtr task = emitAsyncSpawnSM(
                    std::make_shared<FunctionAddress>(mangledName), loweredArgs, paramTypes);
                if (!task) {
                    reportUnsupported(node.getLocation(), "call to async method",
                                      "an argument of this 'async' method cannot be passed through a Task "
                                      "(only int, bool, string and double arguments are supported)");
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                m_lastExprValue = task;
                m_lastExprType = IRType::getPointer();
                return;
            }

            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("c");
            auto callInst = std::make_shared<CallInst>(returnType, mangledName, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        }
    }

    const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getCallee());
    std::string calleeName = ident ? ident->getName() : "";
    if (node.isGeneric() && !calleeName.empty()) {
        std::string monoName = calleeName;
        for (const auto& arg : node.getTypeArgs()) {
            monoName += "_" + arg;
        }
        if (!m_module->getFunction(monoName) && m_genericFunctions.count(calleeName)) {
            compileSpecializedFunction(*m_genericFunctions.at(calleeName), node.getTypeArgs());
        }
        calleeName = monoName;
    }

    // Super constructor delegation
    if (calleeName == "super") {
        if (m_currentClass && !m_currentClass->baseClass.empty()) {
            std::string baseCtor = m_currentClass->baseClass + "_" + m_currentClass->baseClass;
            auto [thisAlloca, thisType] = lookupVariable("this");
            if (thisAlloca) {
                auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
                m_currentBlock->addInstruction(thisVal);

                std::vector<IRValuePtr> loweredArgs;
                loweredArgs.push_back(thisVal);
                for (const auto& arg : node.getArgs()) {
                    arg->accept(*this);
                    loweredArgs.push_back(m_lastExprValue);
                }

                auto callInst = std::make_shared<CallInst>(IRType::getVoid(), baseCtor, loweredArgs);
                m_currentBlock->addInstruction(callInst);

                // Restore derived class vtable pointer into slot 0
                if (m_currentClass->hasVTable) {
                    auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), m_currentClass->vtableName);
                    auto gep = std::make_shared<GetElementPtrInst>(
                        m_currentClass->structType,
                        thisVal,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                        },
                        m_currentFunction->nextTempName("vptr_restore")
                    );
                    m_currentBlock->addInstruction(gep);
                    m_currentBlock->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
                }
            }
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    // Check if callee is a method on 'this'
    // Phase 3 slice A: unqualified method call from inside a class.
    // Phase 6 slice A: this runs BEFORE the builtin special cases
    // (sqrt/log/panic/...) so a same-named method is never shadowed by
    // a builtin. The vtable map is keyed by methodName + signature
    // suffix, so use the overload sema resolved; the plain-name fallback
    // is only for calls sema left unresolved. The method path is taken
    // only when sema resolved the call to a method (isMethod) — a
    // same-signature builtin must win when sema picked it.
    int enclosingSlotIdx = -1;
    if (m_currentClass) {
        auto enclosingTarget = m_sema.getCallTarget(&node);
        if (enclosingTarget && enclosingTarget->isMethod()) {
            std::string key = calleeName + codegen::VTableBuilder::overloadSuffix(targetParamTypes(enclosingTarget));
            auto it = m_currentClass->methodSlotMap.find(key);
            if (it != m_currentClass->methodSlotMap.end()) enclosingSlotIdx = static_cast<int>(it->second);
        }
        if (enclosingSlotIdx < 0 && !enclosingTarget) {
            auto it = m_currentClass->methodSlotMap.find(calleeName);
            if (it != m_currentClass->methodSlotMap.end()) enclosingSlotIdx = static_cast<int>(it->second);
        }
    }
    if (enclosingSlotIdx >= 0) {
        int slotIdx = enclosingSlotIdx;
        auto [thisAlloca, thisType] = lookupVariable("this");
        if (thisAlloca) {
            auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
            m_currentBlock->addInstruction(thisVal);

            auto gepVTable = std::make_shared<GetElementPtrInst>(
                m_currentClass->structType,
                thisVal,
                std::vector<IRValuePtr>{
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                },
                m_currentFunction->nextTempName("vt_slot")
            );
            m_currentBlock->addInstruction(gepVTable);

            auto loadVTable = std::make_shared<LoadInst>(
                IRType::getPointer(),
                gepVTable,
                m_currentFunction->nextTempName("vtable")
            );
            m_currentBlock->addInstruction(loadVTable);

            auto gepSlot = std::make_shared<GetElementPtrInst>(
                IRType::getPointer(),
                loadVTable,
                std::vector<IRValuePtr>{std::make_shared<ConstantInt>(IRType::getInt32(), slotIdx)},
                m_currentFunction->nextTempName("slot_ptr")
            );
            m_currentBlock->addInstruction(gepSlot);

            auto loadFnPtr = std::make_shared<LoadInst>(IRType::getPointer(), gepSlot, m_currentFunction->nextTempName("fn_ptr"));
            m_currentBlock->addInstruction(loadFnPtr);

            IRTypePtr returnType = IRType::getVoid();
            const auto& slotInfo = m_currentClass->vtableSlots[slotIdx];
            if (slotInfo.functionType && slotInfo.functionType->isFunction()) {
                auto fnType = std::dynamic_pointer_cast<IRFunctionType>(slotInfo.functionType);
                if (fnType) returnType = fnType->getReturnType();
            } else {
                auto* existing = m_module->getFunction(slotInfo.mangledName);
                if (existing) returnType = existing->getReturnType();
            }

            std::vector<IRValuePtr> loweredArgs;
            loweredArgs.push_back(thisVal);
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }

            // Phase 5 slice A: an async method called unqualified
            // from inside its class also dispatches virtually — spawn
            // it by passing the vtable-loaded body address directly
            // as the thunk to sp_rt_async_spawn_sm (Design 2).
            if (auto enclosingTarget = m_sema.getCallTarget(&node); enclosingTarget && enclosingTarget->isAsync()) {
                std::string kind;
                if (!taskPayloadKind(m_sema.getNodeType(&node), kind)) {
                    reportUnsupported(node.getLocation(), "call to async method",
                                      "the return type of this 'async' method cannot be carried by a Task "
                                      "(only void, int, bool, string, double, array and struct payloads are supported)");
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                // Param types come from sema; `this` is the current
                // class (loweredArgs starts with thisVal).
                sema::TypePtr thisType = nullptr;
                if (m_currentClass) thisType = m_sema.getTypeContext().findUserType(m_currentClass->name);
                std::vector<IRTypePtr> paramTypes = asyncSpawnParamTypes(
                    enclosingTarget->getFunctionType()->getParamTypes(), thisType);
                IRValuePtr task = emitAsyncSpawnSM(loadFnPtr, loweredArgs, paramTypes);
                if (!task) {
                    reportUnsupported(node.getLocation(), "call to async method",
                                      "an argument of this 'async' method cannot be passed through a Task "
                                      "(only int, bool, string and double arguments are supported)");
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
                m_lastExprValue = task;
                m_lastExprType = IRType::getPointer();
                return;
            }

            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("vcall");
            auto callInst = std::make_shared<CallInst>(returnType, loadFnPtr, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        }
    }

    if (calleeName == "Ok" && node.getArgs().size() == 1) {
        node.getArgs()[0]->accept(*this);
        m_lastExprValue = emitCreateTaggedUnion("Result", 0, m_lastExprValue);
        m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        return;
    }
    if (calleeName == "Err" && node.getArgs().size() == 1) {
        node.getArgs()[0]->accept(*this);
        m_lastExprValue = emitCreateTaggedUnion("Result", 1, m_lastExprValue);
        m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        return;
    }
    if (calleeName == "Some" && node.getArgs().size() == 1) {
        node.getArgs()[0]->accept(*this);
        m_lastExprValue = emitCreateTaggedUnion("Option", 0, m_lastExprValue);
        m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
        return;
    }
    if (calleeName == "None" && node.getArgs().empty()) {
        m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
        m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
        return;
    }

    // Built-in standard functions: println, print, assert, panic
    if (calleeName == "println") {
        if (node.getArgs().empty()) {
            auto emptyStr = m_module->getOrCreateStringLiteral("");
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{emptyStr});
            m_currentBlock->addInstruction(call);
        } else {
            node.getArgs()[0]->accept(*this);
            IRValuePtr argVal = m_lastExprValue;
            IRTypePtr argType = m_lastExprType;

            auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
            if (argType && argType->isPointer()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isFloat()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_float", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isDouble()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_double", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isInt1()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_bool", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isChar()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_char", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isUnsignedInteger()) {
                if (!argType->isInt64()) {
                    std::string zextName = m_currentFunction ? m_currentFunction->nextTempName("zext") : "%zext";
                    auto zext = std::make_shared<CastInst>(IROpcode::ZExt, argVal, IRType::getInt64(), zextName);
                    if (m_currentBlock) m_currentBlock->addInstruction(zext);
                    argVal = zext;
                }
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_uint", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else {
                argVal = emitCastIfNeeded(argVal, IRType::getInt64());
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_int", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            }
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "print") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            IRValuePtr argVal = m_lastExprValue;
            IRTypePtr argType = m_lastExprType;

            auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
            if (argType && argType->isPointer()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_string", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isFloat()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_float", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isDouble()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_double", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isInt1()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_bool", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isChar()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_char", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isUnsignedInteger()) {
                if (!argType->isInt64()) {
                    std::string zextName = m_currentFunction ? m_currentFunction->nextTempName("zext") : "%zext";
                    auto zext = std::make_shared<CastInst>(IROpcode::ZExt, argVal, IRType::getInt64(), zextName);
                    if (m_currentBlock) m_currentBlock->addInstruction(zext);
                    argVal = zext;
                }
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_uint", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else {
                argVal = emitCastIfNeeded(argVal, IRType::getInt64());
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_int", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            }
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "readLine" || calleeName == "readln") {
        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("readline") : "%readline";
        auto call = std::make_shared<CallInst>(
            IRType::getPointer(IRType::getInt8()),
            "sp_rt_read_line",
            std::vector<IRValuePtr>{},
            resName
        );
        m_currentBlock->addInstruction(call);
        m_lastExprValue = call;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }

    if (calleeName == "eprintln") {
        if (node.getArgs().empty()) {
            auto emptyStr = m_module->getOrCreateStringLiteral("");
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{emptyStr});
            m_currentBlock->addInstruction(call);
        } else {
            node.getArgs()[0]->accept(*this);
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{m_lastExprValue});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "eprint") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprint_string", std::vector<IRValuePtr>{m_lastExprValue});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "flush") {
        auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_flush", std::vector<IRValuePtr>{});
        m_currentBlock->addInstruction(call);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "eflush") {
        auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eflush", std::vector<IRValuePtr>{});
        m_currentBlock->addInstruction(call);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "assert") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            IRValuePtr condVal = m_lastExprValue;
            condVal = emitCastIfNeeded(condVal, IRType::getInt1());

            IRValuePtr msgVal = nullptr;
            if (node.getArgs().size() >= 2) {
                node.getArgs()[1]->accept(*this);
                msgVal = m_lastExprValue;
            } else {
                msgVal = m_module->getOrCreateStringLiteral("assertion failed");
            }

            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_assert", std::vector<IRValuePtr>{condVal, msgVal});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "panic") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            IRValuePtr msgVal = m_lastExprValue;
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_panic", std::vector<IRValuePtr>{msgVal});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "sqrt" || calleeName == "sin" || calleeName == "cos" || calleeName == "tan" ||
        calleeName == "asin" || calleeName == "acos" || calleeName == "atan" ||
        calleeName == "floor" || calleeName == "ceil" || calleeName == "round" ||
        calleeName == "log" || calleeName == "log10" || calleeName == "log2" || calleeName == "exp") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            auto val = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
            std::string fn = "sp_rt_math_" + calleeName;
            std::string reg = m_currentFunction->nextTempName("math_res");
            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{val}, reg);
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getDouble();
            return;
        }
    }

    if (calleeName == "pow" || calleeName == "atan2") {
        if (node.getArgs().size() >= 2) {
            node.getArgs()[0]->accept(*this);
            auto v1 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
            node.getArgs()[1]->accept(*this);
            auto v2 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
            std::string fn = "sp_rt_math_" + calleeName;
            std::string reg = m_currentFunction->nextTempName("math_res");
            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getDouble();
            return;
        }
    }

    if (calleeName == "abs") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            auto val = m_lastExprValue;
            auto type = m_lastExprType;
            auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
            bool isFlt = (type && type->isFloatingPoint()) || (semaType && (semaType->isFloat() || semaType->isDouble()));
            if (isFlt) {
                val = emitCastIfNeeded(val, IRType::getDouble());
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getDouble(), "sp_rt_math_abs_double", std::vector<IRValuePtr>{val}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getDouble();
            } else {
                val = emitCastIfNeeded(val, IRType::getInt64());
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_math_abs_int", std::vector<IRValuePtr>{val}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
            }
            return;
        }
    }

    if (calleeName == "min" || calleeName == "max") {
        if (node.getArgs().size() >= 2) {
            node.getArgs()[0]->accept(*this);
            auto v1 = m_lastExprValue;
            auto t1 = m_lastExprType;
            auto semaT1 = m_sema.getNodeType(node.getArgs()[0].get());
            node.getArgs()[1]->accept(*this);
            auto v2 = m_lastExprValue;
            auto t2 = m_lastExprType;
            auto semaT2 = m_sema.getNodeType(node.getArgs()[1].get());
            bool isFlt = (t1 && t1->isFloatingPoint()) || (t2 && t2->isFloatingPoint()) ||
                         (semaT1 && (semaT1->isFloat() || semaT1->isDouble())) ||
                         (semaT2 && (semaT2->isFloat() || semaT2->isDouble()));
            if (isFlt) {
                v1 = emitCastIfNeeded(v1, IRType::getDouble());
                v2 = emitCastIfNeeded(v2, IRType::getDouble());
                std::string fn = (calleeName == "min") ? "sp_rt_math_min_double" : "sp_rt_math_max_double";
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getDouble();
            } else {
                v1 = emitCastIfNeeded(v1, IRType::getInt64());
                v2 = emitCastIfNeeded(v2, IRType::getInt64());
                std::string fn = (calleeName == "min") ? "sp_rt_math_min_int" : "sp_rt_math_max_int";
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getInt64(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
            }
            return;
        }
    }

    if (calleeName == "tryReadText" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string existsReg = m_currentFunction->nextTempName("fs_exists");
        auto existsCall = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_exists", std::vector<IRValuePtr>{p}, existsReg);
        m_currentBlock->addInstruction(existsCall);

        auto thenBB = m_currentFunction->createBasicBlock("try_read_ok");
        auto elseBB = m_currentFunction->createBasicBlock("try_read_err");
        auto mergeBB = m_currentFunction->createBasicBlock("try_read_end");

        auto branchInst = std::make_shared<CondBranchInst>(existsCall, thenBB->getName(), elseBB->getName());
        m_currentBlock->addInstruction(branchInst);

        auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_read_slot"));
        m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

        // Success block
        setInsertPoint(thenBB);
        std::string readReg = m_currentFunction->nextTempName("fs_res");
        auto readCall = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_fs_read_text", std::vector<IRValuePtr>{p}, readReg);
        thenBB->addInstruction(readCall);
        emitPopulateTaggedUnion(resultSlot, "Result", 0, readCall);
        thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Err block (IOError.NotFound = 0)
        setInsertPoint(elseBB);
        auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
        emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
        elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Merge block
        setInsertPoint(mergeBB);
        m_lastExprValue = resultSlot;
        m_lastExprType = structType;
        return;
    }

    if ((calleeName == "tryWriteText" || calleeName == "tryAppendText") && node.getArgs().size() >= 2) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        node.getArgs()[1]->accept(*this);
        auto c = m_lastExprValue;
        std::string fn = (calleeName == "tryWriteText") ? "sp_rt_fs_write_text" : "sp_rt_fs_append_text";
        std::string writeReg = m_currentFunction->nextTempName("fs_res");
        auto writeCall = std::make_shared<CallInst>(IRType::getInt1(), fn, std::vector<IRValuePtr>{p, c}, writeReg);
        m_currentBlock->addInstruction(writeCall);

        auto thenBB = m_currentFunction->createBasicBlock("try_write_ok");
        auto elseBB = m_currentFunction->createBasicBlock("try_write_err");
        auto mergeBB = m_currentFunction->createBasicBlock("try_write_end");

        auto branchInst = std::make_shared<CondBranchInst>(writeCall, thenBB->getName(), elseBB->getName());
        m_currentBlock->addInstruction(branchInst);

        auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_write_slot"));
        m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

        // Success block
        setInsertPoint(thenBB);
        auto trueVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
        emitPopulateTaggedUnion(resultSlot, "Result", 0, trueVal);
        thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Err block (IOError.PermissionDenied = 1)
        setInsertPoint(elseBB);
        auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
        emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
        elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Merge block
        setInsertPoint(mergeBB);
        m_lastExprValue = resultSlot;
        m_lastExprType = structType;
        return;
    }

    if ((calleeName == "tryDelete" || calleeName == "tryDeleteFile") && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string delReg = m_currentFunction->nextTempName("fs_res");
        auto delCall = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_delete", std::vector<IRValuePtr>{p}, delReg);
        m_currentBlock->addInstruction(delCall);

        auto thenBB = m_currentFunction->createBasicBlock("try_del_ok");
        auto elseBB = m_currentFunction->createBasicBlock("try_del_err");
        auto mergeBB = m_currentFunction->createBasicBlock("try_del_end");

        auto branchInst = std::make_shared<CondBranchInst>(delCall, thenBB->getName(), elseBB->getName());
        m_currentBlock->addInstruction(branchInst);

        auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_del_slot"));
        m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

        // Success block
        setInsertPoint(thenBB);
        auto trueVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
        emitPopulateTaggedUnion(resultSlot, "Result", 0, trueVal);
        thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Err block (IOError.NotFound = 0)
        setInsertPoint(elseBB);
        auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
        emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
        elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Merge block
        setInsertPoint(mergeBB);
        m_lastExprValue = resultSlot;
        m_lastExprType = structType;
        return;
    }

    if (calleeName == "tryCreateDir" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string mkdirReg = m_currentFunction->nextTempName("fs_res");
        auto mkdirCall = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_create_dir", std::vector<IRValuePtr>{p}, mkdirReg);
        m_currentBlock->addInstruction(mkdirCall);

        auto thenBB = m_currentFunction->createBasicBlock("try_mkdir_ok");
        auto elseBB = m_currentFunction->createBasicBlock("try_mkdir_err");
        auto mergeBB = m_currentFunction->createBasicBlock("try_mkdir_end");

        auto branchInst = std::make_shared<CondBranchInst>(mkdirCall, thenBB->getName(), elseBB->getName());
        m_currentBlock->addInstruction(branchInst);

        auto structType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        auto resultSlot = std::make_shared<AllocaInst>(structType, m_currentFunction->nextTempName("try_mkdir_slot"));
        m_currentFunction->getEntryBlock()->addInstruction(resultSlot);

        // Success block
        setInsertPoint(thenBB);
        auto trueVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
        emitPopulateTaggedUnion(resultSlot, "Result", 0, trueVal);
        thenBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Err block (IOError.PermissionDenied = 1)
        setInsertPoint(elseBB);
        auto errCodeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
        emitPopulateTaggedUnion(resultSlot, "Result", 1, errCodeVal);
        elseBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

        // Merge block
        setInsertPoint(mergeBB);
        m_lastExprValue = resultSlot;
        m_lastExprType = structType;
        return;
    }

    if (calleeName == "tryReadLine") {
        std::string reg = m_currentFunction->nextTempName("line_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_read_line", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = emitCreateTaggedUnion("Result", 0, callInst);
        m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        return;
    }

    if (calleeName == "readText" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_fs_read_text", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if ((calleeName == "writeText" || calleeName == "appendText") && node.getArgs().size() >= 2) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        node.getArgs()[1]->accept(*this);
        auto c = m_lastExprValue;
        std::string fn = (calleeName == "writeText") ? "sp_rt_fs_write_text" : "sp_rt_fs_append_text";
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), fn, std::vector<IRValuePtr>{p, c}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "exists" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_exists", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if ((calleeName == "delete" || calleeName == "deleteFile") && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_delete", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "createDir" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_create_dir", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "listDir" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_fs_list_dir", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer();
        return;
    }
    if (calleeName == "isFile" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_file", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "isDir" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_dir", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }

    if (calleeName == "now") {
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ms", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "nowSeconds") {
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_secs", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "nowNanos") {
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ns", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if ((calleeName == "sleep" || calleeName == "sleepSeconds") && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        std::string fn = (calleeName == "sleep") ? "sp_rt_time_sleep_ms" : "sp_rt_time_sleep_secs";
        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), fn, std::vector<IRValuePtr>{ms});
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }
    if (calleeName == "formatTime" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_time_format_iso", std::vector<IRValuePtr>{ms}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "parseTime" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto s = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_parse_iso", std::vector<IRValuePtr>{s}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }

    if (calleeName == "getEnv" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto n = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_get_env", std::vector<IRValuePtr>{n}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "setEnv" && node.getArgs().size() >= 2) {
        node.getArgs()[0]->accept(*this);
        auto n = m_lastExprValue;
        node.getArgs()[1]->accept(*this);
        auto v = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_os_set_env", std::vector<IRValuePtr>{n, v}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "platform") {
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_platform", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "arch") {
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_arch", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "args") {
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_os_args", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer();
        return;
    }
    if (calleeName == "exit" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto code = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_os_exit", std::vector<IRValuePtr>{code});
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "exec" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto cmd = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_exec", std::vector<IRValuePtr>{cmd}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "spawn" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto cmd = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_spawn", std::vector<IRValuePtr>{cmd}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if ((calleeName == "runCommand" || calleeName == "execCapture") && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto cmd = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_process_run", std::vector<IRValuePtr>{cmd}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "pid") {
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_pid", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }

    if (calleeName == "totalAllocatedBytes") {
        std::string reg = m_currentFunction->nextTempName("mem_bytes");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_mem_allocated_bytes", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "totalLiveObjects") {
        std::string reg = m_currentFunction->nextTempName("mem_objs");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_mem_live_objects", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }

    // Check if callee is a local variable holding a function pointer
    if (!calleeName.empty()) {
        auto [localAlloca, localType] = lookupVariable(calleeName);
        if (localAlloca != nullptr) {
            std::vector<IRValuePtr> loweredArgs;
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }
            auto loadFn = std::make_shared<LoadInst>(
                IRType::getPointer(),
                localAlloca,
                m_currentFunction->nextTempName("fn_ptr")
            );
            m_currentBlock->addInstruction(loadFn);

            auto semaType = m_sema.getNodeType(&node);
            IRTypePtr returnType = semaType ? mapType(semaType.get()) : IRType::getInt64();
            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("icall");

            auto callInst = std::make_shared<CallInst>(returnType, loadFn, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        }
    }

    if (calleeName.empty()) {
        std::vector<IRValuePtr> loweredArgs;
        for (const auto& arg : node.getArgs()) {
            arg->accept(*this);
            loweredArgs.push_back(m_lastExprValue);
        }
        node.getCallee().accept(*this);
        IRValuePtr calleeVal = m_lastExprValue;

        auto semaType = m_sema.getNodeType(&node);
        IRTypePtr returnType = semaType ? mapType(semaType.get()) : IRType::getInt64();
        std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("icall");

        auto callInst = std::make_shared<CallInst>(returnType, calleeVal, loweredArgs, callReg);
        m_currentBlock->addInstruction(callInst);

        m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
        m_lastExprType = returnType;
        return;
    }

    // Phase 3 slice A: resolve an overloaded free function to its mangled
    // name (the overload sema picked). Builtins above already returned;
    // generic calls were rewritten to their monomorphized name above.
    if (!calleeName.empty()) {
        auto callTarget = m_sema.getCallTarget(&node);
        if (callTarget && !callTarget->isMethod()) {
            calleeName = llvmCallName(calleeName, callTarget);
        }
    }

    // General user function call
    std::vector<IRValuePtr> loweredArgs;
    auto* targetFunc = m_module->getFunction(calleeName);
    for (size_t i = 0; i < node.getArgs().size(); ++i) {
        node.getArgs()[i]->accept(*this);
        IRValuePtr argVal = m_lastExprValue;
        if (targetFunc && i < targetFunc->getParams().size()) {
            auto expectedType = targetFunc->getParams()[i]->getType();
            if (expectedType && expectedType->isStruct() && argVal && argVal->getType()->isPointer()) {
                auto loadInst = std::make_shared<LoadInst>(expectedType, argVal, m_currentFunction->nextTempName("tu_arg"));
                m_currentBlock->addInstruction(loadInst);
                argVal = loadInst;
            }
        }
        loweredArgs.push_back(argVal);
    }
    if (m_astFunctions.count(calleeName)) {
        const auto* fnDecl = m_astFunctions.at(calleeName);
        const auto& params = fnDecl->getParams();
        // Phase 5 slice A: for an async fn the body's IR params are
        // (task, ctx), so a default value's cast type comes from the
        // declared param type (sema), not from the body's params.
        auto calleeTarget = m_sema.getCallTarget(&node);
        bool calleeIsAsync = calleeTarget && calleeTarget->isAsync();
        for (size_t i = loweredArgs.size(); i < params.size(); ++i) {
            if (params[i].hasDefaultValue()) {
                params[i].getDefaultValue()->accept(*this);
                IRValuePtr defVal = m_lastExprValue;
                if (calleeIsAsync) {
                    const auto& semaParams = calleeTarget->getFunctionType()->getParamTypes();
                    if (i < semaParams.size() && semaParams[i]) {
                        defVal = emitCastIfNeeded(defVal, mapType(semaParams[i].get()));
                    }
                } else if (targetFunc && i < targetFunc->getParams().size()) {
                    defVal = emitCastIfNeeded(defVal, targetFunc->getParams()[i]->getType());
                }
                loweredArgs.push_back(defVal);
            }
        }
    }

    IRTypePtr returnType = IRType::getInt32();
    if (targetFunc) {
        returnType = targetFunc->getReturnType();
    } else if (m_astFunctions.count(calleeName)) {
        returnType = mapType(m_astFunctions.at(calleeName)->getReturnType());
    }

    // Phase 5 slice A: calling an `async fn` returns a Task instead
    // of running the body — spawn it (the body itself is the thunk
    // + a heap ctx) and hand the task to the caller. The callee name
    // is already the monomorphized name for generic calls (see above).
    if (auto target = m_sema.getCallTarget(&node); target && target->isAsync()) {
        std::string kind;
        if (!taskPayloadKind(m_sema.getNodeType(&node), kind)) {
            reportUnsupported(node.getLocation(), "call to async function",
                              "the return type of this 'async fn' cannot be carried by a Task "
                              "(only void, int, bool, string, double, array and struct payloads are supported)");
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }
        // Param types come from sema (the body's IR params are now
        // (task, ctx)); a free fn has no `this`.
        std::vector<IRTypePtr> paramTypes = asyncSpawnParamTypes(
            target->getFunctionType()->getParamTypes(), nullptr);
        IRValuePtr task = emitAsyncSpawnSM(
            std::make_shared<FunctionAddress>(calleeName), loweredArgs, paramTypes);
        if (!task) {
            reportUnsupported(node.getLocation(), "call to async function",
                              "an argument of this 'async fn' cannot be passed through a Task "
                              "(only int, bool, string and double arguments are supported)");
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }
        m_lastExprValue = task;
        m_lastExprType = IRType::getPointer();
        return;
    }

    std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("c");
    auto callInst = std::make_shared<CallInst>(returnType, calleeName, loweredArgs, callReg);
    m_currentBlock->addInstruction(callInst);

    m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
    m_lastExprType = returnType;
}

void IRBuilder::visit(const ast::TernaryExpr& node) {
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    condVal = emitCastIfNeeded(condVal, IRType::getInt1());

    auto thenBB = createBlock("tern_then");
    auto elseBB = createBlock("tern_else");
    auto mergeBB = createBlock("tern_merge");

    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, thenBB->getName(), elseBB->getName()));

    // Then
    setInsertPoint(thenBB);
    node.getThenExpr().accept(*this);
    IRValuePtr thenVal = m_lastExprValue;
    IRTypePtr resType = m_lastExprType ? m_lastExprType : IRType::getInt32();
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

    // Else
    setInsertPoint(elseBB);
    node.getElseExpr().accept(*this);
    IRValuePtr elseVal = m_lastExprValue;
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

    // Result alloca in entry block
    auto resAlloca = std::make_shared<AllocaInst>(resType, m_currentFunction->nextTempName("tern_res"));
    m_currentFunction->getEntryBlock()->addInstruction(resAlloca);

    // Store in both branches
    thenBB->addInstruction(std::make_shared<StoreInst>(thenVal, resAlloca));
    elseBB->addInstruction(std::make_shared<StoreInst>(elseVal, resAlloca));

    setInsertPoint(mergeBB);
    auto loadInst = std::make_shared<LoadInst>(resType, resAlloca, m_currentFunction->nextTempName("tern_val"));
    mergeBB->addInstruction(loadInst);

    m_lastExprValue = loadInst;
    m_lastExprType = resType;
}

IRValuePtr IRBuilder::nullValueOf(IRTypePtr type) {
    if (!type) return nullptr;
    if (type->isPointer()) return std::make_shared<ConstantNull>(type);
    if (type->isStruct()) return std::make_shared<ConstantNull>(type);
    if (type->isInt1()) return std::make_shared<ConstantBool>(false);
    if (type->isInteger()) return std::make_shared<ConstantInt>(type, 0);
    if (type->isFloatingPoint()) return std::make_shared<ConstantFloat>(type, 0.0);
    return nullptr;
}

IRValuePtr IRBuilder::emitMemberPointer(const ast::MemberExpr& node, IRTypePtr& outFieldType, IRValuePtr preEvaluatedObject) {
    // 1. Evaluate the base object (unless the caller already did —
    // safe navigation evaluates it once for the null check and reuses
    // the value here so side effects run exactly once).
    IRValuePtr objPtr = preEvaluatedObject;
    if (!objPtr) {
        node.getObject().accept(*this);
        objPtr = m_lastExprValue;
    }
    if (!objPtr) return nullptr;

    // 2. Identify the class or struct of the object
    std::string className;
    if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getObject())) {
        if (ident->getName() == "this" && m_currentClass) {
            className = m_currentClass->name;
        } else {
            auto [varAlloca, varType] = lookupVariable(ident->getName());
            if (varType && varType->isStruct()) {
                auto st = std::dynamic_pointer_cast<IRStructType>(varType);
                if (st) {
                    className = st->getName();
                    if (className.rfind("struct.", 0) == 0) {
                        className = className.substr(7);
                    }
                }
            }
        }
    }

    if (className.empty()) {
        auto semaType = m_sema.getNodeType(&node.getObject());
        if (semaType && semaType->isNullable()) {
            const auto* nullT = static_cast<const sema::NullableType*>(semaType.get());
            if (nullT && nullT->getUnderlyingType()) {
                semaType = nullT->getUnderlyingType();
            }
        }
        if (semaType && (semaType->isClass() || semaType->isStruct() || semaType->isEnum())) {
            className = semaType->toString();
        }
    }

    if (className.empty() && m_currentClass) {
        className = m_currentClass->name;
    }

    if (className.empty() || !m_vtableBuilder) return nullptr;
    const auto* layout = m_vtableBuilder->getClassLayout(className);
    if (!layout) return nullptr;

    // 3. Find field index in class layout
    auto it = layout->fieldIndexMap.find(node.getMember());
    if (it == layout->fieldIndexMap.end()) return nullptr;

    uint32_t fieldIndex = it->second;
    size_t fVecIdx = layout->hasVTable ? (fieldIndex - 1) : fieldIndex;
    if (fVecIdx >= layout->fields.size()) return nullptr;

    outFieldType = layout->fields[fVecIdx].irType;

    // 4. Emit GEP
    auto gep = std::make_shared<GetElementPtrInst>(
        layout->structType,
        objPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), fieldIndex)
        },
        m_currentFunction->nextTempName("fld_ptr")
    );
    m_currentBlock->addInstruction(gep);
    return gep;
}

void IRBuilder::visit(const ast::MemberExpr& node) {
    // 1. Check for Enum variant access (e.g. Direction.North, HttpStatus.OK)
    std::string objIdentName;
    if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getObject())) {
        objIdentName = ident->getName();
    }
    if (!objIdentName.empty()) {
        auto enumUserType = m_sema.getTypeContext().findUserType(objIdentName);
        if (enumUserType && enumUserType->isEnum()) {
            auto enumType = std::static_pointer_cast<sema::EnumType>(enumUserType);
            if (enumType->isTaggedUnion()) {
                if (objIdentName == "Option" && node.getMember() == "None") {
                    m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
                    m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            } else {
                int64_t val = enumType->getVariantValue(node.getMember());
                m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
                m_lastExprType = IRType::getInt32();
                return;
            }
        }
        if (objIdentName == "Math") {
            if (node.getMember() == "PI") {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 3.141592653589793);
                m_lastExprType = IRType::getDouble();
                return;
            }
            if (node.getMember() == "E") {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 2.718281828459045);
                m_lastExprType = IRType::getDouble();
                return;
            }
            if (node.getMember() == "TAU") {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 6.283185307179586);
                m_lastExprType = IRType::getDouble();
                return;
            }
        }
    }
    auto semaType = m_sema.getNodeType(&node.getObject());
    if (semaType && semaType->isEnum()) {
        auto enumType = std::static_pointer_cast<sema::EnumType>(semaType);
        if (!enumType->isTaggedUnion()) {
            int64_t val = enumType->getVariantValue(node.getMember());
            m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
            m_lastExprType = IRType::getInt32();
            return;
        }
    }

    // 2. Built-in collection property access (.length / .size)
    if (semaType && (semaType->isArray() || semaType->isString() || semaType->isMap() ||
                    (semaType->isClass() && (semaType->toString().rfind("List", 0) == 0 || semaType->toString().rfind("Map", 0) == 0)))) {
        if (node.getMember() == "length" || node.getMember() == "size") {
            node.getObject().accept(*this);
            auto objVal = m_lastExprValue;
            std::string fnName = semaType->isString() ? "sp_rt_str_len"
                                 : semaType->isArray() ? "sp_rt_array_length"
                                 : (semaType->isMap() || semaType->toString().rfind("Map", 0) == 0) ? "sp_rt_map_size"
                                 : "sp_rt_list_size";
            std::string lenReg = m_currentFunction->nextTempName("len");
            auto callInst = std::make_shared<CallInst>(
                IRType::getInt64(),
                fnName,
                std::vector<IRValuePtr>{objVal},
                lenReg
            );
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getInt64();
            return;
        }
    }

    if (semaType && semaType->isClass() && semaType->toString() == "Duration") {
        if (node.getMember() == "millis") {
            node.getObject().accept(*this);
            auto objVal = m_lastExprValue;
            std::string reg = m_currentFunction->nextTempName("dur_ms");
            auto callInst = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_duration_to_millis",
                std::vector<IRValuePtr>{objVal},
                reg
            );
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getInt64();
            return;
        }
    }

    // 3. Class/Struct member field access
    IRTypePtr fieldType = nullptr;
    IRValuePtr fieldPtr = nullptr;
    IRValuePtr objVal = nullptr;
    if (node.isSafeNav()) {
        // Phase 6 slice A: safe navigation (?.). Before this fix the
        // field was loaded unconditionally, so a null object segfaulted
        // here (the GEP + load ran on a null base with no null check).
        // The object is evaluated once, up front; the GEP below only
        // computes an address (legal on a null base — it does not
        // dereference), and the load itself runs only in the non-null
        // branch. A null object yields the null value of the field's
        // type (null for pointers, 0 / false / 0.0 otherwise) — the
        // same convention `??` and `== null` use for value types.
        // NOTE: `?.` METHOD calls (obj?.method()) are not covered by
        // this fix — the member-call path in visit(CallExpr) has no
        // null check and still segfaults on a null receiver (see the
        // slice doc for the follow-up).
        node.getObject().accept(*this);
        objVal = m_lastExprValue;
        if (!objVal) {
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }
        fieldPtr = emitMemberPointer(node, fieldType, objVal);
    } else {
        fieldPtr = emitMemberPointer(node, fieldType);
    }
    if (fieldPtr && fieldType) {
        if (node.isSafeNav() && m_currentFunction && m_currentBlock && m_currentFunction->getEntryBlock()) {
            auto isNull = std::make_shared<CompareInst>(
                IROpcode::ICmpEq, objVal, std::make_shared<ConstantNull>(objVal->getType()),
                m_currentFunction->nextTempName("safenav_null"));
            m_currentBlock->addInstruction(isNull);

            auto nullBB = createBlock("safenav_null");
            auto accessBB = createBlock("safenav_access");
            auto mergeBB = createBlock("safenav_merge");

            auto resAlloca = std::make_shared<AllocaInst>(fieldType, m_currentFunction->nextTempName("safenav_res"));
            m_currentFunction->getEntryBlock()->addInstruction(resAlloca);
            m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(isNull, nullBB->getName(), accessBB->getName()));

            setInsertPoint(nullBB);
            if (auto nullVal = nullValueOf(fieldType)) {
                nullBB->addInstruction(std::make_shared<StoreInst>(nullVal, resAlloca));
            }
            nullBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

            setInsertPoint(accessBB);
            auto loadVal = std::make_shared<LoadInst>(
                fieldType, fieldPtr, m_currentFunction->nextTempName("mem_val"));
            accessBB->addInstruction(loadVal);
            accessBB->addInstruction(std::make_shared<StoreInst>(loadVal, resAlloca));
            accessBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

            setInsertPoint(mergeBB);
            auto mergeVal = std::make_shared<LoadInst>(
                fieldType, resAlloca, m_currentFunction->nextTempName("safenav_val"));
            mergeBB->addInstruction(mergeVal);
            m_lastExprValue = mergeVal;
            m_lastExprType = fieldType;
        } else {
            auto loadVal = std::make_shared<LoadInst>(
                fieldType,
                fieldPtr,
                m_currentFunction->nextTempName("mem_val")
            );
            m_currentBlock->addInstruction(loadVal);
            m_lastExprValue = loadVal;
            m_lastExprType = fieldType;
        }
    } else {
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
    }
}

void IRBuilder::visit(const ast::IndexExpr& node) {
    node.getObject().accept(*this);
    IRValuePtr objVal = m_lastExprValue;

    node.getIndex().accept(*this);
    IRValuePtr idxVal = m_lastExprValue;

    if (!objVal || !idxVal) {
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }

    auto semaObjType = m_sema.getNodeType(&node.getObject());
    auto semaResultType = m_sema.getNodeType(&node);
    if (semaResultType && semaResultType->isNullable()) {
        const auto* nullT = static_cast<const sema::NullableType*>(semaResultType.get());
        if (nullT && nullT->getUnderlyingType()) {
            semaResultType = nullT->getUnderlyingType();
        }
    }
    IRTypePtr targetIRType = mapType(semaResultType.get());

    bool isMapType = semaObjType && (semaObjType->isMap() || (semaObjType->isClass() && semaObjType->toString().rfind("Map", 0) == 0));
    bool isListType = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("List", 0) == 0;
    bool isPtrType = semaObjType && semaObjType->isPointer();

    if (isPtrType) {
        auto semaPointee = static_cast<const sema::PointerType&>(*semaObjType).getPointeeType();
        IRTypePtr elemType = semaPointee ? mapType(semaPointee.get()) : IRType::getInt64();
        std::string gepName = m_currentFunction->nextTempName("ptr_gep");
        auto gep = std::make_shared<GetElementPtrInst>(
            elemType, objVal,
            std::vector<IRValuePtr>{idxVal},
            gepName
        );
        m_currentBlock->addInstruction(gep);
        auto loadInst = std::make_shared<LoadInst>(elemType, gep, m_currentFunction->nextTempName("ptr_elem"));
        m_currentBlock->addInstruction(loadInst);
        m_lastExprValue = loadInst;
        m_lastExprType = elemType;
        return;
    }

    if (isMapType) {
        if (idxVal->getType()->isPointer()) {
            std::string getReg = m_currentFunction->nextTempName("map_val");
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_map_get_str",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        } else {
            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
            std::string getReg = m_currentFunction->nextTempName("map_val");
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_map_get_i64",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        }
        return;
    }

    idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());

    if (isListType) {
        if (targetIRType && targetIRType->isFloatingPoint()) {
            std::string getReg = m_currentFunction->nextTempName("list_elem");
            auto getCall = std::make_shared<CallInst>(
                IRType::getDouble(),
                "sp_rt_list_get_f64",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        } else {
            std::string getReg = m_currentFunction->nextTempName("list_elem");
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_list_get_i64",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        }
        return;
    }

    if (targetIRType && targetIRType->isFloatingPoint()) {
        std::string getReg = m_currentFunction->nextTempName("arr_elem");
        auto getCall = std::make_shared<CallInst>(
            IRType::getDouble(),
            "sp_rt_array_get_f64",
            std::vector<IRValuePtr>{objVal, idxVal},
            getReg
        );
        m_currentBlock->addInstruction(getCall);
        m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
        m_lastExprType = targetIRType;
    } else {
        std::string getReg = m_currentFunction->nextTempName("arr_elem");
        auto getCall = std::make_shared<CallInst>(
            IRType::getInt64(),
            "sp_rt_array_get_i64",
            std::vector<IRValuePtr>{objVal, idxVal},
            getReg
        );
        m_currentBlock->addInstruction(getCall);
        m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
        m_lastExprType = targetIRType;
    }
}

void IRBuilder::visit(const ast::ArrayExpr& node) {
    size_t count = node.getElements().size();
    auto countVal = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(count));
    auto elemSizeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 8);

    std::string arrReg = m_currentFunction->nextTempName("arr");
    auto allocCall = std::make_shared<CallInst>(
        IRType::getPointer(),
        "sp_rt_array_alloc",
        std::vector<IRValuePtr>{countVal, elemSizeVal},
        arrReg
    );
    m_currentBlock->addInstruction(allocCall);

    for (size_t i = 0; i < count; ++i) {
        node.getElements()[i]->accept(*this);
        IRValuePtr elemVal = m_lastExprValue;
        auto idxVal = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(i));

        if (m_lastExprType && m_lastExprType->isFloatingPoint()) {
            if (m_lastExprType->isFloat()) {
                elemVal = emitCastIfNeeded(elemVal, IRType::getDouble());
            }
            auto setCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_array_set_f64",
                std::vector<IRValuePtr>{allocCall, idxVal, elemVal}
            );
            m_currentBlock->addInstruction(setCall);
        } else {
            elemVal = emitCastIfNeeded(elemVal, IRType::getInt64());
            auto setCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_array_set_i64",
                std::vector<IRValuePtr>{allocCall, idxVal, elemVal}
            );
            m_currentBlock->addInstruction(setCall);
        }
    }

    m_lastExprValue = allocCall;
    m_lastExprType = IRType::getPointer();
}
void IRBuilder::visit(const ast::MapExpr& node) {
    // Phase 3 slice B (pro-4.0.0): real map literals. A literal lowers
    // to sp_rt_map_alloc followed by one sp_rt_map_set_str / set_i64
    // per entry, matching the existing map index/store lowering.
    //
    // TYPING RULES (the runtime stores int64 values only):
    // - keys: string (set_str) or integer/char/bool (set_i64, cast to
    //   i64). Any other key type is a loud error[E1000].
    // - values: integer/char/bool only (cast to i64). String, float,
    //   struct, array, map or Result/Option values have no runtime
    //   representation and fail loudly instead of being silently
    //   dropped or truncated.
    // - an empty literal allocates an empty map (sema types it
    //   Map<string, Any>).
    auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_map_alloc",
                                                std::vector<IRValuePtr>{},
                                                m_currentFunction->nextTempName("map_alloc"));
    m_currentBlock->addInstruction(allocCall);
    IRValuePtr mapVal = allocCall;

    for (const auto& entry : node.getEntries()) {
        if (!entry.first || !entry.second) continue;
        auto keyType = m_sema.getNodeType(entry.first.get());
        auto valueType = m_sema.getNodeType(entry.second.get());

        bool keyIsString = keyType && (keyType->isString() || keyType->isPointer());
        bool keyIsInt = keyType && (keyType->isInteger() || keyType->isChar() || keyType->isBoolean() ||
                                    keyType->isUnsignedInteger());
        if (!keyIsString && !keyIsInt && keyType && !keyType->isError()) {
            reportUnsupported(entry.first->getLocation(), "map literal with '" + keyType->toString() + "' key",
                              "map keys must be strings or integers in this phase");
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }
        bool valueIsInt = valueType && (valueType->isInteger() || valueType->isChar() || valueType->isBoolean() ||
                                        valueType->isUnsignedInteger());
        if (!valueIsInt && valueType && !valueType->isError()) {
            reportUnsupported(entry.second->getLocation(),
                              "map literal with '" + valueType->toString() + "' value",
                              "map values must be integers in this phase; the map runtime stores int64 values only");
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }

        entry.first->accept(*this);
        IRValuePtr keyVal = m_lastExprValue;
        entry.second->accept(*this);
        IRValuePtr valueVal = m_lastExprValue;
        if (!keyVal || !valueVal) {
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }
        IRValuePtr valueI64 = emitCastIfNeeded(valueVal, IRType::getInt64());
        if (keyIsString) {
            auto setCall = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_map_set_str",
                                                      std::vector<IRValuePtr>{mapVal, keyVal, valueI64});
            m_currentBlock->addInstruction(setCall);
        } else {
            IRValuePtr keyI64 = emitCastIfNeeded(keyVal, IRType::getInt64());
            auto setCall = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_map_set_i64",
                                                      std::vector<IRValuePtr>{mapVal, keyI64, valueI64});
            m_currentBlock->addInstruction(setCall);
        }
    }

    m_lastExprValue = mapVal;
    auto mapSemaType = m_sema.getNodeType(&node);
    m_lastExprType = mapSemaType ? mapType(mapSemaType.get()) : IRType::getPointer();
}
void IRBuilder::visit(const ast::LambdaExpr& node) {
    std::string lambdaName = "__sp_lambda_" + std::to_string(m_lambdaCounter++);

    // Determine return type
    auto semaType = m_sema.getNodeType(&node);
    IRTypePtr retIRType = nullptr;
    if (semaType && semaType->isFunction()) {
        auto fnType = std::dynamic_pointer_cast<sema::FunctionType>(semaType);
        if (fnType && fnType->getReturnType() && !fnType->getReturnType()->isUnknown()) {
            retIRType = mapType(fnType->getReturnType().get());
        }
    } else if (!node.getReturnType().empty()) {
        retIRType = mapType(node.getReturnType());
    }

    if (!retIRType) {
        if (const auto* exprBody = dynamic_cast<const ast::ExpressionNode*>(&node.getBody())) {
            auto bodyType = m_sema.getNodeType(exprBody);
            if (bodyType) retIRType = mapType(bodyType.get());
        } else if (const auto* blockBody = dynamic_cast<const ast::BlockStmt*>(&node.getBody())) {
            for (const auto& stmt : blockBody->getStatements()) {
                if (const auto* retStmt = dynamic_cast<const ast::ReturnStmt*>(stmt.get())) {
                    if (retStmt->hasValue()) {
                        auto valType = m_sema.getNodeType(retStmt->getValue());
                        if (valType) {
                            retIRType = mapType(valType.get());
                            break;
                        }
                    }
                }
            }
        }
    }
    if (!retIRType) {
        retIRType = IRType::getInt64();
    }

    // Determine parameter types
    std::vector<std::shared_ptr<ParameterValue>> lambdaParams;
    for (size_t i = 0; i < node.getParams().size(); ++i) {
        const auto& p = node.getParams()[i];
        IRTypePtr pType = IRType::getInt64();
        if (!p.type.empty()) {
            pType = mapType(p.type);
        } else if (semaType && semaType->isFunction()) {
            auto fnType = std::dynamic_pointer_cast<sema::FunctionType>(semaType);
            if (fnType && i < fnType->getParamTypes().size() && fnType->getParamTypes()[i]) {
                pType = mapType(fnType->getParamTypes()[i].get());
            }
        }
        lambdaParams.push_back(std::make_shared<ParameterValue>(pType, "%" + p.name, i));
    }

    // Save previous function context and scope stack
    auto prevFunc = m_currentFunction;
    auto prevBlock = m_currentBlock;
    // Phase 3 slice A: snapshot the enclosing variable names so a
    // reference to one from the lambda body fails loudly (capturing
    // lambdas are not lowered). Each snapshot holds ALL enclosing
    // variables, so nested lambdas only need the innermost frame.
    {
        std::unordered_set<std::string> outerVars;
        for (const auto& scope : m_scopes) {
            for (const auto& [name, var] : scope) {
                (void)var;
                outerVars.insert(name);
            }
        }
        m_lambdaOuterVars.push_back(std::move(outerVars));
    }
    auto prevScopes = std::move(m_scopes);
    auto prevRefCountedScopes = std::move(m_refCountedScopes);
    m_scopes.clear();
    m_refCountedScopes.clear();

    auto lambdaFunc = m_module->createFunction(lambdaName, retIRType, lambdaParams);
    m_currentFunction = lambdaFunc;
    auto entryBB = lambdaFunc->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();

    // Stack allocas for parameters
    for (size_t i = 0; i < node.getParams().size(); ++i) {
        const auto& p = node.getParams()[i];
        const auto& paramVal = lambdaParams[i];
        auto allocaInst = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.name);
        entryBB->addInstruction(allocaInst);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaInst));
        declareVariable(p.name, allocaInst, paramVal->getType());
    }

    // Phase 5 slice A: a lambda is a separate function, not part of
    // the enclosing async fn's state machine — clear the SM state and
    // the async flag while lowering the body, so an `await` inside a
    // lambda fails loudly with the E1000 backstop (lambdas cannot be
    // async; sema's lambda visitor does not reset m_inAsyncContext).
    // The lambda's own locals use ordinary entry-block allocas.
    auto savedAsyncSM = std::move(m_asyncSM);
    bool savedInAsyncFunction = m_inAsyncFunction;
    m_inAsyncFunction = false;

    // Lower body
    if (const auto* exprBody = dynamic_cast<const ast::ExpressionNode*>(&node.getBody())) {
        exprBody->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        if (retVal && !retIRType->isVoid()) {
            retVal = emitCastIfNeeded(retVal, retIRType);
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        }
    } else if (const auto* blockBody = dynamic_cast<const ast::BlockStmt*>(&node.getBody())) {
        for (const auto& stmt : blockBody->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
        if (!m_currentBlock->hasTerminator()) {
            if (retIRType->isVoid()) {
                m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
            } else {
                m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                    std::make_shared<ConstantInt>(retIRType, 0)));
            }
        }
    }

    m_asyncSM = std::move(savedAsyncSM);
    m_inAsyncFunction = savedInAsyncFunction;
    popScope();
    m_scopes = std::move(prevScopes);
    m_refCountedScopes = std::move(prevRefCountedScopes);
    m_currentFunction = prevFunc;
    m_currentBlock = prevBlock;
    m_lambdaOuterVars.pop_back();

    auto fnPtr = std::make_shared<GlobalVariable>(IRType::getInt8(), "@" + lambdaName);
    m_lastExprValue = fnPtr;
    m_lastExprType = IRType::getPointer();
}
void IRBuilder::visit(const ast::NewExpr& node) {
    std::string typeName = node.getTypeName();
    if (typeName == "List") {
        auto elemSizeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 8);
        auto initCapVal = std::make_shared<ConstantInt>(IRType::getInt64(), 16);
        std::string listReg = m_currentFunction->nextTempName("list");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_list_alloc",
            std::vector<IRValuePtr>{elemSizeVal, initCapVal},
            listReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Map") {
        std::string mapReg = m_currentFunction->nextTempName("map");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_map_alloc",
            std::vector<IRValuePtr>{},
            mapReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Set") {
        std::string setReg = m_currentFunction->nextTempName("set");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_set_alloc",
            std::vector<IRValuePtr>{},
            setReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Stack") {
        std::string reg = m_currentFunction->nextTempName("stack");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_stack_alloc",
            std::vector<IRValuePtr>{},
            reg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Queue") {
        std::string reg = m_currentFunction->nextTempName("queue");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_queue_alloc",
            std::vector<IRValuePtr>{},
            reg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "PriorityQueue") {
        std::string reg = m_currentFunction->nextTempName("pq");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_pq_alloc",
            std::vector<IRValuePtr>{},
            reg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Duration") {
        std::string durReg = m_currentFunction->nextTempName("dur");
        auto zeroVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_duration_from_millis",
            std::vector<IRValuePtr>{zeroVal},
            durReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Arena") {
        IRValuePtr capVal = nullptr;
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            capVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        } else {
            capVal = std::make_shared<ConstantInt>(IRType::getInt64(), 65536);
        }
        std::string arenaReg = m_currentFunction->nextTempName("arena");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_arena_create",
            std::vector<IRValuePtr>{capVal},
            arenaReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Mutex") {
        std::string reg = m_currentFunction->nextTempName("mtx");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_mutex_create", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "RwLock") {
        std::string reg = m_currentFunction->nextTempName("rw");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_rwlock_create", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Semaphore") {
        IRValuePtr countVal = nullptr;
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            countVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        } else {
            countVal = std::make_shared<ConstantInt>(IRType::getInt64(), 1);
        }
        std::string reg = m_currentFunction->nextTempName("sem");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_semaphore_create", std::vector<IRValuePtr>{countVal}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "ConditionVariable") {
        std::string reg = m_currentFunction->nextTempName("cv");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_condvar_create", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "AtomicInt") {
        IRValuePtr initVal = nullptr;
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            initVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        } else {
            initVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
        }
        std::string reg = m_currentFunction->nextTempName("atom");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_atomic_int_create", std::vector<IRValuePtr>{initVal}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "AtomicBool") {
        IRValuePtr initVal = nullptr;
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            initVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt1());
        } else {
            initVal = std::make_shared<ConstantInt>(IRType::getInt1(), 0);
        }
        std::string reg = m_currentFunction->nextTempName("atom");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_atomic_bool_create", std::vector<IRValuePtr>{initVal}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Channel" || node.getTypeName() == "Channel") {
        IRValuePtr capVal = nullptr;
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            capVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        } else {
            capVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
        }
        std::string reg = m_currentFunction->nextTempName("chan");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_channel_create", std::vector<IRValuePtr>{capVal}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "ThreadPool") {
        IRValuePtr threadsVal = nullptr;
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            threadsVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        } else {
            threadsVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
        }
        std::string reg = m_currentFunction->nextTempName("tpool");
        auto allocCall = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_threadpool_create", std::vector<IRValuePtr>{threadsVal}, reg);
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (node.isGeneric()) {
        typeName = node.getTypeName();
        for (const auto& arg : node.getTypeArgs()) {
            typeName += "_" + arg;
        }
    }

    const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(typeName) : nullptr;
    if (!layout) {
        m_lastExprValue = std::make_shared<ConstantNull>();
        m_lastExprType = IRType::getPointer();
        return;
    }

    // 1. Allocate object on heap via sp_rt_arc_alloc
    uint64_t size = layout->objectSize > 0 ? layout->objectSize : 8;
    auto sizeVal = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(size));

    std::string effectiveName = layout ? layout->name : typeName;
    std::string dtorName = effectiveName + "_destructor";
    IRValuePtr dtorPtr = nullptr;
    if (m_module->getFunction(dtorName)) {
        dtorPtr = std::make_shared<GlobalVariable>(IRType::getInt8(), "@" + dtorName);
    } else {
        dtorPtr = std::make_shared<ConstantNull>();
    }

    std::string rawPtrReg = m_currentFunction->nextTempName("obj_arc");
    auto allocCall = std::make_shared<CallInst>(
        IRType::getPointer(),
        "sp_rt_arc_alloc",
        std::vector<IRValuePtr>{sizeVal, dtorPtr},
        rawPtrReg
    );
    m_currentBlock->addInstruction(allocCall);

    // 2. Set vtable pointer in slot 0
    if (layout->hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout->vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout->structType,
            allocCall,
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            m_currentFunction->nextTempName("vptr")
        );
        m_currentBlock->addInstruction(gep);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    // 3. Call constructor @ClassName_ClassName(ptr %raw, args...)
    std::string ctorName = effectiveName + "_" + effectiveName;
    auto* ctorFunc = m_module->getFunction(ctorName);

    std::vector<IRValuePtr> loweredArgs;
    loweredArgs.push_back(allocCall);
    for (size_t i = 0; i < node.getArgs().size(); ++i) {
        node.getArgs()[i]->accept(*this);
        IRValuePtr argVal = m_lastExprValue;
        if (ctorFunc && i + 1 < ctorFunc->getParams().size()) {
            argVal = emitCastIfNeeded(argVal, ctorFunc->getParams()[i + 1]->getType());
        }
        loweredArgs.push_back(argVal);
    }

    auto ctorCall = std::make_shared<CallInst>(IRType::getVoid(), ctorName, loweredArgs);
    m_currentBlock->addInstruction(ctorCall);

    m_lastExprValue = allocCall;
    m_lastExprType = IRType::getPointer();
}

void IRBuilder::visit(const ast::TryExpr& node) {
    node.getExpr().accept(*this);
    IRValuePtr operandVal = m_lastExprValue;
    if (!operandVal) {
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getInt32();
        return;
    }

    auto semaOpType = m_sema.getNodeType(&node.getExpr());
    std::string opTypeName = semaOpType ? semaOpType->toString() : "";
    std::string tuBaseName = (opTypeName.rfind("Option", 0) == 0) ? "Option" : "Result";
    auto structType = ir::IRType::getStruct("struct." + tuBaseName, {IRType::getInt32(), IRType::getInt64()});

    IRValuePtr objPtr = operandVal;
    if (operandVal->getType()->isStruct()) {
        auto tempAlloca = std::make_shared<AllocaInst>(operandVal->getType(), m_currentFunction->nextTempName("try_tmp"));
        m_currentFunction->getEntryBlock()->addInstruction(tempAlloca);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(operandVal, tempAlloca));
        objPtr = tempAlloca;
    }

    // 1. Load tag (field 0)
    auto gepTag = std::make_shared<GetElementPtrInst>(
        structType,
        objPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
        },
        m_currentFunction->nextTempName("try_tag_ptr")
    );
    m_currentBlock->addInstruction(gepTag);
    auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("try_tag"));
    m_currentBlock->addInstruction(loadTag);

    // 2. Load payload (field 1)
    auto gepPayload = std::make_shared<GetElementPtrInst>(
        structType,
        objPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 1)
        },
        m_currentFunction->nextTempName("try_payload_ptr")
    );
    m_currentBlock->addInstruction(gepPayload);
    auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("try_payload"));
    m_currentBlock->addInstruction(loadPayload);

    // 3. Compare tag != 0 (error / none)
    auto cmpErr = std::make_shared<CompareInst>(IROpcode::ICmpNe, loadTag,
        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
        m_currentFunction->nextTempName("is_err"));
    m_currentBlock->addInstruction(cmpErr);

    auto errBB = createBlock("try_err");
    auto okBB = createBlock("try_ok");
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(cmpErr, errBB->getName(), okBB->getName()));

    // 4. Error block: early return!
    setInsertPoint(errBB);
    emitAllFunctionDrops();
    IRTypePtr retType = m_currentFunction->getReturnType();
    if (retType->isStruct()) {
        auto earlyRetVal = std::make_shared<LoadInst>(
            retType,
            objPtr,
            m_currentFunction->nextTempName("early_ret")
        );
        errBB->addInstruction(earlyRetVal);
        errBB->addInstruction(std::make_shared<ReturnInst>(earlyRetVal));
    } else if (retType->isPointer()) {
        errBB->addInstruction(std::make_shared<ReturnInst>(objPtr));
    } else {
        errBB->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(retType)));
    }

    // 5. Ok block: unwrapped payload
    setInsertPoint(okBB);
    auto semaNodeType = m_sema.getNodeType(&node);
    IRTypePtr targetType = semaNodeType ? mapType(semaNodeType.get()) : IRType::getInt64();
    auto unwrapped = emitCastIfNeeded(loadPayload, targetType);

    m_lastExprValue = unwrapped;
    m_lastExprType = targetType;
}

// --- Phase 5 slice A: async spawn / await helpers (state machine) --------
// See the header for the contract. The spawn model: calling an async fn
// returns a Task immediately; the body runs as a posted callback on the
// scheduler. The body itself is the entry point — `void <name>(ptr
// %task, ptr %ctx)` — it allocates a heap frame, stores task + args,
// and calls the resume function `void __sp_async_resume_<name>(ptr
// %frame)`, which holds the dispatcher + one segment per await point.
// Awaiting suspends: the segment stores its state, registers the resume
// as the awaited task's continuation (posted, never synchronous), and
// returns; the dispatcher re-enters at the recorded state. This
// REPLACES the Phase 3 thunk model (no __sp_async_thunk_* functions,
// no vtable change, no frame-size-at-call-site problem — see the
// Phase 34 slice A doc for the Design 2 rationale).

std::string IRBuilder::asyncResumeName(const std::string& compiledName) const {
    return "__sp_async_resume_" + compiledName;
}

// Inner type string of Task<T>/Future<T> (e.g. "int" for Task<int>),
// or "" when the type is not a Task/Future or is nested.
static std::string taskInnerType(const sema::TypePtr& taskType) {
    if (!taskType) return "";
    std::string name = taskType->toString();
    std::string inner;
    if (name.rfind("Task<", 0) == 0 && name.size() > 6 && name.back() == '>') {
        inner = name.substr(5, name.size() - 6);
    } else if (name.rfind("Future<", 0) == 0 && name.size() > 8 && name.back() == '>') {
        inner = name.substr(7, name.size() - 8);
    } else {
        return "";
    }
    // Nested Task<...> / parameterized payloads are not carried.
    if (inner.find('<') != std::string::npos) return "";
    return inner;
}

bool IRBuilder::taskPayloadKind(const sema::TypePtr& taskType, std::string& outKind) const {
    std::string inner = taskInnerType(taskType);
    if (inner.empty()) return false;
    if (inner == "void") { outKind = "void"; return true; }
    if (inner == "int" || inner == "int8" || inner == "int16" || inner == "int32" || inner == "int64") {
        outKind = "int"; return true;
    }
    if (inner == "bool") { outKind = "bool"; return true; }
    if (inner == "string") { outKind = "string"; return true; }
    // Phase 5 slice A: double / array / struct payloads.
    if (inner == "double" || inner == "float" || inner == "float32" || inner == "float64") {
        outKind = "double"; return true;
    }
    if (inner.size() > 2 && inner.substr(inner.size() - 2) == "[]") {
        outKind = "array"; return true;
    }
    // A user struct type (value type). Classes are reference types
    // with ARC ownership, which cannot cross the task boundary, so a
    // class payload is NOT carried (fails loudly at the declaration).
    if (auto userType = m_sema.getTypeContext().findUserType(inner)) {
        if (userType->isStruct()) { outKind = "struct"; return true; }
    }
    return false;
}

bool IRBuilder::asyncReturnKind(const sema::TypePtr& returnType, std::string& outKind) const {
    if (!returnType) return false;
    std::string name = returnType->toString();
    if (name == "void") { outKind = "void"; return true; }
    if (name == "int" || name == "int8" || name == "int16" || name == "int32" || name == "int64") {
        outKind = "int"; return true;
    }
    if (name == "bool") { outKind = "bool"; return true; }
    if (name == "string") { outKind = "string"; return true; }
    // Phase 5 slice A: double / array / struct payloads.
    if (name == "double" || name == "float" || name == "float32" || name == "float64") {
        outKind = "double"; return true;
    }
    if (name.size() > 2 && name.substr(name.size() - 2) == "[]") {
        outKind = "array"; return true;
    }
    // A user struct type (value type). Classes are reference types
    // with ARC ownership, which cannot cross the task boundary, so a
    // class return is NOT carried (fails loudly at the declaration).
    if (auto userType = m_sema.getTypeContext().findUserType(name)) {
        if (userType->isStruct()) { outKind = "struct"; return true; }
    }
    return false;
}

IRValuePtr IRBuilder::emitAsyncScheduler() {
    auto schedCall = std::make_shared<CallInst>(
        IRType::getPointer(), "sp_rt_async_default_scheduler",
        std::vector<IRValuePtr>{},
        m_currentFunction->nextTempName("async_sched"));
    m_currentBlock->addInstruction(schedCall);
    return schedCall;
}

IRValuePtr IRBuilder::emitAsyncSpawnSM(IRValuePtr thunk,
                                        const std::vector<IRValuePtr>& argValues,
                                        const std::vector<IRTypePtr>& argTypes) {
    // Validate first: no IR is emitted when an argument cannot be carried.
    if (!thunk || !thunk->getType()->isPointer()) return nullptr;
    if (argValues.size() != argTypes.size()) return nullptr;
    std::vector<IRTypePtr> storageTypes;
    for (size_t i = 0; i < argTypes.size(); ++i) {
        if (!argValues[i] || !argTypes[i]) return nullptr;
        if (argTypes[i]->isInteger()) storageTypes.push_back(IRType::getInt64());
        else if (argTypes[i]->isPointer()) storageTypes.push_back(IRType::getPointer());
        // Phase 5 slice A: isFloatingPoint() covers BOTH float and
        // double — isFloat() is 32-bit only, so a `double` argument
        // was wrongly rejected here (and silently skipped in the body
        // prologue, see asyncSMEnd). Both widen to double storage.
        else if (argTypes[i]->isFloatingPoint()) storageTypes.push_back(IRType::getDouble());
        else return nullptr; // struct (by value) cannot be carried in an 8-byte ctx slot
    }

    auto sched = emitAsyncScheduler();
    auto taskCall = std::make_shared<CallInst>(
        IRType::getPointer(), "sp_rt_async_create_task",
        std::vector<IRValuePtr>{},
        m_currentFunction->nextTempName("async_task"));
    m_currentBlock->addInstruction(taskCall);

    // Ctx layout: [arg0, arg1, ...] in 8-byte slots. The body entry
    // (void (ptr task, ptr ctx)) loads the args from here into the
    // heap frame and frees the ctx; the task is NOT completed here —
    // the body completes it in its finish block.
    int64_t ctxSize = static_cast<int64_t>(argValues.size()) * 8;
    if (ctxSize < 8) ctxSize = 8;
    auto allocCall = std::make_shared<CallInst>(
        IRType::getPointer(), "sp_rt_alloc",
        std::vector<IRValuePtr>{std::make_shared<ConstantInt>(IRType::getInt64(), ctxSize)},
        m_currentFunction->nextTempName("async_ctx"));
    m_currentBlock->addInstruction(allocCall);

    auto storeSlot = [&](size_t slot, IRValuePtr value) {
        auto gep = std::make_shared<GetElementPtrInst>(
            IRType::getInt8(), allocCall,
            std::vector<IRValuePtr>{std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(slot * 8))},
            m_currentFunction->nextTempName("ctx_slot"));
        m_currentBlock->addInstruction(gep);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(value, gep));
    };
    for (size_t i = 0; i < argValues.size(); ++i) {
        IRValuePtr stored = emitCastIfNeeded(argValues[i], storageTypes[i]);
        storeSlot(i, stored);
    }

    m_currentBlock->addInstruction(std::make_shared<CallInst>(
        IRType::getVoid(), "sp_rt_async_spawn_sm",
        std::vector<IRValuePtr>{sched, taskCall, thunk, allocCall}));
    return taskCall;
}

IRValuePtr IRBuilder::emitStructValueFromBox(IRValuePtr box, IRTypePtr structTy) {
    // A cancelled task's box is null; loading a struct through null
    // would segfault, so branch: null -> zero value, else load the box.
    // The temp alloca lives in the entry block (dominates every block);
    // the value is stored in either branch and loaded after the merge.
    auto* entryBB = m_currentFunction->getEntryBlock();
    auto tmp = std::make_shared<AllocaInst>(structTy, m_currentFunction->nextTempName("await_struct_tmp"));
    entryBB->addInstruction(tmp);
    auto isNull = std::make_shared<CompareInst>(
        IROpcode::ICmpEq, box, std::make_shared<ConstantNull>(IRType::getPointer()),
        m_currentFunction->nextTempName("await_box_null"));
    m_currentBlock->addInstruction(isNull);
    IRBasicBlock* defBB = createBlock("await_struct_default");
    IRBasicBlock* loadBB = createBlock("await_struct_load");
    IRBasicBlock* mergeBB = createBlock("await_struct_merge");
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(
        isNull, defBB->getName(), loadBB->getName()));
    setInsertPoint(defBB);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantNull>(structTy), tmp));
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
    setInsertPoint(loadBB);
    auto loaded = std::make_shared<LoadInst>(structTy, box, m_currentFunction->nextTempName("await_struct_val"));
    m_currentBlock->addInstruction(loaded);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(loaded, tmp));
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
    setInsertPoint(mergeBB);
    auto result = std::make_shared<LoadInst>(structTy, tmp, m_currentFunction->nextTempName("await_struct"));
    m_currentBlock->addInstruction(result);
    return result;
}

IRValuePtr IRBuilder::emitAwaitTask(IRValuePtr sched, IRValuePtr task, const std::string& payloadKind,
                                    IRTypePtr structTy) {
    if (payloadKind == "void") {
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_await_void",
            std::vector<IRValuePtr>{sched, task}));
        return nullptr;
    }
    if (payloadKind == "int" || payloadKind == "bool") {
        auto awaitCall = std::make_shared<CallInst>(
            IRType::getInt64(), "sp_rt_async_task_await_i64",
            std::vector<IRValuePtr>{sched, task},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(awaitCall);
        if (payloadKind == "bool") return emitCastIfNeeded(awaitCall, IRType::getInt1());
        return awaitCall;
    }
    if (payloadKind == "string") {
        auto awaitCall = std::make_shared<CallInst>(
            IRType::getPointer(), "sp_rt_async_task_await_str",
            std::vector<IRValuePtr>{sched, task},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(awaitCall);
        return awaitCall;
    }
    // Phase 5 slice A: double / array / struct payloads (pump path).
    if (payloadKind == "double") {
        auto awaitCall = std::make_shared<CallInst>(
            IRType::getDouble(), "sp_rt_async_task_await_dbl",
            std::vector<IRValuePtr>{sched, task},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(awaitCall);
        return awaitCall;
    }
    if (payloadKind == "array") {
        // The pump await for a pointer payload returns the array
        // pointer directly (arrays are plain malloc, never owned by
        // the task, so the pointer outlives the task by design).
        auto awaitCall = std::make_shared<CallInst>(
            IRType::getPointer(), "sp_rt_async_task_await_ptr",
            std::vector<IRValuePtr>{sched, task},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(awaitCall);
        return awaitCall;
    }
    if (payloadKind == "struct" && structTy) {
        auto awaitCall = std::make_shared<CallInst>(
            IRType::getPointer(), "sp_rt_async_task_await_ptr",
            std::vector<IRValuePtr>{sched, task},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(awaitCall);
        return emitStructValueFromBox(awaitCall, structTy);
    }
    return nullptr;
}

void IRBuilder::emitMainExitPump() {
    auto sched = emitAsyncScheduler();
    m_currentBlock->addInstruction(std::make_shared<CallInst>(
        IRType::getVoid(), "sp_rt_async_scheduler_run_tasks_until_idle",
        std::vector<IRValuePtr>{sched}));
}

// --- Phase 5 slice A: heap-frame state machine ----------------------------
// See IRBuilder.h (AsyncSM) and docs/v2/PHASE-34-SLICE-A.md. The body
// entry `void <name>(ptr %task, ptr %ctx)` allocates a heap frame,
// stores task + args, and calls the resume function
// `void __sp_async_resume_<name>(ptr %frame)`, which holds the
// dispatcher + one segment per await point. Locals live in frame
// slots (8 bytes each), so they survive across await points; the
// stack unwinds at every suspend (true suspension / flat stack).

std::vector<IRTypePtr> IRBuilder::asyncSpawnParamTypes(
    const std::vector<sema::TypePtr>& semaParamTypes,
    const sema::TypePtr& thisType) const {
    std::vector<IRTypePtr> out;
    if (thisType) out.push_back(IRType::getPointer()); // `this`
    for (const auto& t : semaParamTypes) {
        if (!t) { out.push_back(nullptr); continue; }
        out.push_back(mapType(t.get()));
    }
    return out;
}

int64_t IRBuilder::asyncStructSizeBytes(const IRTypePtr& structTy) {
    if (!structTy || !structTy->isStruct()) return 0;
    auto structType = std::dynamic_pointer_cast<IRStructType>(structTy);
    if (!structType) return 0;
    // x86-64 layout over the element types. Each field is aligned to
    // its own ABI alignment (capped at 8); the struct size is rounded
    // up to the struct's alignment. This is an upper bound on LLVM's
    // layout for the same struct type (nested structs use alignment 8
    // here, which can only over-allocate) — the size is used only for
    // the heap box allocation, where over-allocating is safe.
    int64_t offset = 0;
    int64_t structAlign = 1;
    for (const auto& el : structType->getElementTypes()) {
        int64_t fieldSize = 8;
        int64_t fieldAlign = 8;
        if (el->isInt1() || el->isInt8()) { fieldSize = 1; fieldAlign = 1; }
        else if (el->isInt16()) { fieldSize = 2; fieldAlign = 2; }
        else if (el->isInt32() || el->isFloat()) { fieldSize = 4; fieldAlign = 4; }
        else if (el->isInt64() || el->isDouble() || el->isPointer()) { fieldSize = 8; fieldAlign = 8; }
        else if (el->isStruct()) { fieldSize = asyncStructSizeBytes(el); fieldAlign = 8; }
        if (fieldAlign > structAlign) structAlign = fieldAlign;
        offset = (offset + fieldAlign - 1) & ~(fieldAlign - 1);
        offset += fieldSize;
    }
    offset = (offset + structAlign - 1) & ~(structAlign - 1);
    return offset;
}

IRValuePtr IRBuilder::asyncFrameSlotGEP(int64_t slot) {
    auto it = m_asyncSM->slotGEPs.find(slot);
    if (it != m_asyncSM->slotGEPs.end()) return it->second;
    // GEPs are created in the dispatcher (entry) block so they
    // dominate every segment; addInstruction inserts before the
    // terminator when one exists, so GEPs land before the dispatcher
    // chain (which is built at the end).
    auto gep = std::make_shared<GetElementPtrInst>(
        IRType::getInt8(), m_asyncSM->frameParam,
        std::vector<IRValuePtr>{std::make_shared<ConstantInt>(
            IRType::getInt64(), asyncFrameOffset(slot))},
        m_currentFunction->nextTempName("frame_slot"));
    m_asyncSM->dispatcherBB->addInstruction(gep);
    m_asyncSM->slotGEPs[slot] = gep;
    return gep;
}

IRValuePtr IRBuilder::asyncFrameLoad(int64_t slot, IRTypePtr ty, const std::string& name) {
    auto gep = asyncFrameSlotGEP(slot);
    auto load = std::make_shared<LoadInst>(ty, gep, m_currentFunction->nextTempName(name));
    m_currentBlock->addInstruction(load);
    return load;
}

void IRBuilder::asyncFrameStore(int64_t slot, IRValuePtr val) {
    auto gep = asyncFrameSlotGEP(slot);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(val, gep));
}

int64_t IRBuilder::asyncAssignLocalSlot(const void* node, int64_t slotCount) {
    auto it = m_asyncSM->nodeSlots.find(node);
    if (it != m_asyncSM->nodeSlots.end()) return it->second;
    if (slotCount < 1) slotCount = 1;
    int64_t slot = m_asyncSM->nextSlot;
    m_asyncSM->nextSlot += slotCount;
    m_asyncSM->nodeSlots[node] = slot;
    return slot;
}

int64_t IRBuilder::asyncAuxSlot(const void* node, int64_t tag, int64_t slotCount) {
    auto key = std::make_pair(node, tag);
    auto it = m_asyncSM->auxSlots.find(key);
    if (it != m_asyncSM->auxSlots.end()) return it->second;
    if (slotCount < 1) slotCount = 1;
    int64_t slot = m_asyncSM->nextSlot;
    m_asyncSM->nextSlot += slotCount;
    m_asyncSM->auxSlots[key] = slot;
    return slot;
}

IRValuePtr IRBuilder::asyncNodeSlotGEP(const void* node, IRTypePtr fallbackType,
                                       const diagnostics::SourceLocation& loc) {
    auto it = m_asyncSM->nodeSlots.find(node);
    if (it != m_asyncSM->nodeSlots.end()) return asyncFrameSlotGEP(it->second);
    reportUnsupported(loc, "loop variable",
        "internal error: an async loop variable was not assigned a frame slot by the pre-scan");
    auto alloca = std::make_shared<AllocaInst>(fallbackType, m_currentFunction->nextTempName("async_fallback"));
    m_currentFunction->getEntryBlock()->addInstruction(alloca);
    return alloca;
}

sema::TypePtr IRBuilder::asyncResolveType(const std::string& typeStr) {
    auto& typeCtx = const_cast<sema::TypeContext&>(m_sema.getTypeContext());
    // NOTE: this must call TypeContext::resolveType (the sema method),
    // not asyncResolveType itself — a bulk rename once turned this
    // into infinite recursion.
    return typeCtx.resolveType(typeStr);
}

IRTypePtr IRBuilder::asyncStructTypeForTask(const sema::TypePtr& taskType) {
    std::string inner = taskInnerType(taskType);
    if (inner.empty()) return nullptr;
    auto innerSema = asyncResolveType(inner);
    if (!innerSema) return nullptr;
    return asyncStructTypeFor(innerSema);
}

IRTypePtr IRBuilder::asyncAwaitedIRType(const std::string& kind, IRTypePtr structTy) {
    if (kind == "void") return IRType::getVoid();
    if (kind == "string") return IRType::getPointer();
    if (kind == "bool") return IRType::getInt1();
    if (kind == "double") return IRType::getDouble();
    if (kind == "array") return IRType::getPointer();
    if (kind == "struct") return structTy;
    return IRType::getInt64();
}

IRTypePtr IRBuilder::asyncStructTypeFor(const sema::TypePtr& semaType) {
    if (!semaType) return nullptr;
    if (semaType->isStruct()) {
        if (m_vtableBuilder) {
            if (const auto* layout = m_vtableBuilder->getClassLayout(semaType->toString())) {
                if (layout->structType) return layout->structType;
            }
        }
        return nullptr;
    }
    if (semaType->isEnum()) {
        // Tagged-union enums (Result/Option) lower to a struct value.
        IRTypePtr mapped = mapType(semaType->toString());
        if (mapped && mapped->isStruct()) return mapped;
    }
    return nullptr;
}

int64_t IRBuilder::asyncSlotCountForType(const sema::TypePtr& semaType) {
    if (auto st = asyncStructTypeFor(semaType)) {
        int64_t bytes = asyncStructSizeBytes(st);
        return (bytes + 7) / 8;
    }
    return 1;
}

int64_t IRBuilder::asyncVarDeclSlotCount(const ast::VarDeclStmt& node) {
    // Declared type wins (mirrors visit(VarDeclStmt)'s valType).
    const std::string& typeName = node.getType();
    if (!typeName.empty() && typeName != "var" && typeName != "let" && typeName != "const") {
        auto ty = asyncResolveType(typeName);
        return asyncSlotCountForType(ty);
    }
    // Inferred: from the initializer's sema type.
    if (node.hasInitializer()) {
        auto initTy = m_sema.getNodeType(node.getInitializer());
        return asyncSlotCountForType(initTy);
    }
    return 1;
}

int64_t IRBuilder::asyncForInSlotCount(const ast::ForInStmt& node) {
    // Declared loop-var type wins.
    const std::string& typeName = node.getType();
    if (!typeName.empty() && typeName != "var" && typeName != "let" && typeName != "const") {
        auto ty = asyncResolveType(typeName);
        return asyncSlotCountForType(ty);
    }
    // Inferred: the iterable's element type (arrays; `a..b` ranges
    // are int and take 1 slot).
    auto iterTy = m_sema.getNodeType(&node.getIterable());
    if (iterTy && iterTy->isArray()) {
        auto arrTy = std::dynamic_pointer_cast<sema::ArrayType>(iterTy);
        if (arrTy) return asyncSlotCountForType(arrTy->getElementType());
    }
    if (iterTy) {
        // List<T>: the element type is inside the type string.
        std::string name = iterTy->toString();
        if (name.rfind("List<", 0) == 0 && name.size() > 6 && name.back() == '>') {
            std::string inner = name.substr(5, name.size() - 6);
            auto elemTy = asyncResolveType(inner);
            return asyncSlotCountForType(elemTy);
        }
    }
    return 1;
}

// --- Await pre-scan: collect / first / hoist -------------------------------
// Children of an expression in evaluation order (const). Lambdas are
// NOT descended into (a lambda body is a separate function); parse
// error-recovery null children are skipped via the hasX() guards.
static void asyncChildExprsConst(const ast::ExpressionNode& e,
                                 std::vector<const ast::ExpressionNode*>& out) {
    if (auto* b = dynamic_cast<const ast::BinaryExpr*>(&e)) {
        out.push_back(&b->getLeft());
        if (b->hasRight()) out.push_back(&b->getRight());
    } else if (auto* u = dynamic_cast<const ast::UnaryExpr*>(&e)) {
        if (u->hasOperand()) out.push_back(&u->getOperand());
    } else if (auto* c = dynamic_cast<const ast::CallExpr*>(&e)) {
        out.push_back(&c->getCallee());
        for (const auto& a : c->getArgs()) if (a) out.push_back(a.get());
    } else if (auto* m = dynamic_cast<const ast::MemberExpr*>(&e)) {
        out.push_back(&m->getObject());
    } else if (auto* ix = dynamic_cast<const ast::IndexExpr*>(&e)) {
        out.push_back(&ix->getObject());
        if (ix->hasIndex()) out.push_back(&ix->getIndex());
    } else if (auto* arr = dynamic_cast<const ast::ArrayExpr*>(&e)) {
        for (const auto& el : arr->getElements()) if (el) out.push_back(el.get());
    } else if (auto* mp = dynamic_cast<const ast::MapExpr*>(&e)) {
        for (const auto& [k, v] : mp->getEntries()) {
            if (k) out.push_back(k.get());
            if (v) out.push_back(v.get());
        }
    } else if (auto* t = dynamic_cast<const ast::TernaryExpr*>(&e)) {
        out.push_back(&t->getCondition());
        if (t->hasThenExpr()) out.push_back(&t->getThenExpr());
        if (t->hasElseExpr()) out.push_back(&t->getElseExpr());
    } else if (auto* a = dynamic_cast<const ast::AssignExpr*>(&e)) {
        out.push_back(&a->getTarget());
        if (a->hasValue()) out.push_back(&a->getValue());
    } else if (auto* n = dynamic_cast<const ast::NewExpr*>(&e)) {
        for (const auto& arg : n->getArgs()) if (arg) out.push_back(arg.get());
    } else if (auto* tr = dynamic_cast<const ast::TryExpr*>(&e)) {
        out.push_back(&tr->getExpr());
    } else if (auto* aw = dynamic_cast<const ast::AwaitExpr*>(&e)) {
        out.push_back(&aw->getExpr());
    } else if (auto* ct = dynamic_cast<const ast::ComptimeExpr*>(&e)) {
        if (ct->hasExpr() && ct->getExpr()) out.push_back(ct->getExpr());
    }
    // LambdaExpr: not descended into. Literal/Identifier: no children.
}

// Mutable child slots of an expression in evaluation order (for the
// hoisting rewrite). Same traversal as asyncChildExprsConst.
static void asyncChildExprSlots(ast::ExpressionNode& e,
                                std::vector<ast::ExprPtr*>& out) {
    if (auto* b = dynamic_cast<ast::BinaryExpr*>(&e)) {
        out.push_back(&b->getLeftPtr());
        if (b->hasRight()) out.push_back(&b->getRightPtr());
    } else if (auto* u = dynamic_cast<ast::UnaryExpr*>(&e)) {
        if (u->hasOperand()) out.push_back(&u->getOperandPtr());
    } else if (auto* c = dynamic_cast<ast::CallExpr*>(&e)) {
        out.push_back(&c->getCalleePtr());
        for (auto& a : c->getArgs()) out.push_back(&a);
    } else if (auto* m = dynamic_cast<ast::MemberExpr*>(&e)) {
        out.push_back(&m->getObjectPtr());
    } else if (auto* ix = dynamic_cast<ast::IndexExpr*>(&e)) {
        out.push_back(&ix->getObjectPtr());
        if (ix->hasIndex()) out.push_back(&ix->getIndexPtr());
    } else if (auto* arr = dynamic_cast<ast::ArrayExpr*>(&e)) {
        for (auto& el : arr->getElements()) out.push_back(&el);
    } else if (auto* mp = dynamic_cast<ast::MapExpr*>(&e)) {
        for (auto& [k, v] : mp->getEntries()) {
            out.push_back(&k);
            out.push_back(&v);
        }
    } else if (auto* t = dynamic_cast<ast::TernaryExpr*>(&e)) {
        out.push_back(&t->getConditionPtr());
        if (t->hasThenExpr()) out.push_back(&t->getThenExprPtr());
        if (t->hasElseExpr()) out.push_back(&t->getElseExprPtr());
    } else if (auto* a = dynamic_cast<ast::AssignExpr*>(&e)) {
        out.push_back(&a->getTargetPtr());
        if (a->hasValue()) out.push_back(&a->getValuePtr());
    } else if (auto* n = dynamic_cast<ast::NewExpr*>(&e)) {
        for (auto& arg : n->getArgs()) out.push_back(&arg);
    } else if (auto* tr = dynamic_cast<ast::TryExpr*>(&e)) {
        out.push_back(&tr->getExprPtr());
    } else if (auto* aw = dynamic_cast<ast::AwaitExpr*>(&e)) {
        out.push_back(&aw->getExprPtr());
    } else if (auto* ct = dynamic_cast<ast::ComptimeExpr*>(&e)) {
        if (ct->hasExpr()) out.push_back(&ct->getExprPtr());
    }
    // LambdaExpr: not descended into. Literal/Identifier: no children.
}

void IRBuilder::asyncCollectAwaits(const ast::ExpressionNode& e,
                                   std::vector<const ast::AwaitExpr*>& out) {
    if (auto* aw = dynamic_cast<const ast::AwaitExpr*>(&e)) {
        // Inner awaits (in the operand) evaluate first.
        asyncCollectAwaits(aw->getExpr(), out);
        out.push_back(aw);
        return;
    }
    std::vector<const ast::ExpressionNode*> children;
    asyncChildExprsConst(e, children);
    for (const auto* child : children) asyncCollectAwaits(*child, out);
}

const ast::AwaitExpr* IRBuilder::asyncFirstAwait(const ast::ExpressionNode& e) {
    if (auto* aw = dynamic_cast<const ast::AwaitExpr*>(&e)) return aw;
    std::vector<const ast::ExpressionNode*> children;
    asyncChildExprsConst(e, children);
    if (children.empty()) return nullptr;
    // Walk down the first child at each level: the await reached this
    // way is the first thing evaluated, so nothing is live across it.
    return asyncFirstAwait(*children[0]);
}

bool IRBuilder::asyncHoistAwaits(ast::ExprPtr& slot, const ast::AwaitExpr* keep,
                                 std::vector<ast::StmtPtr>& out) {
    if (!slot) return true;
    if (auto* aw = dynamic_cast<ast::AwaitExpr*>(slot.get())) {
        // Hoist inner awaits first (they evaluate before this one).
        if (!asyncHoistAwaits(aw->getExprPtr(), keep, out)) return false;
        if (aw == keep) return true; // The first-evaluated await stays.
        std::string name = "__sp_await_" + std::to_string(m_asyncSM->nextHoistId++);
        auto loc = aw->getLocation();
        ast::ExprPtr moved = std::move(slot);
        slot = std::make_unique<ast::IdentifierExpr>(name, loc);
        auto letStmt = std::make_unique<ast::VarDeclStmt>(
            "", name, std::move(moved), false, true, loc);
        // The hoisted let's slot count follows the awaited value's
        // type (a struct value needs more than one slot).
        auto awaitTy = m_sema.getNodeType(letStmt->getInitializer());
        asyncAssignLocalSlot(letStmt.get(), asyncSlotCountForType(awaitTy));
        out.push_back(std::move(letStmt));
        return true;
    }
    std::vector<ast::ExprPtr*> children;
    asyncChildExprSlots(*slot, children);
    for (auto* child : children) {
        if (*child && !asyncHoistAwaits(*child, keep, out)) return false;
    }
    return true;
}

int64_t IRBuilder::asyncScanStmtList(std::vector<ast::StmtPtr>& stmts) {
    int64_t total = 0;
    std::vector<ast::StmtPtr> out;
    out.reserve(stmts.size());
    for (auto& stmt : stmts) {
        if (!stmt) continue;
        if (auto* vd = dynamic_cast<ast::VarDeclStmt*>(stmt.get())) {
            asyncAssignLocalSlot(vd, asyncVarDeclSlotCount(*vd));
        }
        int64_t n = asyncScanNested(*stmt);
        if (n < 0) return -1;
        total += n;
        std::vector<ast::StmtPtr> hoisted;
        n = asyncScanStmtExprs(*stmt, &hoisted);
        if (n < 0) return -1;
        total += n;
        for (auto& h : hoisted) out.push_back(std::move(h));
        out.push_back(std::move(stmt));
    }
    stmts = std::move(out);
    return total;
}

int64_t IRBuilder::asyncScanBlock(ast::BlockStmt& block) {
    return asyncScanStmtList(block.getStatements());
}

int64_t IRBuilder::asyncScanNested(ast::StatementNode& stmt) {
    int64_t total = 0;
    // A branch body: a block scans in place; a single statement is
    // scanned with hoisting disallowed (there is no block to insert
    // the hoisted lets into — the user must write `{ ... }`).
    auto scanBranch = [&](ast::StmtPtr& branch) -> int64_t {
        if (!branch) return 0;
        if (auto* block = dynamic_cast<ast::BlockStmt*>(branch.get())) {
            return asyncScanBlock(*block);
        }
        int64_t n = asyncScanNested(*branch);
        if (n < 0) return -1;
        n = asyncScanStmtExprs(*branch, nullptr);
        if (n < 0) return -1;
        return n;
    };
    if (auto* block = dynamic_cast<ast::BlockStmt*>(&stmt)) {
        return asyncScanBlock(*block);
    } else if (auto* is = dynamic_cast<ast::IfStmt*>(&stmt)) {
        int64_t n = scanBranch(is->getThenBranchPtr());
        if (n < 0) return -1;
        total += n;
        if (is->hasElseBranch()) {
            n = scanBranch(is->getElseBranchPtr());
            if (n < 0) return -1;
            total += n;
        }
    } else if (auto* ws = dynamic_cast<ast::WhileStmt*>(&stmt)) {
        int64_t n = scanBranch(ws->getBodyPtr());
        if (n < 0) return -1;
        total += n;
    } else if (auto* dws = dynamic_cast<ast::DoWhileStmt*>(&stmt)) {
        int64_t n = scanBranch(dws->getBodyPtr());
        if (n < 0) return -1;
        total += n;
    } else if (auto* fs = dynamic_cast<ast::ForStmt*>(&stmt)) {
        if (fs->hasInit()) {
            if (const auto* initVd = dynamic_cast<const ast::VarDeclStmt*>(fs->getInit())) {
                asyncAssignLocalSlot(initVd, asyncVarDeclSlotCount(*initVd));
            }
        }
        int64_t n = scanBranch(fs->getBodyPtr());
        if (n < 0) return -1;
        total += n;
    } else if (auto* fis = dynamic_cast<ast::ForInStmt*>(&stmt)) {
        asyncAssignLocalSlot(fis, asyncForInSlotCount(*fis)); // loop var slot, keyed by the ForInStmt node
        // A collection loop also carries an induction variable (idx)
        // across iterations; a range loop reuses the loop var itself
        // as the counter. The idx must live in the frame (an
        // entry-block alloca would not survive an await in the body).
        bool isRange = false;
        if (fis->hasIterable()) {
            if (const auto* bin = dynamic_cast<const ast::BinaryExpr*>(&fis->getIterable())) {
                isRange = (bin->getOp() == lexer::TokenType::DotDot);
            }
        }
        if (!isRange) asyncAuxSlot(fis, 0, 1);
        int64_t n = scanBranch(fis->getBodyPtr());
        if (n < 0) return -1;
        total += n;
    } else if (auto* ss = dynamic_cast<ast::SwitchStmt*>(&stmt)) {
        for (auto& c : ss->getCases()) {
            int64_t n = asyncScanStmtList(c.getStatements());
            if (n < 0) return -1;
            total += n;
        }
        int64_t n = asyncScanStmtList(ss->getDefaultStatements());
        if (n < 0) return -1;
        total += n;
    } else if (auto* ms = dynamic_cast<ast::MatchStmt*>(&stmt)) {
        for (auto& arm : ms->getArms()) {
            if (!arm.hasBody()) continue;
            auto& body = arm.getBodyPtr();
            if (auto* block = dynamic_cast<ast::BlockStmt*>(body.get())) {
                int64_t n = asyncScanBlock(*block);
                if (n < 0) return -1;
                total += n;
            } else if (auto* exprBody = dynamic_cast<ast::ExpressionNode*>(body.get())) {
                // Expression arm body: a single first-evaluated await
                // is fine; more (or a non-first await) cannot be
                // hoisted out of the arm, so fail loudly.
                std::vector<const ast::AwaitExpr*> awaits;
                asyncCollectAwaits(*exprBody, awaits);
                total += static_cast<int64_t>(awaits.size());
                if (!awaits.empty()) {
                    const ast::AwaitExpr* keep = asyncFirstAwait(*exprBody);
                    if (!(awaits.size() == 1 && keep == awaits[0])) {
                        reportUnsupported(ms->getLocation(), "await",
                            "a match arm body expression with more than one 'await' (or an "
                            "'await' that is not the first thing evaluated) must be a block "
                            "'{ ... }' so the extra awaits can be hoisted");
                        return -1;
                    }
                }
            } else if (auto* stmtBody = dynamic_cast<ast::StatementNode*>(body.get())) {
                int64_t n = asyncScanNested(*stmtBody);
                if (n < 0) return -1;
                total += n;
                n = asyncScanStmtExprs(*stmtBody, nullptr);
                if (n < 0) return -1;
                total += n;
            }
        }
    } else if (auto* tcs = dynamic_cast<ast::TryCatchStmt*>(&stmt)) {
        int64_t n = asyncScanBlock(*tcs->getTryBlockPtr());
        if (n < 0) return -1;
        total += n;
        for (auto& catchClause : tcs->getCatches()) {
            n = asyncScanBlock(*catchClause.getBodyPtr());
            if (n < 0) return -1;
            total += n;
        }
        if (tcs->hasFinally()) {
            n = asyncScanBlock(*tcs->getFinallyBlockPtr());
            if (n < 0) return -1;
            total += n;
        }
    }
    return total;
}

int64_t IRBuilder::asyncScanStmtExprs(ast::StatementNode& stmt, std::vector<ast::StmtPtr>* insert) {
    std::vector<ast::ExprPtr*> slots;
    if (auto* es = dynamic_cast<ast::ExprStmt*>(&stmt)) {
        if (es->hasExpr()) slots.push_back(&es->getExprPtr());
    } else if (auto* vd = dynamic_cast<ast::VarDeclStmt*>(&stmt)) {
        if (vd->hasInitializer()) slots.push_back(&vd->getInitializerPtr());
    } else if (auto* rs = dynamic_cast<ast::ReturnStmt*>(&stmt)) {
        if (rs->hasValue()) slots.push_back(&rs->getValuePtr());
    } else if (auto* is = dynamic_cast<ast::IfStmt*>(&stmt)) {
        if (is->hasCondition()) slots.push_back(&is->getConditionPtr());
    } else if (auto* ws = dynamic_cast<ast::WhileStmt*>(&stmt)) {
        if (ws->hasCondition()) slots.push_back(&ws->getConditionPtr());
    } else if (auto* dws = dynamic_cast<ast::DoWhileStmt*>(&stmt)) {
        if (dws->hasCondition()) slots.push_back(&dws->getConditionPtr());
    } else if (auto* fs = dynamic_cast<ast::ForStmt*>(&stmt)) {
        if (fs->hasInit()) {
            if (auto* initVd = dynamic_cast<ast::VarDeclStmt*>(fs->getInitPtr().get())) {
                if (initVd->hasInitializer()) slots.push_back(&initVd->getInitializerPtr());
            } else if (auto* initEs = dynamic_cast<ast::ExprStmt*>(fs->getInitPtr().get())) {
                if (initEs->hasExpr()) slots.push_back(&initEs->getExprPtr());
            }
        }
        if (fs->hasCondition()) slots.push_back(&fs->getConditionPtr());
        if (fs->hasIncrement()) slots.push_back(&fs->getIncrementPtr());
    } else if (auto* fis = dynamic_cast<ast::ForInStmt*>(&stmt)) {
        if (fis->hasIterable()) slots.push_back(&fis->getIterablePtr());
    } else if (auto* ss = dynamic_cast<ast::SwitchStmt*>(&stmt)) {
        if (ss->hasSubject()) slots.push_back(&ss->getSubjectPtr());
        for (auto& c : ss->getCases()) {
            if (c.hasValue()) slots.push_back(&c.getValuePtr());
        }
    } else if (auto* ms = dynamic_cast<ast::MatchStmt*>(&stmt)) {
        if (ms->hasSubject()) slots.push_back(&ms->getSubjectPtr());
        for (auto& arm : ms->getArms()) {
            if (!arm.isWildcard()) slots.push_back(&arm.getPatternPtr());
        }
    } else if (auto* ts = dynamic_cast<ast::ThrowStmt*>(&stmt)) {
        if (ts->hasExpr()) slots.push_back(&ts->getExprPtr());
    }
    // (BlockStmt / Break / Continue / TryCatchStmt have no own exprs.)
    int64_t total = 0;
    for (auto* slot : slots) {
        if (!slot || !*slot) continue;
        std::vector<const ast::AwaitExpr*> awaits;
        asyncCollectAwaits(**slot, awaits);
        total += static_cast<int64_t>(awaits.size());
        if (awaits.empty()) continue;
        const ast::AwaitExpr* keep = asyncFirstAwait(**slot);
        if (awaits.size() == 1 && keep == awaits[0]) continue;
        if (!insert) {
            reportUnsupported(stmt.getLocation(), "await",
                "a statement with more than one 'await' (or an 'await' that is not "
                "the first thing evaluated) must be inside a block '{ ... }' so the "
                "extra awaits can be hoisted into separate statements");
            return -1;
        }
        std::vector<ast::StmtPtr> hoisted;
        if (!asyncHoistAwaits(*slot, keep, hoisted)) {
            reportUnsupported(stmt.getLocation(), "await",
                "an 'await' in this position cannot be hoisted; split the statement");
            return -1;
        }
        for (auto& h : hoisted) insert->push_back(std::move(h));
    }
    return total;
}

int64_t IRBuilder::asyncPrepareBody(ast::FunctionDecl& decl, const std::string& compiledName,
                                    const sema::TypePtr& returnType,
                                    const std::vector<sema::TypePtr>& semaParamTypes,
                                    const sema::TypePtr& thisType) {
    (void)compiledName;
    // Short-form bodies become block form first (a ReturnStmt), so the
    // rest of the pipeline only ever sees a body block.
    if (decl.isShortForm()) {
        ast::ExprPtr shortBody = std::move(decl.getShortBodyPtr());
        auto block = std::make_unique<ast::BlockStmt>(
            std::vector<ast::StmtPtr>{}, decl.getLocation());
        block->getStatements().push_back(std::make_unique<ast::ReturnStmt>(
            std::move(shortBody), decl.getLocation()));
        decl.getBodyPtr() = std::move(block);
    }
    if (!decl.hasBody()) return -1; // declaration only — no body to lower
    m_asyncSM = std::make_unique<AsyncSM>();
    // Validate the declared return type: it must be carriable by a Task.
    std::string kind;
    if (!asyncReturnKind(returnType, kind)) {
        reportUnsupported(decl.getLocation(), "async fn",
            "the return type of this 'async fn' cannot be carried by a Task "
            "(only void, int, bool, string, double, array and struct payloads are supported)");
        m_asyncSM.reset();
        return -1;
    }
    m_asyncSM->payloadKind = kind;
    if (kind == "struct") {
        std::string structName = returnType->toString();
        const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(structName) : nullptr;
        if (!layout || !layout->structType) {
            reportUnsupported(decl.getLocation(), "async fn",
                "the struct return type of this 'async fn' has no lowered struct type");
            m_asyncSM.reset();
            return -1;
        }
        m_asyncSM->structType = layout->structType;
    }
    // Validate + record the params (`this` first for methods). A
    // struct parameter cannot be carried in an 8-byte ctx slot.
    if (thisType) {
        m_asyncSM->paramNames.push_back("this");
        m_asyncSM->paramIRTypes.push_back(IRType::getPointer());
    }
    for (size_t i = 0; i < semaParamTypes.size(); ++i) {
        const auto& t = semaParamTypes[i];
        if (t && t->isStruct()) {
            reportUnsupported(decl.getLocation(), "async fn",
                "a parameter of struct type cannot be carried to an 'async fn' "
                "(pass a pointer or a class instead)");
            m_asyncSM.reset();
            return -1;
        }
        m_asyncSM->paramNames.push_back(decl.getParams()[i].getName());
        m_asyncSM->paramIRTypes.push_back(mapType(t.get()));
    }
    for (size_t i = 0; i < m_asyncSM->paramNames.size(); ++i) {
        m_asyncSM->paramSlots[static_cast<int>(i)] = AsyncSM::kFirstParamSlot + static_cast<int64_t>(i);
    }
    m_asyncSM->nextSlot = AsyncSM::kFirstParamSlot + static_cast<int64_t>(m_asyncSM->paramNames.size());
    // Scan the body: assign local slots + hoist extra awaits.
    int64_t count = asyncScanBlock(*decl.getBodyPtr());
    if (count < 0) {
        m_asyncSM.reset();
        return -1;
    }
    m_asyncSM->frameSize = m_asyncSM->nextSlot * 8;
    return count;
}

void IRBuilder::asyncSMBegin(const std::string& compiledName, IRFunction* bodyFn) {
    auto& sm = *m_asyncSM;
    sm.bodyFn = bodyFn;
    // The resume function: void __sp_async_resume_<name>(ptr %frame).
    std::vector<std::shared_ptr<ParameterValue>> resumeParams;
    resumeParams.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%frame", 0));
    auto resumeFn = m_module->createFunction(asyncResumeName(compiledName), IRType::getVoid(), resumeParams);
    sm.resumeFn = resumeFn;
    sm.frameParam = resumeParams[0];
    sm.dispatcherBB = resumeFn->createBasicBlock("entry");
    sm.segment0BB = resumeFn->createBasicBlock("segment0");
    sm.finishBB = resumeFn->createBasicBlock("finish");
    // Lower the body statements into the resume function, starting in
    // segment 0. Params are declared as frame slots (the body prologue,
    // emitted by asyncSMEnd, fills them from the ctx).
    m_currentFunction = resumeFn;
    setInsertPoint(sm.segment0BB);
    for (size_t i = 0; i < sm.paramNames.size(); ++i) {
        int64_t slot = sm.paramSlots[static_cast<int>(i)];
        auto gep = asyncFrameSlotGEP(slot);
        declareVariable(sm.paramNames[i], gep, sm.paramIRTypes[i]);
    }
}

void IRBuilder::asyncSMEnd() {
    auto& sm = *m_asyncSM;
    // A body that falls off the end without a return branches to the
    // finish block (sema requires a return on all paths for non-void
    // fns, but be safe rather than leave the segment unterminated).
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(sm.finishBB->getName()));
    }
    // Dispatcher chain (entry block): state 0 -> segment 0, state
    // N -> continuations[N-1], anything else -> finish. The state
    // value is loaded once in the entry block (it dominates the check
    // blocks); each check is `icmp eq %state, N`.
    setInsertPoint(sm.dispatcherBB);
    auto state = asyncFrameLoad(AsyncSM::kStateSlot, IRType::getInt64(), "async_state");
    IRBasicBlock* checkBB = sm.dispatcherBB;
    for (size_t i = 0; i <= sm.continuations.size(); ++i) {
        IRBasicBlock* target = (i == 0) ? sm.segment0BB : sm.continuations[i - 1];
        bool isLast = (i == sm.continuations.size());
        IRBasicBlock* elseBB = isLast
            ? sm.finishBB
            : m_currentFunction->createBasicBlock("async_check_" + std::to_string(i + 1));
        auto cmp = std::make_shared<CompareInst>(
            IROpcode::ICmpEq, state,
            std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(i)),
            m_currentFunction->nextTempName("async_state_eq"));
        checkBB->addInstruction(cmp);
        checkBB->addInstruction(std::make_shared<CondBranchInst>(
            cmp, target->getName(), elseBB->getName()));
        checkBB = elseBB;
        if (isLast) break;
    }
    // Finish block: complete the task with the return value (a no-op
    // when the task was already cancelled), free the frame, return.
    setInsertPoint(sm.finishBB);
    auto task = asyncFrameLoad(AsyncSM::kTaskSlot, IRType::getPointer(), "async_task");
    const std::string& kind = sm.payloadKind;
    if (kind == "void") {
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_complete_void",
            std::vector<IRValuePtr>{task}));
    } else if (kind == "int" || kind == "bool") {
        auto v = asyncFrameLoad(AsyncSM::kRetValSlot, IRType::getInt64(), "async_ret");
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_complete_i64",
            std::vector<IRValuePtr>{task, v}));
    } else if (kind == "string") {
        auto v = asyncFrameLoad(AsyncSM::kRetValSlot, IRType::getPointer(), "async_ret");
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_complete_str",
            std::vector<IRValuePtr>{task, v}));
        // Balance the retain in visit(ReturnStmt): the task copied the
        // string, so the returned reference can be released now.
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_release",
            std::vector<IRValuePtr>{v}));
    } else if (kind == "double") {
        auto v = asyncFrameLoad(AsyncSM::kRetValSlot, IRType::getDouble(), "async_ret");
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_complete_dbl",
            std::vector<IRValuePtr>{task, v}));
    } else if (kind == "array") {
        auto v = asyncFrameLoad(AsyncSM::kRetValSlot, IRType::getPointer(), "async_ret");
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_complete_ptr",
            std::vector<IRValuePtr>{task, v}));
    } else if (kind == "struct") {
        auto v = asyncFrameLoad(AsyncSM::kRetValSlot, IRType::getPointer(), "async_ret");
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_complete_ptr_owned",
            std::vector<IRValuePtr>{task, v}));
    }
    m_currentBlock->addInstruction(std::make_shared<CallInst>(
        IRType::getVoid(), "sp_rt_free",
        std::vector<IRValuePtr>{sm.frameParam}));
    m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
    // Body prologue (in the body function's entry block): allocate
    // the frame, store the task + the ctx args into their slots, call
    // the resume function, free the ctx, return.
    IRFunction* bodyFn = sm.bodyFn;
    IRBasicBlock* bodyEntry = bodyFn->getEntryBlock();
    const auto& bodyParams = bodyFn->getParams();
    IRValuePtr taskParam = bodyParams[0];
    IRValuePtr ctxParam = bodyParams[1];
    m_currentFunction = bodyFn;
    setInsertPoint(bodyEntry);
    auto frameAlloc = std::make_shared<CallInst>(
        IRType::getPointer(), "sp_rt_alloc",
        std::vector<IRValuePtr>{std::make_shared<ConstantInt>(IRType::getInt64(), sm.frameSize)},
        m_currentFunction->nextTempName("async_frame"));
    m_currentBlock->addInstruction(frameAlloc);
    auto bodySlotGEP = [&](int64_t slot) {
        auto gep = std::make_shared<GetElementPtrInst>(
            IRType::getInt8(), frameAlloc,
            std::vector<IRValuePtr>{std::make_shared<ConstantInt>(
                IRType::getInt64(), asyncFrameOffset(slot))},
            m_currentFunction->nextTempName("frame_slot"));
        m_currentBlock->addInstruction(gep);
        return gep;
    };
    // frame[1] = task.
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(taskParam, bodySlotGEP(AsyncSM::kTaskSlot)));
    // frame[paramSlot[i]] = ctx[i] (cast from the ctx storage type to
    // the declared param type: i64 -> i32/i1, double -> double, ...).
    for (size_t i = 0; i < sm.paramNames.size(); ++i) {
        IRTypePtr paramTy = sm.paramIRTypes[i];
        IRTypePtr storageTy;
        if (paramTy->isInteger()) storageTy = IRType::getInt64();
        else if (paramTy->isPointer()) storageTy = IRType::getPointer();
        // isFloatingPoint() covers BOTH float and double (see
        // emitAsyncSpawnSM) — isFloat() alone would silently skip a
        // `double` param, leaving its frame slot uninitialized.
        else if (paramTy->isFloatingPoint()) storageTy = IRType::getDouble();
        else continue; // cannot happen: asyncPrepareBody rejected struct params
        auto gep = std::make_shared<GetElementPtrInst>(
            IRType::getInt8(), ctxParam,
            std::vector<IRValuePtr>{std::make_shared<ConstantInt>(
                IRType::getInt64(), static_cast<int64_t>(i * 8))},
            m_currentFunction->nextTempName("ctx_slot"));
        m_currentBlock->addInstruction(gep);
        auto loaded = std::make_shared<LoadInst>(storageTy, gep, m_currentFunction->nextTempName("ctx_arg"));
        m_currentBlock->addInstruction(loaded);
        IRValuePtr argVal = emitCastIfNeeded(loaded, paramTy);
        int64_t slot = sm.paramSlots[static_cast<int>(i)];
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(argVal, bodySlotGEP(slot)));
    }
    m_currentBlock->addInstruction(std::make_shared<CallInst>(
        IRType::getVoid(), sm.resumeFn->getName(),
        std::vector<IRValuePtr>{frameAlloc}));
    m_currentBlock->addInstruction(std::make_shared<CallInst>(
        IRType::getVoid(), "sp_rt_free",
        std::vector<IRValuePtr>{ctxParam}));
    m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
    m_asyncSM.reset();
}

IRValuePtr IRBuilder::asyncSuspendAndLoad(IRValuePtr task, const std::string& kind, IRTypePtr structTy, bool destroyTask) {
    auto& sm = *m_asyncSM;
    // One continuation per await: contBB is the dispatcher target (it
    // does the cancellation check), bodyBB is where lowering
    // continues. State N (1-based) resumes at continuations[N-1].
    int64_t state = static_cast<int64_t>(sm.continuations.size()) + 1;
    IRBasicBlock* contBB = m_currentFunction->createBasicBlock(
        "async_cont_" + std::to_string(state));
    IRBasicBlock* bodyBB = m_currentFunction->createBasicBlock(
        "async_resume_" + std::to_string(state));
    sm.continuations.push_back(contBB);
    // In the current segment: store the next state + the awaited task,
    // register the resume (posted to the scheduler, never called
    // synchronously), and return — the stack unwinds.
    asyncFrameStore(AsyncSM::kStateSlot, std::make_shared<ConstantInt>(IRType::getInt64(), state));
    asyncFrameStore(AsyncSM::kAwaitTaskSlot, task);
    auto sched = emitAsyncScheduler();
    auto resumeAddr = std::make_shared<FunctionAddress>(sm.resumeFn->getName());
    m_currentBlock->addInstruction(std::make_shared<CallInst>(
        IRType::getVoid(), "sp_rt_async_task_on_complete_resume",
        std::vector<IRValuePtr>{sched, task, resumeAddr, sm.frameParam}));
    m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
    // Continuation: a body cancelled while suspended stops here — it
    // branches to the finish block (which completes the task, a no-op,
    // and frees the frame) instead of running the rest of the body.
    setInsertPoint(contBB);
    auto ownTask = asyncFrameLoad(AsyncSM::kTaskSlot, IRType::getPointer(), "async_task");
    auto wasCancelled = std::make_shared<CallInst>(
        IRType::getInt1(), "sp_rt_async_task_was_cancelled",
        std::vector<IRValuePtr>{ownTask},
        m_currentFunction->nextTempName("async_cancelled"));
    m_currentBlock->addInstruction(wasCancelled);
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(
        wasCancelled, sm.finishBB->getName(), bodyBB->getName()));
    // Resume body: load the awaited value with the non-blocking
    // getters (the awaited task is already complete — its
    // continuation fired to resume us; a cancelled awaited task
    // yields the default value and the body continues).
    setInsertPoint(bodyBB);
    auto awaitTask = asyncFrameLoad(AsyncSM::kAwaitTaskSlot, IRType::getPointer(), "async_await_task");
    IRValuePtr value = nullptr;
    if (kind == "void") {
        value = nullptr;
    } else if (kind == "int" || kind == "bool") {
        auto v = std::make_shared<CallInst>(
            IRType::getInt64(), "sp_rt_async_task_value_i64",
            std::vector<IRValuePtr>{awaitTask},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(v);
        value = (kind == "bool") ? emitCastIfNeeded(v, IRType::getInt1()) : v;
    } else if (kind == "string") {
        value = std::make_shared<CallInst>(
            IRType::getPointer(), "sp_rt_async_task_value_str",
            std::vector<IRValuePtr>{awaitTask},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(value));
    } else if (kind == "double") {
        value = std::make_shared<CallInst>(
            IRType::getDouble(), "sp_rt_async_task_value_dbl",
            std::vector<IRValuePtr>{awaitTask},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(value));
    } else if (kind == "array") {
        value = std::make_shared<CallInst>(
            IRType::getPointer(), "sp_rt_async_task_value_ptr",
            std::vector<IRValuePtr>{awaitTask},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(value));
    } else if (kind == "struct" && structTy) {
        auto box = std::make_shared<CallInst>(
            IRType::getPointer(), "sp_rt_async_task_value_ptr",
            std::vector<IRValuePtr>{awaitTask},
            m_currentFunction->nextTempName("awaited"));
        m_currentBlock->addInstruction(box);
        value = emitStructValueFromBox(box, structTy);
    } else {
        reportUnsupported(diagnostics::SourceLocation{}, "await expression",
            "internal error: unsupported async payload kind '" + kind + "'");
        return nullptr;
    }

    // A temporary task (created by this await itself) is destroyed
    // now that its value has been copied out. The task handle is
    // re-loaded from the frame — the SSA value from the previous
    // segment does not dominate the continuation. For an array the
    // pointer outlives the task by design, for a string the getter
    // returns a fresh copy, and for a struct the value was loaded by
    // value, so destroying the task here is safe. (The destroy lands
    // in the merge block for a struct await — emitStructValueFromBox
    // moved the insert point there — which is where lowering must
    // continue, so the insert point is NOT reset to bodyBB below.)
    if (destroyTask) {
        auto t = asyncFrameLoad(AsyncSM::kAwaitTaskSlot, IRType::getPointer(), "await_task");
        m_currentBlock->addInstruction(std::make_shared<CallInst>(
            IRType::getVoid(), "sp_rt_async_task_destroy",
            std::vector<IRValuePtr>{t}));
    }

    // The insert point is already where lowering must continue:
    // bodyBB for scalar kinds, the merge block for a struct await.
    // (Resetting it to bodyBB here would abandon the merge block
    // without a terminator and add instructions after bodyBB's
    // CondBranch.)
    return value;
}

void IRBuilder::visit(const ast::AwaitExpr& node) {
    // Phase 2 slice B2 (pro-4.0.0) + Phase 3 slice C: REAL await.
    //
    // SUPPORTED (lowers, no longer errors) — all inside an `async fn`
    // (free fn, method, or generic specialization; m_inAsyncFunction):
    //   - `await sleep_async(ms)` (ms: int). Yields void.
    //   - `await delayed_value(ms, value)` (ms: int; value: int, bool
    //     or string). Yields the value after `ms`.
    //   - `await worker(...)` where `worker` is a user `async fn`:
    //     the call spawns the body as a Task (see visit(CallExpr)) and
    //     this await pumps until it completes, then destroys the
    //     temporary task. Yields the body's return value.
    //   - `await <expr of type Task<T>>` (e.g. a variable holding a
    //     Task, or a call to a non-async fn returning Task<T>): pumps
    //     until the task completes. The task is NOT destroyed — its
    //     owner keeps it, so awaiting it twice returns the same value.
    //   Payloads are void / int / bool / string / double / array /
    //   struct (the Task payload the runtime can carry). All forms
    //   may appear anywhere in the async fn body (statement,
    //   initializer, inside if/while, ...) because the lowering is
    //   pump-based, not a state machine (see below).
    //
    // LOWERING (Phase 5 slice A): inside an async fn the await
    // SUSPENDS through the heap-frame state machine (see
    // asyncSuspendAndLoad): the current segment stores the next
    // state + the awaited task, registers the resume as the awaited
    // task's continuation, and returns — the stack unwinds. The
    // continuation resumes in a fresh invocation of the resume fn.
    // Outside an async fn (which cannot happen — see the backstop)
    // the pump path is kept as a fallback: emitAwaitTask pumps the
    // default scheduler until the task completes. That is real
    // suspension of the async fn's progress with a live loop — not
    // a blocking sleep: no sp_rt_time_sleep_ms /
    // sp_rt_thread_sleep_ms is emitted anywhere on this path.
    // Honest limits of this design: a task whose body never returns
    // never completes, and a body cancelled while suspended on a
    // task that never completes is never resumed (its frame is never
    // freed — a bounded leak, documented in the Phase 34 slice A doc).
    //
    // NOT SUPPORTED (still error[E1000], never silently miscompiled):
    //   - await on anything else: literals, other calls (sync fns,
    //     `await now()`), operators, ...
    //   - await outside an `async fn` (sema normally rejects this
    //     first with E1802; this is the IRBuilder backstop).
    //   - async fns with other payload types (classes, ...),
    //     and async fns with unsupported argument types.
    //   Async methods ARE supported, including through a virtual
    //   dispatch: the vtable-loaded body address is passed directly
    //   as the thunk to sp_rt_async_spawn_sm (see visit(CallExpr)).
    if (!m_inAsyncFunction) {
        reportUnsupported(node.getLocation(), "await",
                          "await is only supported inside an 'async fn' in this phase");
        node.getExpr().accept(*this);
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }

    const auto* call = dynamic_cast<const ast::CallExpr*>(&node.getExpr());
    const ast::IdentifierExpr* calleeIdent = nullptr;
    if (call) {
        calleeIdent = dynamic_cast<const ast::IdentifierExpr*>(&call->getCallee());
    }
    const std::string calleeName = calleeIdent ? calleeIdent->getName() : "";
    const bool isSleepAsync = (calleeName == "sleep_async" && call->getArgs().size() == 1);
    const bool isDelayedValue = (calleeName == "delayed_value" && call->getArgs().size() == 2);

    // (1) Builtins: sleep_async / delayed_value (incl. the Phase 3
    // string/bool delayed_value overloads).
    if (call && (isSleepAsync || isDelayedValue)) {
        // Evaluate the ms argument (in source order) without visiting
        // the call itself: a plain visit(CallExpr) would emit a call
        // to the SP-level builtin, which has no IR function.
        call->getArgs()[0]->accept(*this);
        IRValuePtr ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        auto sched = emitAsyncScheduler();

        if (isSleepAsync) {
            auto taskCall = std::make_shared<CallInst>(
                IRType::getPointer(), "sp_rt_async_sleep_ms",
                std::vector<IRValuePtr>{sched, ms},
                m_currentFunction->nextTempName("async_task"));
            m_currentBlock->addInstruction(taskCall);
            if (inAsyncSM()) {
                // Phase 5 slice A: suspend through the state machine.
                // The temporary task is destroyed after the resume.
                asyncSuspendAndLoad(taskCall, "void", nullptr, true);
            } else {
                emitAwaitTask(sched, taskCall, "void");
                m_currentBlock->addInstruction(std::make_shared<CallInst>(
                    IRType::getVoid(), "sp_rt_async_task_destroy",
                    std::vector<IRValuePtr>{taskCall}));
            }
            m_lastExprValue = nullptr;
            m_lastExprType = IRType::getVoid();
            return;
        }

        // delayed_value: payload kind from the value arg's sema type.
        std::string valKind;
        if (auto valSemaType = m_sema.getNodeType(call->getArgs()[1].get())) {
            std::string tn = valSemaType->toString();
            if (tn == "string") valKind = "string";
            else if (tn == "bool") valKind = "bool";
            else if (tn == "double" || tn == "float" || tn == "float32" || tn == "float64") valKind = "double";
            else if (tn == "int" || tn == "int8" || tn == "int16" || tn == "int32" || tn == "int64") valKind = "int";
        }
        if (valKind.empty()) {
            reportUnsupported(node.getLocation(), "await",
                              "'delayed_value' only carries int, bool, string and double payloads in this phase");
            node.getExpr().accept(*this);
            m_lastExprValue = nullptr;
            m_lastExprType = nullptr;
            return;
        }
        call->getArgs()[1]->accept(*this);
        IRValuePtr value;
        std::string delayedFn;
        if (valKind == "string") {
            value = m_lastExprValue;
            delayedFn = "sp_rt_async_delayed_str";
        } else if (valKind == "double") {
            value = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
            delayedFn = "sp_rt_async_delayed_dbl";
        } else {
            value = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
            delayedFn = "sp_rt_async_delayed_i64";
        }
        auto taskCall = std::make_shared<CallInst>(
            IRType::getPointer(), delayedFn,
            std::vector<IRValuePtr>{sched, ms, value},
            m_currentFunction->nextTempName("async_task"));
        m_currentBlock->addInstruction(taskCall);
        IRValuePtr awaited;
        if (inAsyncSM()) {
            // Phase 5 slice A: suspend through the state machine.
            // The temporary task is destroyed after the resume.
            awaited = asyncSuspendAndLoad(taskCall, valKind, nullptr, true);
        } else {
            awaited = emitAwaitTask(sched, taskCall, valKind);
            m_currentBlock->addInstruction(std::make_shared<CallInst>(
                IRType::getVoid(), "sp_rt_async_task_destroy",
                std::vector<IRValuePtr>{taskCall}));
        }
        m_lastExprValue = awaited;
        m_lastExprType = valKind == "string" ? IRType::getPointer()
                       : valKind == "bool" ? IRType::getInt1()
                       : valKind == "double" ? IRType::getDouble()
                       : IRType::getInt64();
        return;
    }

    // (2) await on a call to a user async fn: spawn (via visit(CallExpr))
    // + await inline, then destroy the temporary task.
    if (call) {
        if (auto target = m_sema.getCallTarget(call); target && target->isAsync()) {
            std::string kind;
            if (!taskPayloadKind(m_sema.getNodeType(call), kind)) {
                reportUnsupported(node.getLocation(), "await",
                                  "the return type of this 'async fn' cannot be carried by a Task "
                                  "(only void, int, bool, string, double, array and struct payloads are supported)");
                node.getExpr().accept(*this);
                m_lastExprValue = nullptr;
                m_lastExprType = nullptr;
                return;
            }
            IRTypePtr structTy = nullptr;
            if (kind == "struct") {
                structTy = asyncStructTypeForTask(m_sema.getNodeType(call));
                if (!structTy) {
                    reportUnsupported(node.getLocation(), "await",
                        "internal error: the struct return type of this 'async fn' has no IR struct type");
                    node.getExpr().accept(*this);
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
            }
            call->accept(*this);
            IRValuePtr task = m_lastExprValue;
            IRValuePtr awaited;
            if (inAsyncSM()) {
                // Phase 5 slice A: suspend through the state machine.
                // The temporary task is destroyed after the resume.
                awaited = asyncSuspendAndLoad(task, kind, structTy, true);
            } else {
                auto sched = emitAsyncScheduler();
                awaited = emitAwaitTask(sched, task, kind, structTy);
                m_currentBlock->addInstruction(std::make_shared<CallInst>(
                    IRType::getVoid(), "sp_rt_async_task_destroy",
                    std::vector<IRValuePtr>{task}));
            }
            m_lastExprValue = awaited;
            m_lastExprType = asyncAwaitedIRType(kind, structTy);
            return;
        }
    }

    // (3) await on an existing Task value: pump until it completes.
    // The task is NOT destroyed here — its owner keeps it.
    {
        std::string kind;
        if (taskPayloadKind(m_sema.getNodeType(&node.getExpr()), kind)) {
            IRTypePtr structTy = nullptr;
            if (kind == "struct") {
                structTy = asyncStructTypeForTask(m_sema.getNodeType(&node.getExpr()));
                if (!structTy) {
                    reportUnsupported(node.getLocation(), "await",
                        "internal error: the struct type carried by this Task has no IR struct type");
                    node.getExpr().accept(*this);
                    m_lastExprValue = nullptr;
                    m_lastExprType = nullptr;
                    return;
                }
            }
            node.getExpr().accept(*this);
            IRValuePtr task = m_lastExprValue;
            IRValuePtr awaited;
            if (inAsyncSM()) {
                // Phase 5 slice A: suspend through the state machine.
                awaited = asyncSuspendAndLoad(task, kind, structTy);
            } else {
                auto sched = emitAsyncScheduler();
                awaited = emitAwaitTask(sched, task, kind, structTy);
            }
            m_lastExprValue = awaited;
            m_lastExprType = asyncAwaitedIRType(kind, structTy);
            return;
        }
    }

    // (4) Backstop: everything else still fails loudly.
    reportUnsupported(node.getLocation(), "await",
                      "only 'await sleep_async(ms)', 'await delayed_value(ms, value)', "
                      "'await <async fn call>' and 'await <Task<T> value>' are supported in this phase; "
                      "awaiting any other expression is not implemented yet");
    node.getExpr().accept(*this);
    m_lastExprValue = nullptr;
    m_lastExprType = nullptr;
}

void IRBuilder::visit(const ast::ComptimeExpr& node) {
    // Phase 3 slice D (pro-4.0.0): REAL compile-time evaluation. The inner
    // expression (or block) is interpreted by sema::ComptimeEvaluator and
    // the computed value is lowered as an ordinary literal, so the constant
    // is folded into the IR with zero runtime code. Anything the evaluator
    // cannot compute — runtime values, calls to non-comptime functions,
    // unsupported constructs — fails the build loudly instead of passing
    // through as runtime code (the Phase 1 E1000 behaviour is kept for
    // exactly those cases; the evaluator reports the specific reason,
    // which is embedded in the detail below).
    if (!m_comptimeEval) {
        reportUnsupported(node.getLocation(), "comptime",
                          "the compile-time evaluator was not initialized for this build");
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }
    sema::ComptimeValue value = m_comptimeEval->evaluate(node);
    if (m_comptimeEval->hasError() || value.kind == sema::ComptimeKind::Void) {
        std::string detail;
        if (m_comptimeEval->hasError() && !m_comptimeEval->getLastError().empty()) {
            detail = m_comptimeEval->getLastError() + " ";
        } else {
            detail = "the expression could not be evaluated at compile time ";
        }
        detail += "(comptime supports: literal int/float/bool/string/char arithmetic and logic, string concatenation, ternary, arrays + indexing, `.length`, and calls to `comptime fn` functions; anything depending on runtime values fails)";
        reportUnsupported(node.getLocation(), "comptime", detail);
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }
    auto literal = value.toLiteralAST(node.getLocation());
    literal->accept(*this);
}

} // namespace sp::ir
