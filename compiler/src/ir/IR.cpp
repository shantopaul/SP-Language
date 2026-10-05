#include "sp/ir/IRType.h"
#include "sp/ir/IRValue.h"
#include "sp/ir/IRInstruction.h"
#include "sp/ir/IRBasicBlock.h"
#include "sp/ir/IRFunction.h"
#include "sp/ir/IRModule.h"
#include <sstream>
#include <iomanip>

namespace sp::ir {

// --- IRType ---

std::shared_ptr<IRType> IRType::getVoid() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Void);
    return inst;
}

std::shared_ptr<IRType> IRType::getInt1() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Int1);
    return inst;
}

std::shared_ptr<IRType> IRType::getInt8() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Int8);
    return inst;
}

std::shared_ptr<IRType> IRType::getInt16() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Int16);
    return inst;
}

std::shared_ptr<IRType> IRType::getInt32() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Int32);
    return inst;
}

std::shared_ptr<IRType> IRType::getInt64() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Int64);
    return inst;
}

std::shared_ptr<IRType> IRType::getFloat() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Float);
    return inst;
}

std::shared_ptr<IRType> IRType::getDouble() {
    static auto inst = std::make_shared<IRType>(IRTypeKind::Double);
    return inst;
}

std::shared_ptr<IRType> IRType::getPointer(std::shared_ptr<IRType> pointeeType) {
    return std::make_shared<IRPointerType>(std::move(pointeeType));
}

std::shared_ptr<IRType> IRType::getFunction(std::shared_ptr<IRType> returnType,
                                           std::vector<std::shared_ptr<IRType>> paramTypes) {
    return std::make_shared<IRFunctionType>(std::move(returnType), std::move(paramTypes));
}

std::shared_ptr<IRStructType> IRType::getStruct(std::string name,
                                               std::vector<std::shared_ptr<IRType>> elementTypes) {
    return std::make_shared<IRStructType>(std::move(name), std::move(elementTypes));
}

std::string IRType::toString() const {
    switch (m_kind) {
        case IRTypeKind::Void: return "void";
        case IRTypeKind::Int1: return "i1";
        case IRTypeKind::Int8: return "i8";
        case IRTypeKind::Int16: return "i16";
        case IRTypeKind::Int32: return "i32";
        case IRTypeKind::Int64: return "i64";
        case IRTypeKind::Float: return "float";
        case IRTypeKind::Double: return "double";
        case IRTypeKind::Pointer: return "ptr";
        case IRTypeKind::Function: return "function";
        case IRTypeKind::Struct: return "struct";
    }
    return "unknown";
}

std::string IRPointerType::toString() const {
    return "ptr";
}

std::string IRFunctionType::toString() const {
    std::string s = m_returnType ? m_returnType->toString() : "void";
    s += "(";
    for (size_t i = 0; i < m_paramTypes.size(); ++i) {
        if (i > 0) s += ", ";
        s += m_paramTypes[i] ? m_paramTypes[i]->toString() : "void";
    }
    s += ")";
    return s;
}

std::string IRStructType::toString() const {
    if (!m_name.empty()) {
        return "%" + m_name;
    }
    std::string s = "{ ";
    for (size_t i = 0; i < m_elementTypes.size(); ++i) {
        if (i > 0) s += ", ";
        s += m_elementTypes[i] ? m_elementTypes[i]->toString() : "void";
    }
    s += " }";
    return s;
}

// --- IRValue ---

std::string IRValue::toString() const {
    if (!m_name.empty()) {
        return m_name;
    }
    return "%val";
}

std::string ConstantInt::toString() const {
    return std::to_string(m_value);
}

std::string ConstantFloat::toString() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << m_value;
    return ss.str();
}

std::string ConstantBool::toString() const {
    return m_value ? "true" : "false";
}

std::string ConstantString::toString() const {
    return getName();
}

std::string ConstantNull::toString() const {
    return "null";
}

std::string ParameterValue::toString() const {
    return getName();
}

std::string GlobalVariable::toString() const {
    return getName();
}

// --- IRInstruction ---

