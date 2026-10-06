#include "sp/ir/IRType.h"
#include "sp/ir/IRValue.h"
#include "sp/ir/IRInstruction.h"
#include "sp/ir/IRBasicBlock.h"
#include "sp/ir/IRFunction.h"
#include "sp/ir/IRModule.h"
#include <sstream>
#include <iomanip>
#include <cstring>

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
    double dval = m_value;
    if (getType() && getType()->isFloat()) {
        float fval = static_cast<float>(m_value);
        dval = static_cast<double>(fval);
    }
    uint64_t bits = 0;
    std::memcpy(&bits, &dval, sizeof(bits));
    std::ostringstream ss;
    ss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << bits;
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
        case IROpcode::PtrToInt:
            ss << "ptrtoint " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
            break;
        case IROpcode::IntToPtr:
            ss << "inttoptr " << getOperand(0)->getType()->toString() << " " << getOperand(0)->toString() << " to " << getType()->toString();
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

    // void sp_rt_println_uint(i64)
    createFunction("sp_rt_println_uint", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "val", 0)},
                   true);

    // void sp_rt_println_float(float)
    createFunction("sp_rt_println_float", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getFloat(), "val", 0)},
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

    // void sp_rt_print_uint(i64)
    createFunction("sp_rt_print_uint", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "val", 0)},
                   true);

    // void sp_rt_print_float(float)
    createFunction("sp_rt_print_float", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getFloat(), "val", 0)},
                   true);

    // void sp_rt_print_double(double)
    createFunction("sp_rt_print_double", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "val", 0)},
                   true);

    // void sp_rt_print_bool(i1)
    createFunction("sp_rt_print_bool", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt1(), "val", 0)},
                   true);

    // void sp_rt_print_char(i8)
    createFunction("sp_rt_print_char", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt8(), "val", 0)},
                   true);

    // void sp_rt_println_char(i8)
    createFunction("sp_rt_println_char", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt8(), "val", 0)},
                   true);

    // ptr sp_rt_read_line()
    createFunction("sp_rt_read_line", IRType::getPointer(IRType::getInt8()),
                   {},
                   true);

    // ptr sp_rt_readln()
    createFunction("sp_rt_readln", IRType::getPointer(IRType::getInt8()),
                   {},
                   true);

    // void sp_rt_eprint_string(ptr)
    createFunction("sp_rt_eprint_string", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "str", 0)},
                   true);

    // void sp_rt_eprintln_string(ptr)
    createFunction("sp_rt_eprintln_string", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "str", 0)},
                   true);

    // void sp_rt_flush()
    createFunction("sp_rt_flush", IRType::getVoid(),
                   {},
                   true);

    // void sp_rt_eflush()
    createFunction("sp_rt_eflush", IRType::getVoid(),
                   {},
                   true);

    // i64 sp_rt_str_to_int(ptr)
    createFunction("sp_rt_str_to_int", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // double sp_rt_str_to_double(ptr)
    createFunction("sp_rt_str_to_double", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // float sp_rt_str_to_float(ptr)
    createFunction("sp_rt_str_to_float", IRType::getFloat(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // i1 sp_rt_str_to_bool(ptr)
    createFunction("sp_rt_str_to_bool", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
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

    // ptr sp_rt_str_concat(ptr, ptr)
    createFunction("sp_rt_str_concat", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s1", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s2", 1)},
                   true);

    // i1 sp_rt_str_equals(ptr, ptr)
    createFunction("sp_rt_str_equals", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s1", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s2", 1)},
                   true);

    // ptr sp_rt_str_from_int(i64)
    createFunction("sp_rt_str_from_int", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "val", 0)},
                   true);

    // ptr sp_rt_str_from_uint(i64)
    createFunction("sp_rt_str_from_uint", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "val", 0)},
                   true);

    // ptr sp_rt_str_from_float(float)
    createFunction("sp_rt_str_from_float", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getFloat(), "val", 0)},
                   true);

    // ptr sp_rt_str_from_double(double)
    createFunction("sp_rt_str_from_double", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "val", 0)},
                   true);

    // ptr sp_rt_str_from_bool(i1)
    createFunction("sp_rt_str_from_bool", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getInt1(), "val", 0)},
                   true);

    // ptr sp_rt_str_from_char(i8)
    createFunction("sp_rt_str_from_char", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getInt8(), "val", 0)},
                   true);

    // i64 sp_rt_str_len(ptr)
    createFunction("sp_rt_str_len", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // ptr sp_rt_str_upper(ptr)
    createFunction("sp_rt_str_upper", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // ptr sp_rt_str_lower(ptr)
    createFunction("sp_rt_str_lower", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // ptr sp_rt_str_trim(ptr)
    createFunction("sp_rt_str_trim", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // i1 sp_rt_str_contains(ptr, ptr)
    createFunction("sp_rt_str_contains", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "sub", 1)},
                   true);

    // i1 sp_rt_str_starts_with(ptr, ptr)
    createFunction("sp_rt_str_starts_with", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "prefix", 1)},
                   true);

    // i1 sp_rt_str_ends_with(ptr, ptr)
    createFunction("sp_rt_str_ends_with", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "suffix", 1)},
                   true);

    // i64 sp_rt_str_index_of(ptr, ptr)
    createFunction("sp_rt_str_index_of", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "sub", 1)},
                   true);

    // ptr sp_rt_str_substring(ptr, i64, i64)
    createFunction("sp_rt_str_substring", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "start", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "end", 2)},
                   true);

    // ptr sp_rt_str_replace(ptr, ptr, ptr)
    createFunction("sp_rt_str_replace", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "from", 1),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "to", 2)},
                   true);

    // i8 sp_rt_str_char_at(ptr, i64)
    createFunction("sp_rt_str_char_at", IRType::getInt8(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "idx", 1)},
                   true);

    // ptr sp_rt_str_split(ptr, ptr)
    createFunction("sp_rt_str_split", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "delim", 1)},
                   true);

    // ptr sp_rt_str_reverse(ptr)
    createFunction("sp_rt_str_reverse", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // ptr sp_rt_str_repeat(ptr, i64)
    createFunction("sp_rt_str_repeat", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "count", 1)},
                   true);

    // i1 sp_rt_str_is_empty(ptr)
    createFunction("sp_rt_str_is_empty", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "s", 0)},
                   true);

    // Math runtime functions
    createFunction("sp_rt_math_sqrt", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_pow", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "base", 0),
                    std::make_shared<ParameterValue>(IRType::getDouble(), "exp", 1)}, true);
    createFunction("sp_rt_math_sin", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_cos", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_tan", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_asin", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_acos", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_atan", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_atan2", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "y", 0),
                    std::make_shared<ParameterValue>(IRType::getDouble(), "x", 1)}, true);
    createFunction("sp_rt_math_floor", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_ceil", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_round", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_abs_double", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_abs_int", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "x", 0)}, true);
    createFunction("sp_rt_math_min_double", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "a", 0),
                    std::make_shared<ParameterValue>(IRType::getDouble(), "b", 1)}, true);
    createFunction("sp_rt_math_min_int", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "a", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "b", 1)}, true);
    createFunction("sp_rt_math_max_double", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "a", 0),
                    std::make_shared<ParameterValue>(IRType::getDouble(), "b", 1)}, true);
    createFunction("sp_rt_math_max_int", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "a", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "b", 1)}, true);
    createFunction("sp_rt_math_log", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_log10", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_log2", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);
    createFunction("sp_rt_math_exp", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getDouble(), "x", 0)}, true);

    // File system runtime functions (std.fs)
    createFunction("sp_rt_fs_read_text", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0)}, true);
    createFunction("sp_rt_fs_write_text", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "content", 1)}, true);
    createFunction("sp_rt_fs_append_text", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "content", 1)}, true);
    createFunction("sp_rt_fs_exists", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0)}, true);
    createFunction("sp_rt_fs_delete", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0)}, true);
    createFunction("sp_rt_fs_create_dir", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0)}, true);
    createFunction("sp_rt_fs_list_dir", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0)}, true);
    createFunction("sp_rt_fs_is_file", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0)}, true);
    createFunction("sp_rt_fs_is_dir", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "path", 0)}, true);

    // Time runtime functions (std.time)
    createFunction("sp_rt_time_now_ms", IRType::getInt64(), {}, true);
    createFunction("sp_rt_time_now_secs", IRType::getInt64(), {}, true);
    createFunction("sp_rt_time_now_ns", IRType::getInt64(), {}, true);
    createFunction("sp_rt_time_sleep_ms", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "ms", 0)}, true);
    createFunction("sp_rt_time_sleep_secs", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "s", 0)}, true);
    createFunction("sp_rt_time_format_iso", IRType::getPointer(IRType::getInt8()),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "epoch_ms", 0)}, true);
    createFunction("sp_rt_time_parse_iso", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(IRType::getInt8()), "iso_str", 0)}, true);

    // Duration runtime functions (std.time)
    createFunction("sp_rt_duration_from_millis", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "ms", 0)}, true);
    createFunction("sp_rt_duration_from_seconds", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "s", 0)}, true);
    createFunction("sp_rt_duration_from_minutes", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "m", 0)}, true);
    createFunction("sp_rt_duration_from_hours", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "h", 0)}, true);
    createFunction("sp_rt_duration_to_millis", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "d", 0)}, true);
    createFunction("sp_rt_duration_to_seconds", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "d", 0)}, true);

    // Native Array primitives
    // ptr sp_rt_array_alloc(i64, i64)
    createFunction("sp_rt_array_alloc", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "length", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "elem_size", 1)},
                   true);

    // i64 sp_rt_array_length(ptr)
    createFunction("sp_rt_array_length", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0)},
                   true);

    // ptr sp_rt_array_data(ptr)
    createFunction("sp_rt_array_data", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0)},
                   true);

    // ptr sp_rt_array_get_ptr(ptr, i64, i64)
    createFunction("sp_rt_array_get_ptr", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "elem_size", 2)},
                   true);

    // i64 sp_rt_array_get_i64(ptr, i64)
    createFunction("sp_rt_array_get_i64", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1)},
                   true);

    // void sp_rt_array_set_i64(ptr, i64, i64)
    createFunction("sp_rt_array_set_i64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 2)},
                   true);

    // double sp_rt_array_get_f64(ptr, i64)
    createFunction("sp_rt_array_get_f64", IRType::getDouble(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1)},
                   true);

    // void sp_rt_array_set_f64(ptr, i64, double)
    createFunction("sp_rt_array_set_f64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1),
                    std::make_shared<ParameterValue>(IRType::getDouble(), "val", 2)},
                   true);

    // Native List<T> primitives
    // ptr sp_rt_list_alloc(i64, i64)
    createFunction("sp_rt_list_alloc", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getInt64(), "elem_size", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "init_cap", 1)},
                   true);

    // i64 sp_rt_list_size(ptr)
    createFunction("sp_rt_list_size", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0)},
                   true);

    // void sp_rt_list_push_i64(ptr, i64)
    createFunction("sp_rt_list_push_i64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 1)},
                   true);

    // i64 sp_rt_list_get_i64(ptr, i64)
    createFunction("sp_rt_list_get_i64", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1)},
                   true);

    // void sp_rt_list_set_i64(ptr, i64, i64)
    createFunction("sp_rt_list_set_i64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 2)},
                   true);

    // i64 sp_rt_list_pop_i64(ptr)
    createFunction("sp_rt_list_pop_i64", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0)},
                   true);

    // Native Map<K, V> primitives
    // ptr sp_rt_map_alloc()
    createFunction("sp_rt_map_alloc", IRType::getPointer(), {}, true);

    // i64 sp_rt_map_size(ptr)
    createFunction("sp_rt_map_size", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0)},
                   true);

    // void sp_rt_map_set_str(ptr, ptr, i64)
    createFunction("sp_rt_map_set_str", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "key", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 2)},
                   true);

    // i64 sp_rt_map_get_str(ptr, ptr)
    createFunction("sp_rt_map_get_str", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "key", 1)},
                   true);

    // i1 sp_rt_map_contains_str(ptr, ptr)
    createFunction("sp_rt_map_contains_str", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "key", 1)},
                   true);

    // void sp_rt_map_set_i64(ptr, i64, i64)
    createFunction("sp_rt_map_set_i64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "key", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 2)},
                   true);

    // i64 sp_rt_map_get_i64(ptr, i64)
    createFunction("sp_rt_map_get_i64", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "key", 1)},
                   true);

    // Native Set<T> declarations
    createFunction("sp_rt_set_alloc", IRType::getPointer(), {}, true);

    createFunction("sp_rt_set_size", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0)},
                   true);

    createFunction("sp_rt_set_add_i64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 1)},
                   true);

    createFunction("sp_rt_set_add_str", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "val", 1)},
                   true);

    createFunction("sp_rt_set_contains_i64", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 1)},
                   true);

    createFunction("sp_rt_set_contains_str", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "val", 1)},
                   true);

    createFunction("sp_rt_set_remove_i64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 1)},
                   true);

    createFunction("sp_rt_set_remove_str", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "val", 1)},
                   true);

    createFunction("sp_rt_set_clear", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0)}, true);

    createFunction("sp_rt_set_is_empty", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "set", 0)}, true);

    createFunction("sp_rt_list_clear", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0)}, true);

    createFunction("sp_rt_list_contains_i64", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "val", 1)}, true);

    createFunction("sp_rt_list_remove_at", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "index", 1)}, true);

    createFunction("sp_rt_list_is_empty", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0)}, true);

    createFunction("sp_rt_map_contains_i64", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "key", 1)}, true);

    createFunction("sp_rt_map_remove_str", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "key", 1)}, true);

    createFunction("sp_rt_map_remove_i64", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "key", 1)}, true);

    createFunction("sp_rt_map_clear", IRType::getVoid(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0)}, true);

    createFunction("sp_rt_map_is_empty", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "map", 0)}, true);

    createFunction("sp_rt_array_is_empty", IRType::getInt1(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0)}, true);

    // Higher-order collection functions
    createFunction("sp_rt_array_map", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "fn", 1)},
                   true);

    createFunction("sp_rt_array_filter", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "fn", 1)},
                   true);

    createFunction("sp_rt_array_reduce", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "arr", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "fn", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "init", 2)},
                   true);

    createFunction("sp_rt_list_map", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "fn", 1)},
                   true);

    createFunction("sp_rt_list_filter", IRType::getPointer(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "fn", 1)},
                   true);

    createFunction("sp_rt_list_reduce", IRType::getInt64(),
                   {std::make_shared<ParameterValue>(IRType::getPointer(), "list", 0),
                    std::make_shared<ParameterValue>(IRType::getPointer(), "fn", 1),
                    std::make_shared<ParameterValue>(IRType::getInt64(), "init", 2)},
                   true);
}

} // namespace sp::ir
