#pragma once

#include "sp/ir/IRValue.h"
#include <vector>
#include <string>
#include <memory>

namespace sp::ir {

class IRBasicBlock;

enum class IROpcode {
    // Memory
    Alloca,
    Load,
    Store,
    GetElementPtr,

    // Integer Arithmetic
    Add,
    Sub,
    Mul,
    SDiv,
    UDiv,
    SRem,
    URem,
    Neg,

    // Floating Point Arithmetic
    FAdd,
    FSub,
    FMul,
    FDiv,
    FRem,
    FNeg,

    // Bitwise / Boolean
    And,
    Or,
    Xor,
    Shl,
    LShr,
    AShr,
    Not,

    // Integer Comparison
    ICmpEq,
    ICmpNe,
    ICmpSlt,
    ICmpSle,
    ICmpSgt,
    ICmpSge,
    ICmpUlt,
    ICmpUle,
    ICmpUgt,
    ICmpUge,

    // Floating Point Comparison
    FCmpEq,
    FCmpNe,
    FCmpLt,
    FCmpLe,
    FCmpGt,
    FCmpGe,

    // Conversions
    Trunc,
    ZExt,
    SExt,
    FPToSI,
    SIToFP,
    BitCast,

    // Control Flow / Terminators
    Br,
    CondBr,
    Ret,

    // Functions
    Call,

    // SSA
    Phi
};

class IRInstruction : public IRValue {
public:
    IRInstruction(IROpcode opcode, IRTypePtr type, std::string name = "")
        : IRValue(IRValueKind::Instruction, std::move(type), std::move(name)),
          m_opcode(opcode), m_parent(nullptr) {}

    [[nodiscard]] IROpcode getOpcode() const noexcept { return m_opcode; }
    [[nodiscard]] IRBasicBlock* getParent() const noexcept { return m_parent; }
    void setParent(IRBasicBlock* parent) noexcept { m_parent = parent; }

    [[nodiscard]] const std::vector<IRValuePtr>& getOperands() const noexcept { return m_operands; }
    [[nodiscard]] IRValuePtr getOperand(size_t index) const { return m_operands.at(index); }
    void addOperand(IRValuePtr operand) { m_operands.push_back(std::move(operand)); }
    void setOperand(size_t index, IRValuePtr operand) { m_operands.at(index) = std::move(operand); }
    [[nodiscard]] size_t getOperandCount() const noexcept { return m_operands.size(); }

    [[nodiscard]] bool isTerminator() const noexcept {
        return m_opcode == IROpcode::Br || m_opcode == IROpcode::CondBr || m_opcode == IROpcode::Ret;
    }

    // Value reference string (e.g. "%t0")
    [[nodiscard]] std::string toString() const override {
        return getName();
    }

    // Full instruction representation (e.g. "%t0 = add i32 %a, %b")
    [[nodiscard]] std::string format() const;

private:
    IROpcode m_opcode;
    IRBasicBlock* m_parent{nullptr};
    std::vector<IRValuePtr> m_operands;
};

// Specialized Instructions

class AllocaInst : public IRInstruction {
public:
    AllocaInst(IRTypePtr allocatedType, std::string name = "")
        : IRInstruction(IROpcode::Alloca, IRType::getPointer(allocatedType), std::move(name)),
          m_allocatedType(std::move(allocatedType)) {}

    [[nodiscard]] const IRTypePtr& getAllocatedType() const noexcept { return m_allocatedType; }

private:
    IRTypePtr m_allocatedType;
};

class LoadInst : public IRInstruction {
public:
    LoadInst(IRTypePtr resultType, IRValuePtr ptr, std::string name = "")
        : IRInstruction(IROpcode::Load, std::move(resultType), std::move(name)) {
        addOperand(std::move(ptr));
    }

    [[nodiscard]] IRValuePtr getPointer() const { return getOperand(0); }
};

class StoreInst : public IRInstruction {
public:
    StoreInst(IRValuePtr value, IRValuePtr ptr)
        : IRInstruction(IROpcode::Store, IRType::getVoid()) {
        addOperand(std::move(value));
        addOperand(std::move(ptr));
    }

