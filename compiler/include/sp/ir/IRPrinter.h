#pragma once

#include "sp/ir/IRModule.h"
#include <string>
#include <sstream>

namespace sp::ir {

class IRPrinter {
public:
    IRPrinter() = default;

    std::string print(const IRModule& module);
    std::string print(const IRFunction& function);
    std::string print(const IRBasicBlock& block);
    std::string print(const IRInstruction& instruction);

private:
    void printFunction(const IRFunction& function, std::ostringstream& ss);
    void printBasicBlock(const IRBasicBlock& block, std::ostringstream& ss);
    void printInstruction(const IRInstruction& inst, std::ostringstream& ss);
};

} // namespace sp::ir
