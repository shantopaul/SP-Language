#pragma once

#include "sp/ir/IRBasicBlock.h"
#include <string>
#include <vector>
#include <memory>

namespace sp::ir {

class IRModule;

class IRFunction {
public:
    IRFunction(std::string name,
               IRTypePtr returnType,
               std::vector<std::shared_ptr<ParameterValue>> params,
               bool isExternal = false,
               IRModule* parent = nullptr)
        : m_name(std::move(name)),
          m_returnType(std::move(returnType)),
          m_params(std::move(params)),
          m_isExternal(isExternal),
          m_parent(parent) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const IRTypePtr& getReturnType() const noexcept { return m_returnType; }
    [[nodiscard]] const std::vector<std::shared_ptr<ParameterValue>>& getParams() const noexcept { return m_params; }
    [[nodiscard]] bool isExternal() const noexcept { return m_isExternal; }
    void setExternal(bool isExt) noexcept { m_isExternal = isExt; }

    [[nodiscard]] IRModule* getParent() const noexcept { return m_parent; }
    void setParent(IRModule* parent) noexcept { m_parent = parent; }

    [[nodiscard]] const std::vector<IRBasicBlockPtr>& getBlocks() const noexcept { return m_blocks; }
    [[nodiscard]] std::vector<IRBasicBlockPtr>& getBlocks() noexcept { return m_blocks; }

    IRBasicBlock* createBasicBlock(const std::string& name) {
        auto block = std::make_unique<IRBasicBlock>(name, this);
        auto* rawPtr = block.get();
        m_blocks.push_back(std::move(block));
        return rawPtr;
    }

    [[nodiscard]] IRBasicBlock* getEntryBlock() const noexcept {
        if (m_blocks.empty()) return nullptr;
        return m_blocks.front().get();
    }

    // Register / temp variable name generator: %t0, %t1, ...
    std::string nextTempName(const std::string& prefix = "t") {
        return "%" + prefix + std::to_string(m_tempCounter++);
    }

    void resetTempCounter() noexcept {
        m_tempCounter = 0;
    }

private:
    std::string m_name;
    IRTypePtr m_returnType;
    std::vector<std::shared_ptr<ParameterValue>> m_params;
    bool m_isExternal{false};
    IRModule* m_parent{nullptr};
    std::vector<IRBasicBlockPtr> m_blocks;
    size_t m_tempCounter{0};
};

using IRFunctionPtr = std::unique_ptr<IRFunction>;

} // namespace sp::ir