std::string IRInstruction::format() const {
    std::ostringstream ss;
    if (!getName().empty()) {
        ss << getName() << " = ";
    }

    switch (m_opcode) {
        case IROpcode::Alloca: {
            const auto* ai = dynamic_cast<const AllocaInst*>(this);
            ss << "alloca " << (ai && ai->getAllocatedType() ? ai->getAllocatedType()->toString() : "i32");
            break;
        }
        case IROpcode::Load:
            ss << "load " << getType()->toString() << ", ptr " << getOperand(0)->toString();
            break;
        case IROpcode::Store:
            ss << "store " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString()
               << ", ptr " << getOperand(1)->toString();
            break;
        case IROpcode::Add:
            ss << "add " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Sub:
            ss << "sub " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Mul:
            ss << "mul " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::SDiv:
            ss << "sdiv " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::UDiv:
            ss << "udiv " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::SRem:
            ss << "srem " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::URem:
            ss << "urem " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FAdd:
            ss << "fadd " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FSub:
            ss << "fsub " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FMul:
            ss << "fmul " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FDiv:
            ss << "fdiv " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FRem:
            ss << "frem " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Neg:
            ss << "neg " << getType()->toString() << " " << getOperand(0)->toString();
            break;
        case IROpcode::FNeg:
            ss << "fneg " << getType()->toString() << " " << getOperand(0)->toString();
            break;
        case IROpcode::And:
            ss << "and " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Or:
            ss << "or " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Xor:
            ss << "xor " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Shl:
            ss << "shl " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::LShr:
            ss << "lshr " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::AShr:
            ss << "ashr " << getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Not:
            ss << "not " << getType()->toString() << " " << getOperand(0)->toString();
            break;
        case IROpcode::ICmpEq:
            ss << "icmp eq " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpNe:
            ss << "icmp ne " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpSlt:
            ss << "icmp slt " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpSle:
            ss << "icmp sle " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpSgt:
            ss << "icmp sgt " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpSge:
            ss << "icmp sge " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpUlt:
            ss << "icmp ult " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpUle:
            ss << "icmp ule " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpUgt:
            ss << "icmp ugt " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::ICmpUge:
            ss << "icmp uge " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FCmpEq:
            ss << "fcmp oeq " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FCmpNe:
            ss << "fcmp one " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FCmpLt:
            ss << "fcmp olt " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FCmpLe:
            ss << "fcmp ole " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FCmpGt:
            ss << "fcmp ogt " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::FCmpGe:
            ss << "fcmp oge " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << ", " << getOperand(1)->toString();
            break;
        case IROpcode::Trunc:
            ss << "trunc " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::ZExt:
            ss << "zext " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::SExt:
            ss << "sext " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::FPToSI:
            ss << "fptosi " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::SIToFP:
            ss << "sitofp " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::FPTrunc:
            ss << "fptrunc " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::FPExt:
            ss << "fpext " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::BitCast:
            ss << "bitcast " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::GetElementPtr: {
            const auto* gep = dynamic_cast<const GetElementPtrInst*>(this);
            if (gep) {
                ss << "getelementptr inbounds " << gep->getSourceType()->toString() << ", ptr " << gep->getBasePointer()->toString();
                for (const auto& idx : gep->getIndices()) {
                    ss << ", " << idx->getType()->toString() << " " << idx->toString();
                }
            }
            break;
        }
        case IROpcode::Br: {
            const auto* br = dynamic_cast<const BranchInst*>(this);
            ss << "br label %" << (br ? br->getTargetLabel() : "unknown");
            break;
        }
        case IROpcode::CondBr: {
            const auto* cbr = dynamic_cast<const CondBranchInst*>(this);
            ss << "br i1 " << getOperand(0)->toString() << ", label %"
               << (cbr ? cbr->getTrueLabel() : "true") << ", label %"
               << (cbr ? cbr->getFalseLabel() : "false");
            break;
        }
        case IROpcode::Ret: {
            const auto* ret = dynamic_cast<const ReturnInst*>(this);
            if (ret && ret->hasReturnValue()) {
                ss << "ret " << ret->getReturnValue()->getType()->toString() << " " << ret->getReturnValue()->toString();
            } else {
                ss << "ret void";
            }
            break;
        }
        case IROpcode::Call: {
            const auto* call = dynamic_cast<const CallInst*>(this);
            if (call && call->isIndirect()) {
                ss << "call " << getType()->toString() << " " << call->getCalleeValue()->toString() << "(";
                for (size_t i = 1; i < getOperandCount(); ++i) {
                    if (i > 1) ss << ", ";
                    ss << getOperand(i)->getType()->toString() << " " << getOperand(i)->toString();
                }
                ss << ")";
            } else {
                ss << "call " << getType()->toString() << " @" << (call ? call->getCalleeName() : "fn") << "(";
                for (size_t i = 0; i < getOperandCount(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << getOperand(i)->getType()->toString() << " " << getOperand(i)->toString();
                }
                ss << ")";
            }
            break;
        }
        default:
            ss << "unknown_inst";
            break;
    }
    return ss.str();
}

// --- IRModule Runtime Declarations ---

void IRModule::initRuntimeDeclarations() {
    // void sp_rt_println_string(ptr)
    createFunction("sp_rt_println_string", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "str", 0)},
                   true);

    // void sp_rt_println_int(i64)
    createFunction("sp_rt_println_int", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "val", 0)},
                   true);

    // void sp_rt_println_double(double)
    createFunction("sp_rt_println_double", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "val", 0)},
                   true);

    // void sp_rt_println_bool(i1)
    createFunction("sp_rt_println_bool", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt1(), "val", 0)},
                   true);

    // void sp_rt_print_string(ptr)
    createFunction("sp_rt_print_string", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "str", 0)},
                   true);

    // void sp_rt_print_int(i64)
    createFunction("sp_rt_print_int", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "val", 0)},
                   true);

    // void sp_rt_assert(i1, ptr)
    createFunction("sp_rt_assert", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt1(), "cond", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "msg", 1)},
                   true);

    // void sp_rt_panic(ptr)
    createFunction("sp_rt_panic", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "msg", 0)},
                   true);

    // ptr sp_rt_alloc(i64)
    createFunction("sp_rt_alloc", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "size", 0)},
                   true);

    // void sp_rt_free(ptr)
    createFunction("sp_rt_free", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "ptr", 0)},
                   true);
}

} // namespace sp::ir