    [[nodiscard]] IRValuePtr getValue() const { return getOperand(0); }
    [[nodiscard]] IRValuePtr getPointer() const { return getOperand(1); }
};

class BinaryInst : public IRInstruction {
public:
    BinaryInst(IROpcode opcode, IRValuePtr lhs, IRValuePtr rhs, std::string name = "")
        : IRInstruction(opcode, lhs->getType(), std::move(name)) {
        addOperand(std::move(lhs));
        addOperand(std::move(rhs));
    }

    [[nodiscard]] IRValuePtr getLHS() const { return getOperand(0); }
    [[nodiscard]] IRValuePtr getRHS() const { return getOperand(1); }
};

class CompareInst : public IRInstruction {
public:
    CompareInst(IROpcode opcode, IRValuePtr lhs, IRValuePtr rhs, std::string name = "")
        : IRInstruction(opcode, IRType::getInt1(), std::move(name)) {
        addOperand(std::move(lhs));
        addOperand(std::move(rhs));
    }

    [[nodiscard]] IRValuePtr getLHS() const { return getOperand(0); }
    [[nodiscard]] IRValuePtr getRHS() const { return getOperand(1); }
};

class CastInst : public IRInstruction {
public:
    CastInst(IROpcode opcode, IRValuePtr value, IRTypePtr destType, std::string name = "")
        : IRInstruction(opcode, std::move(destType), std::move(name)) {
        addOperand(std::move(value));
    }

    [[nodiscard]] IRValuePtr getValue() const { return getOperand(0); }
};

class CallInst : public IRInstruction {
public:
    CallInst(IRTypePtr returnType, std::string calleeName, std::vector<IRValuePtr> args, std::string name = "")
        : IRInstruction(IROpcode::Call, std::move(returnType), std::move(name)),
          m_calleeName(std::move(calleeName)) {
        for (auto& arg : args) {
            addOperand(std::move(arg));
        }
    }

    [[nodiscard]] const std::string& getCalleeName() const noexcept { return m_calleeName; }

private:
    std::string m_calleeName;
};

class BranchInst : public IRInstruction {
public:
    explicit BranchInst(std::string targetLabel)
        : IRInstruction(IROpcode::Br, IRType::getVoid()),
          m_targetLabel(std::move(targetLabel)) {}

    [[nodiscard]] const std::string& getTargetLabel() const noexcept { return m_targetLabel; }

private:
    std::string m_targetLabel;
};

class CondBranchInst : public IRInstruction {
public:
    CondBranchInst(IRValuePtr condition, std::string trueLabel, std::string falseLabel)
        : IRInstruction(IROpcode::CondBr, IRType::getVoid()),
          m_trueLabel(std::move(trueLabel)),
          m_falseLabel(std::move(falseLabel)) {
        addOperand(std::move(condition));
    }

    [[nodiscard]] IRValuePtr getCondition() const { return getOperand(0); }
    [[nodiscard]] const std::string& getTrueLabel() const noexcept { return m_trueLabel; }
    [[nodiscard]] const std::string& getFalseLabel() const noexcept { return m_falseLabel; }

private:
    std::string m_trueLabel;
    std::string m_falseLabel;
};

class ReturnInst : public IRInstruction {
public:
    explicit ReturnInst(IRValuePtr returnValue = nullptr)
        : IRInstruction(IROpcode::Ret, IRType::getVoid()) {
        if (returnValue) {
            addOperand(std::move(returnValue));
        }
    }

    [[nodiscard]] bool hasReturnValue() const noexcept { return getOperandCount() > 0; }
    [[nodiscard]] IRValuePtr getReturnValue() const {
        return hasReturnValue() ? getOperand(0) : nullptr;
    }
};

using IRInstructionPtr = std::shared_ptr<IRInstruction>;

} // namespace sp::ir
