#include "sp/ir/IRVerifier.h"
#include "sp/ir/IRInstruction.h"
#include <unordered_set>

namespace sp::ir {

bool IRVerifier::verify(const IRModule& module) {
    m_errors.clear();

    // (c) Signature clashes recorded by IRModule::createFunction.
    for (const auto& error : module.getSignatureErrors()) {
        m_errors.push_back(error);
    }

    std::unordered_set<std::string> declaredFunctions;
    for (const auto& func : module.getFunctions()) {
        if (func) {
            declaredFunctions.insert(func->getName());
        }
    }

    for (const auto& func : module.getFunctions()) {
        if (func) {
            verifyFunction(*func);
        }
    }

    // (d) Every direct call must target a declared function. Indirect
    // calls (through a function-pointer value) cannot be checked this
    // way and are skipped.
    for (const auto& func : module.getFunctions()) {
        if (!func || func->isExternal()) {
            continue;
        }
        for (const auto& block : func->getBlocks()) {
            if (!block) {
                continue;
            }
            for (const auto& inst : block->getInstructions()) {
                if (!inst || inst->getOpcode() != IROpcode::Call) {
                    continue;
                }
                const auto* call = dynamic_cast<const CallInst*>(inst.get());
                if (!call || call->isIndirect()) {
                    continue;
                }
                const std::string& callee = call->getCalleeName();
                if (!callee.empty() && declaredFunctions.find(callee) == declaredFunctions.end()) {
                    m_errors.push_back("Function '" + func->getName() +
                                       "': call to undeclared function '" + callee + "'");
                }
            }
        }
    }

    return m_errors.empty();
}

void IRVerifier::verifyFunction(const IRFunction& func) {
    if (func.isExternal()) {
        return;  // Declaration only; no body to check.
    }
    if (func.getBlocks().empty()) {
        m_errors.push_back("Function '" + func.getName() + "' is defined but has no basic blocks");
        return;
    }
    for (const auto& block : func.getBlocks()) {
        if (block) {
            verifyBlock(*block, func.getName());
        }
    }
}

void IRVerifier::verifyBlock(const IRBasicBlock& block, const std::string& funcName) {
    const auto& insts = block.getInstructions();
    if (insts.empty()) {
        m_errors.push_back("Function '" + funcName + "': basic block '" + block.getName() +
                           "' is empty (no terminator)");
        return;
    }
    for (size_t i = 0; i < insts.size(); ++i) {
        const auto& inst = insts[i];
        if (!inst) {
            continue;
        }
        // (b) Phi has no format() case and would print as "unknown_inst";
        // any instruction that formats that way has no valid lowering.
        if (inst->getOpcode() == IROpcode::Phi) {
            m_errors.push_back("Function '" + funcName + "': basic block '" + block.getName() +
                               "' contains a Phi instruction, which has no lowering "
                               "(it would print as 'unknown_inst')");
            continue;
        }
        if (inst->format().find("unknown_inst") != std::string::npos) {
            m_errors.push_back("Function '" + funcName + "': basic block '" + block.getName() +
                               "' contains an instruction with no lowering "
                               "(formats as 'unknown_inst')");
        }
        // (a) Exactly one terminator, and it must be the last instruction.
        if (inst->isTerminator() && i + 1 != insts.size()) {
            m_errors.push_back("Function '" + funcName + "': basic block '" + block.getName() +
                               "' has a terminator before its last instruction");
        }
    }
    if (!block.hasTerminator()) {
        m_errors.push_back("Function '" + funcName + "': basic block '" + block.getName() +
                           "' does not end with a terminator");
    }
}

} // namespace sp::ir
