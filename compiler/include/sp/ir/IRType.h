#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sp::ir {

enum class IRTypeKind {
    Void,
    Int1,
    Int8,
    Int16,
    Int32,
    Int64,
    Float,
    Double,
    Pointer,
    Function
};

class IRType {
public:
    explicit IRType(IRTypeKind kind) : m_kind(kind) {}
    virtual ~IRType() = default;

    [[nodiscard]] IRTypeKind getKind() const noexcept { return m_kind; }
    [[nodiscard]] virtual std::string toString() const;

    [[nodiscard]] bool isVoid() const noexcept { return m_kind == IRTypeKind::Void; }
    [[nodiscard]] bool isInt1() const noexcept { return m_kind == IRTypeKind::Int1; }
    [[nodiscard]] bool isInt8() const noexcept { return m_kind == IRTypeKind::Int8; }
    [[nodiscard]] bool isInt16() const noexcept { return m_kind == IRTypeKind::Int16; }
    [[nodiscard]] bool isInt32() const noexcept { return m_kind == IRTypeKind::Int32; }
    [[nodiscard]] bool isInt64() const noexcept { return m_kind == IRTypeKind::Int64; }
    [[nodiscard]] bool isFloat() const noexcept { return m_kind == IRTypeKind::Float; }
    [[nodiscard]] bool isDouble() const noexcept { return m_kind == IRTypeKind::Double; }
    [[nodiscard]] bool isPointer() const noexcept { return m_kind == IRTypeKind::Pointer; }
    [[nodiscard]] bool isFunction() const noexcept { return m_kind == IRTypeKind::Function; }

    [[nodiscard]] bool isInteger() const noexcept {
        return m_kind == IRTypeKind::Int1 || m_kind == IRTypeKind::Int8 ||
               m_kind == IRTypeKind::Int16 || m_kind == IRTypeKind::Int32 ||
               m_kind == IRTypeKind::Int64;
    }

    [[nodiscard]] bool isFloatingPoint() const noexcept {
        return m_kind == IRTypeKind::Float || m_kind == IRTypeKind::Double;
    }

    // Factory methods
    static std::shared_ptr<IRType> getVoid();
    static std::shared_ptr<IRType> getInt1();
    static std::shared_ptr<IRType> getInt8();
    static std::shared_ptr<IRType> getInt16();
    static std::shared_ptr<IRType> getInt32();
    static std::shared_ptr<IRType> getInt64();
    static std::shared_ptr<IRType> getFloat();
    static std::shared_ptr<IRType> getDouble();
    static std::shared_ptr<IRType> getPointer(std::shared_ptr<IRType> pointeeType = nullptr);
    static std::shared_ptr<IRType> getFunction(std::shared_ptr<IRType> returnType,
                                               std::vector<std::shared_ptr<IRType>> paramTypes);

private:
    IRTypeKind m_kind;
};

class IRPointerType : public IRType {
public:
    explicit IRPointerType(std::shared_ptr<IRType> pointee = nullptr)
        : IRType(IRTypeKind::Pointer), m_pointee(std::move(pointee)) {}

    [[nodiscard]] const std::shared_ptr<IRType>& getPointeeType() const noexcept { return m_pointee; }
    [[nodiscard]] std::string toString() const override;

private:
    std::shared_ptr<IRType> m_pointee;
};

class IRFunctionType : public IRType {
public:
    IRFunctionType(std::shared_ptr<IRType> returnType, std::vector<std::shared_ptr<IRType>> paramTypes)
        : IRType(IRTypeKind::Function),
          m_returnType(std::move(returnType)),
          m_paramTypes(std::move(paramTypes)) {}

    [[nodiscard]] const std::shared_ptr<IRType>& getReturnType() const noexcept { return m_returnType; }
    [[nodiscard]] const std::vector<std::shared_ptr<IRType>>& getParamTypes() const noexcept { return m_paramTypes; }
    [[nodiscard]] std::string toString() const override;

private:
    std::shared_ptr<IRType> m_returnType;
    std::vector<std::shared_ptr<IRType>> m_paramTypes;
};

using IRTypePtr = std::shared_ptr<IRType>;

} // namespace sp::ir
