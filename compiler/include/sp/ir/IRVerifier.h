#pragma once

#include "sp/ir/IRModule.h"
#include <string>
#include <vector>

namespace sp::ir {

// Phase 1 (pro-4.0.0): structural well-formedness checks for SP IR.
//
// The IR previously had no verifier: malformed IR was only caught — if at
// all — by Clang rejecting the emitted .ll text, far from the source error.
// The verifier runs at the end of IRBuilder::build() and again before LLVM
// IR text emission in LLVMGenerator::generateLLVMIR.
//
// Checks:
//   (a) every non-external function has at least one basic block, and every
//       basic block ends with exactly one terminator (Br/CondBr/Ret);
//   (b) no instruction formats as "unknown_inst" — in particular, Phi has
//       no format() case, so any Phi node is rejected with a clear message;
//   (c) signature clashes recorded by IRModule::createFunction (a
//       redefinition with a different signature used to be silently
//       swallowed; the module cannot hold two entries under one name, so
//       the clash is recorded at creation time and surfaced here);
//   (d) every direct call targets a declared function.
//
// The verifier never throws. It returns false and records one
// human-readable message per problem in getErrors().
class IRVerifier {
public:
    // Returns true when the module is well-formed. Call getErrors() for
    // details when it returns false.
    [[nodiscard]] bool verify(const IRModule& module);

    [[nodiscard]] const std::vector<std::string>& getErrors() const noexcept { return m_errors; }

private:
    void verifyFunction(const IRFunction& func);
    void verifyBlock(const IRBasicBlock& block, const std::string& funcName);

    std::vector<std::string> m_errors;
};

} // namespace sp::ir
