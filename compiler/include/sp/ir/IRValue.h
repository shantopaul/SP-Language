#pragma once

#include "sp/ir/IRType.h"
#include <string>
#include <memory>
#include <utility>

namespace sp::ir {

enum class IRValueKind {
    ConstantInt,
    ConstantFloat,
    ConstantBool,
    ConstantString,
    ConstantNull,
    Parameter,
    GlobalVariable,
    Instruction,
    // Phase 3 slice C: the address of a named function, usable as the
    // callee operand of an indirect CallInst. toString() yields "@name".
    FunctionAddress
};

class IRValue : public std::enable_shared_from_this<IRValue> {
public:
    IRValue(IRValueKind kind, IRTypePtr type, std::string name = "")
        : m_kind(kind), m_type(std::move(type)), m_name(std::move(name)) {}

    virtual ~IRValue() = default;

    [[nodiscard]] IRValueKind getValueKind() const noexcept { return m_kind; }
    [[nodiscard]] const IRTypePtr& getType() const noexcept { return m_type; }
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }

    [[nodiscard]] virtual std::string toString() const;

    [[nodiscard]] bool isConstant() const noexcept {
        return m_kind == IRValueKind::ConstantInt ||
               m_kind == IRValueKind::ConstantFloat ||
               m_kind == IRValueKind::ConstantBool ||
               m_kind == IRValueKind::ConstantString ||
               m_kind == IRValueKind::ConstantNull;
    }

private:
    IRValueKind m_kind;
    IRTypePtr m_type;
    std::string m_name;
};

using IRValuePtr = std::shared_ptr<IRValue>;

class ConstantInt : public IRValue {
public:
    ConstantInt(IRTypePtr type, int64_t value)
        : IRValue(IRValueKind::ConstantInt, std::move(type)), m_value(value) {}

    [[nodiscard]] int64_t getValue() const noexcept { return m_value; }
    [[nodiscard]] std::string toString() const override;

private:
    int64_t m_value;
};

class ConstantFloat : public IRValue {
public:
    ConstantFloat(IRTypePtr type, double value)
        : IRValue(IRValueKind::ConstantFloat, std::move(type)), m_value(value) {}

    [[nodiscard]] double getValue() const noexcept { return m_value; }
    [[nodiscard]] std::string toString() const override;

private:
    double m_value;
};

class ConstantBool : public IRValue {
public:
    explicit ConstantBool(bool value)
        : IRValue(IRValueKind::ConstantBool, IRType::getInt1()), m_value(value) {}

    [[nodiscard]] bool getValue() const noexcept { return m_value; }
    [[nodiscard]] std::string toString() const override;

private:
    bool m_value;
};

class ConstantString : public IRValue {
public:
    ConstantString(std::string value, std::string globalName)
        : IRValue(IRValueKind::ConstantString, IRType::getPointer(IRType::getInt8()), std::move(globalName)),
          m_value(std::move(value)) {}

    [[nodiscard]] const std::string& getValue() const noexcept { return m_value; }
    [[nodiscard]] std::string toString() const override;

private:
    std::string m_value;
};

class ConstantNull : public IRValue {
public:
    explicit ConstantNull(IRTypePtr ptrType = nullptr)
        : IRValue(IRValueKind::ConstantNull, ptrType ? std::move(ptrType) : IRType::getPointer()) {}

    [[nodiscard]] std::string toString() const override;
};

class ParameterValue : public IRValue {
public:
    ParameterValue(IRTypePtr type, std::string name, size_t index)
        : IRValue(IRValueKind::Parameter, std::move(type), std::move(name)), m_index(index) {}

    [[nodiscard]] size_t getIndex() const noexcept { return m_index; }
    [[nodiscard]] std::string toString() const override;

private:
    size_t m_index;
};

class GlobalVariable : public IRValue {
public:
    GlobalVariable(IRTypePtr type, std::string name, IRValuePtr initializer = nullptr, bool isConstant = false)
        : IRValue(IRValueKind::GlobalVariable, IRType::getPointer(std::move(type)), std::move(name)),
          m_initializer(std::move(initializer)), m_isConstant(isConstant) {}

    [[nodiscard]] const IRValuePtr& getInitializer() const noexcept { return m_initializer; }
    [[nodiscard]] bool isConstant() const noexcept { return m_isConstant; }
    [[nodiscard]] std::string toString() const override;

private:
    IRValuePtr m_initializer;
    bool m_isConstant;
};

// The address of a named function (e.g. a compiler-generated async
// thunk), used as the callee operand of an indirect CallInst or as a
// plain pointer argument (e.g. the `thunk` argument of the spawn C ABI).
class FunctionAddress : public IRValue {
public:
    explicit FunctionAddress(std::string functionName)
        : IRValue(IRValueKind::FunctionAddress, IRType::getPointer(), "@" + functionName),
          m_functionName(std::move(functionName)) {}

    [[nodiscard]] const std::string& getFunctionName() const noexcept { return m_functionName; }
    [[nodiscard]] std::string toString() const override { return "@" + m_functionName; }

private:
    std::string m_functionName;
};

} // namespace sp::ir
