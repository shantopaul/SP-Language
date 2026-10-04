#pragma once

#include "sp/ir/IRInstruction.h"
#include <string>
#include <vector>
#include <memory>

namespace sp::ir {

class IRFunction;

class IRBasicBlock {
public:
    IRBasicBlock(std::string name, IRFunction* parent)
        : m_name(std::move(name)), m_parent(parent) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] IRFunction* getParent() const noexcept { return m_parent; }
    void setParent(IRFunction* parent) noexcept { m_parent = parent; }

    [[nodiscard]] const std::vector<IRInstructionPtr>& getInstructions() const noexcept { return m_instructions; }
    [[nodiscard]] std::vector<IRInstructionPtr>& getInstructions() noexcept { return m_instructions; }

    void addInstruction(IRInstructionPtr inst) {
        if (inst) {
            inst->setParent(this);
            m_instructions.push_back(std::move(inst));
        }
    }

    [[nodiscard]] bool hasTerminator() const noexcept {
        return !m_instructions.empty() && m_instructions.back()->isTerminator();
    }

    [[nodiscard]] IRInstructionPtr getTerminator() const noexcept {
        if (hasTerminator()) {
            return m_instructions.back();
        }
        return nullptr;
    }

    [[nodiscard]] bool empty() const noexcept { return m_instructions.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_instructions.size(); }

private:
    std::string m_name;
    IRFunction* m_parent{nullptr};
    std::vector<IRInstructionPtr> m_instructions;
};

using IRBasicBlockPtr = std::unique_ptr<IRBasicBlock>;

} // namespace sp::ir
