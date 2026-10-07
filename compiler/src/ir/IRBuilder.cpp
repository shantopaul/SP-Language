#include "sp/ir/IRBuilder.h"
#include "sp/codegen/VTableBuilder.h"
#include "sp/codegen/ClassGenerator.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace sp::ir {

static IRValuePtr makeDefaultValue(const IRTypePtr& type) {
    if (!type || type->isVoid()) return nullptr;
    if (type->isStruct() || type->isPointer()) {
        return std::make_shared<ConstantNull>(type);
    } else if (type->isFloatingPoint()) {
        return std::make_shared<ConstantFloat>(type, 0.0);
    } else {
        return std::make_shared<ConstantInt>(type, 0);
    }
}

IRValuePtr IRBuilder::emitCreateTaggedUnion(const std::string& typeName, int32_t tag, IRValuePtr payloadVal) {
    std::string structName = typeName;
    if (structName.rfind("Result", 0) == 0) structName = "Result";
    else if (structName.rfind("Option", 0) == 0) structName = "Option";
    auto structType = ir::IRType::getStruct("struct." + structName, {IRType::getInt32(), IRType::getInt64()});

    std::string allocaReg = m_currentFunction->nextTempName("tu_alloca");
    auto allocaInst = std::make_shared<AllocaInst>(structType, allocaReg);
    m_currentFunction->getEntryBlock()->addInstruction(allocaInst);

    auto tagPtr = std::make_shared<GetElementPtrInst>(
        structType,
        allocaInst,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
        },
        m_currentFunction->nextTempName("tag_ptr")
    );
    m_currentBlock->addInstruction(tagPtr);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantInt>(IRType::getInt32(), tag),
        tagPtr
    ));

    IRValuePtr payloadI64 = nullptr;
    if (payloadVal) {
        payloadI64 = emitCastIfNeeded(payloadVal, IRType::getInt64());
    } else {
        payloadI64 = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
    }
    auto payloadPtr = std::make_shared<GetElementPtrInst>(
        structType,
        allocaInst,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 1)
        },
        m_currentFunction->nextTempName("payload_ptr")
    );
    m_currentBlock->addInstruction(payloadPtr);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(payloadI64, payloadPtr));

    return allocaInst;
}

IRBuilder::IRBuilder(const sema::SemanticAnalyzer& sema)
    : m_sema(sema) {}

IRBuilder::~IRBuilder() = default;

std::unique_ptr<IRModule> IRBuilder::build(const ast::Program& program, const std::string& moduleName) {
    m_module = std::make_unique<IRModule>(moduleName);
    m_blockCounter = 0;
    m_varCounter = 0;
    m_scopes.clear();
    m_breakStack.clear();
    m_continueStack.clear();
    m_currentClass = nullptr;

    // Build VTables and Class layouts
    m_vtableBuilder = std::make_unique<codegen::VTableBuilder>(m_sema);
    m_vtableBuilder->buildProgramLayouts(program, *m_module);

    visit(program);
    return std::move(m_module);
}

void IRBuilder::pushScope() {
    m_scopes.emplace_back();
    m_refCountedScopes.emplace_back();
}

void IRBuilder::emitScopeDrops(size_t scopeIndex) {
    if (scopeIndex >= m_refCountedScopes.size()) return;
    if (!m_currentBlock || m_currentBlock->hasTerminator()) return;

    const auto& vars = m_refCountedScopes[scopeIndex];
    for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
        const auto& [name, allocaPtr] = *it;
        std::string dropReg = m_currentFunction->nextTempName("drop_" + name);
        auto loadInst = std::make_shared<LoadInst>(IRType::getPointer(), allocaPtr, dropReg);
        m_currentBlock->addInstruction(loadInst);

        auto releaseCall = std::make_shared<CallInst>(
            IRType::getVoid(),
            "sp_rt_release",
            std::vector<IRValuePtr>{loadInst}
        );
        m_currentBlock->addInstruction(releaseCall);
    }
}

void IRBuilder::emitAllFunctionDrops() {
    if (!m_currentBlock || m_currentBlock->hasTerminator()) return;
    for (int i = static_cast<int>(m_refCountedScopes.size()) - 1; i >= 0; --i) {
        emitScopeDrops(static_cast<size_t>(i));
    }
}

void IRBuilder::popScope() {
    if (!m_refCountedScopes.empty()) {
        emitScopeDrops(m_refCountedScopes.size() - 1);
        m_refCountedScopes.pop_back();
    }
    if (!m_scopes.empty()) {
        m_scopes.pop_back();
    }
}

void IRBuilder::declareVariable(const std::string& name, IRValuePtr allocaPtr, IRTypePtr valType) {
    if (m_scopes.empty()) {
        pushScope();
    }
    m_scopes.back()[name] = {std::move(allocaPtr), std::move(valType)};
}

std::pair<IRValuePtr, IRTypePtr> IRBuilder::lookupVariable(const std::string& name) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return {nullptr, nullptr};
}

IRBasicBlock* IRBuilder::createBlock(const std::string& name) {
    std::string blockName = name + "_" + std::to_string(m_blockCounter++);
    return m_currentFunction->createBasicBlock(blockName);
}

void IRBuilder::setInsertPoint(IRBasicBlock* block) {
    m_currentBlock = block;
}

IRValuePtr IRBuilder::emitCastIfNeeded(IRValuePtr val, IRTypePtr targetType) {
    if (!val || !targetType || val->getType()->getKind() == targetType->getKind()) {
        return val;
    }

    auto srcType = val->getType();
    std::string tempName = m_currentFunction ? m_currentFunction->nextTempName("cast") : "%cast";

    if (srcType->isInteger() && targetType->isInteger()) {
        if (auto constInt = std::dynamic_pointer_cast<ConstantInt>(val)) {
            return std::make_shared<ConstantInt>(targetType, constInt->getValue());
        }

        size_t srcBits = 32;
        if (srcType->isInt1()) srcBits = 1;
        else if (srcType->isInt8()) srcBits = 8;
        else if (srcType->isInt16()) srcBits = 16;
        else if (srcType->isInt32()) srcBits = 32;
        else if (srcType->isInt64()) srcBits = 64;

        size_t dstBits = 32;
        if (targetType->isInt1()) dstBits = 1;
        else if (targetType->isInt8()) dstBits = 8;
        else if (targetType->isInt16()) dstBits = 16;
        else if (targetType->isInt32()) dstBits = 32;
        else if (targetType->isInt64()) dstBits = 64;

        if (srcBits < dstBits) {
            auto cast = std::make_shared<CastInst>(IROpcode::SExt, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        } else if (srcBits > dstBits) {
            auto cast = std::make_shared<CastInst>(IROpcode::Trunc, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        }
    }

    if (srcType->isInteger() && targetType->isFloatingPoint()) {
        if (auto constInt = std::dynamic_pointer_cast<ConstantInt>(val)) {
            return std::make_shared<ConstantFloat>(targetType, static_cast<double>(constInt->getValue()));
        }
        auto cast = std::make_shared<CastInst>(IROpcode::SIToFP, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isFloatingPoint() && targetType->isInteger()) {
        if (auto constFloat = std::dynamic_pointer_cast<ConstantFloat>(val)) {
            return std::make_shared<ConstantInt>(targetType, static_cast<int64_t>(constFloat->getValue()));
        }
        auto cast = std::make_shared<CastInst>(IROpcode::FPToSI, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isPointer() && targetType->isInteger()) {
        auto cast = std::make_shared<CastInst>(IROpcode::PtrToInt, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isInteger() && targetType->isPointer()) {
        auto cast = std::make_shared<CastInst>(IROpcode::IntToPtr, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isFloatingPoint() && targetType->isFloatingPoint()) {
        if (auto constFloat = std::dynamic_pointer_cast<ConstantFloat>(val)) {
            return std::make_shared<ConstantFloat>(targetType, constFloat->getValue());
        }
        if (srcType->isDouble() && targetType->isFloat()) {
            auto cast = std::make_shared<CastInst>(IROpcode::FPTrunc, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        } else if (srcType->isFloat() && targetType->isDouble()) {
            auto cast = std::make_shared<CastInst>(IROpcode::FPExt, val, targetType, tempName);
            if (m_currentBlock) m_currentBlock->addInstruction(cast);
            return cast;
        }
    }

    return val;
}

IRValuePtr IRBuilder::convertToStringValue(IRValuePtr val, IRTypePtr valType, const ast::ExpressionNode* exprNode) {
    if (!val || !valType) return m_module->getOrCreateStringLiteral("");
    if (valType->isPointer()) {
        return val;
    }
    std::string callReg = m_currentFunction ? m_currentFunction->nextTempName("str") : "%str";
    auto semaType = exprNode ? m_sema.getNodeType(exprNode) : nullptr;
    if (valType->isInt1() || (semaType && semaType->getKind() == sema::TypeKind::Bool)) {
        val = emitCastIfNeeded(val, IRType::getInt1());
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_bool",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }
    if (semaType && semaType->isChar()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_char",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (valType->isFloat()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_float",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (valType->isDouble()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_double",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (semaType && semaType->isUnsignedInteger()) {
        if (auto constInt = std::dynamic_pointer_cast<ConstantInt>(val)) {
            uint64_t uval = static_cast<uint64_t>(constInt->getValue());
            if (valType->isInt8()) uval &= 0xFFULL;
            else if (valType->isInt16()) uval &= 0xFFFFULL;
            else if (valType->isInt32()) uval &= 0xFFFFFFFFULL;
            val = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(uval));
        } else if (!valType->isInt64()) {
            std::string zextName = m_currentFunction ? m_currentFunction->nextTempName("zext") : "%zext";
            auto zext = std::make_shared<CastInst>(IROpcode::ZExt, val, IRType::getInt64(), zextName);
            if (m_currentBlock) m_currentBlock->addInstruction(zext);
            val = zext;
        }
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_uint",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    if (!semaType && valType->isInt8()) {
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_from_char",
                                               std::vector<IRValuePtr>{val},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        return call;
    }

    val = emitCastIfNeeded(val, IRType::getInt64());
    auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                           "sp_rt_str_from_int",
                                           std::vector<IRValuePtr>{val},
                                           callReg);
    if (m_currentBlock) m_currentBlock->addInstruction(call);
    return call;
}

IRTypePtr IRBuilder::mapType(const sema::Type* type) const {
    if (!type) return IRType::getInt32();

    switch (type->getKind()) {
        case sema::TypeKind::Void: return IRType::getVoid();
        case sema::TypeKind::Bool: return IRType::getInt1();
        case sema::TypeKind::Int8:
        case sema::TypeKind::UInt8:
        case sema::TypeKind::Char: return IRType::getInt8();
        case sema::TypeKind::Int16:
        case sema::TypeKind::UInt16: return IRType::getInt16();
        case sema::TypeKind::Int32:
        case sema::TypeKind::UInt32: return IRType::getInt32();
        case sema::TypeKind::Int64:
        case sema::TypeKind::UInt64: return IRType::getInt64();
        case sema::TypeKind::Float: return IRType::getFloat();
        case sema::TypeKind::Double: return IRType::getDouble();
        case sema::TypeKind::String: return IRType::getPointer(IRType::getInt8());
        case sema::TypeKind::Enum: {
            const auto* en = static_cast<const sema::EnumType*>(type);
            if (en && en->isTaggedUnion()) {
                std::string name = en->getName();
                if (name.rfind("Result", 0) == 0) name = "Result";
                else if (name.rfind("Option", 0) == 0) name = "Option";
                return IRType::getStruct("struct." + name, {IRType::getInt32(), IRType::getInt64()});
            }
            return IRType::getInt32();
        }
        case sema::TypeKind::Array:
        case sema::TypeKind::Map:
        case sema::TypeKind::Class:
        case sema::TypeKind::Interface:
        case sema::TypeKind::Struct:
        case sema::TypeKind::Pointer:
            return IRType::getPointer();
        case sema::TypeKind::Nullable: {
            const auto* nullT = static_cast<const sema::NullableType*>(type);
            if (nullT && nullT->getUnderlyingType()) {
                return mapType(nullT->getUnderlyingType().get());
            }
            return IRType::getPointer();
        }
        default: return IRType::getInt32();
    }
}

IRTypePtr IRBuilder::mapType(const std::string& typeName) const {
    if (typeName.empty() || typeName == "void") return IRType::getVoid();
    if (typeName == "bool") return IRType::getInt1();
    if (typeName == "byte" || typeName == "char" || typeName == "int8" || typeName == "uint8") return IRType::getInt8();
    if (typeName == "short" || typeName == "int16" || typeName == "uint16") return IRType::getInt16();
    if (typeName == "int" || typeName == "int32" || typeName == "uint" || typeName == "uint32") return IRType::getInt32();
    if (typeName == "long" || typeName == "int64" || typeName == "uint64") return IRType::getInt64();
    if (typeName == "float") return IRType::getFloat();
    if (typeName == "double") return IRType::getDouble();
    if (typeName == "string") return IRType::getPointer(IRType::getInt8());
    if (typeName == "ptr" || typeName.rfind("ptr<", 0) == 0) return IRType::getPointer();

    if (typeName == "Result" || typeName.rfind("Result<", 0) == 0) {
        return IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
    }
    if (typeName == "Option" || typeName.rfind("Option<", 0) == 0) {
        return IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
    }

    auto userType = m_sema.getTypeContext().findUserType(typeName);
    if (userType && userType->isEnum()) {
        auto en = std::static_pointer_cast<sema::EnumType>(userType);
        if (en && en->isTaggedUnion()) {
            std::string name = en->getName();
            if (name.rfind("Result", 0) == 0) name = "Result";
            else if (name.rfind("Option", 0) == 0) name = "Option";
            return IRType::getStruct("struct." + name, {IRType::getInt32(), IRType::getInt64()});
        }
        return IRType::getInt32();
    }

    // Object/class/struct/interface reference
    return IRType::getPointer();
}

// --- AST Visitors ---

void IRBuilder::visit(const ast::Program& node) {
    // Check if there is an explicit main function
    bool hasMainFunc = false;
    std::vector<const ast::StatementNode*> topLevelStmts;

    for (const auto& decl : node.getDeclarations()) {
        if (auto* fn = dynamic_cast<const ast::FunctionDecl*>(decl.get())) {
            if (fn->getName() == "main") {
                hasMainFunc = true;
            }
        }
    }

    // Collect top-level statements outside of function/class/struct declarations
    for (const auto& decl : node.getDeclarations()) {
        if (auto* stmt = dynamic_cast<const ast::StatementNode*>(decl.get())) {
            if (dynamic_cast<const ast::FunctionDecl*>(stmt) ||
                dynamic_cast<const ast::ClassDecl*>(stmt) ||
                dynamic_cast<const ast::InterfaceDecl*>(stmt) ||
                dynamic_cast<const ast::StructDecl*>(stmt) ||
                dynamic_cast<const ast::EnumDecl*>(stmt) ||
                dynamic_cast<const ast::ModuleDecl*>(stmt) ||
                dynamic_cast<const ast::ImportDecl*>(stmt)) {
                continue;
            }
            topLevelStmts.push_back(stmt);
        }
    }

    // Index functions and generic classes/functions
    for (const auto& decl : node.getDeclarations()) {
        if (const auto* cls = dynamic_cast<const ast::ClassDecl*>(decl.get())) {
            if (cls->isGeneric()) {
                m_genericClasses[cls->getName()] = cls;
            }
        } else if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(decl.get())) {
            m_astFunctions[fn->getName()] = fn;
            if (fn->isGeneric()) {
                m_genericFunctions[fn->getName()] = fn;
            }
        } else if (const auto* block = dynamic_cast<const ast::BlockStmt*>(decl.get())) {
            for (const auto& inner : block->getStatements()) {
                if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(inner.get())) {
                    m_astFunctions[fn->getName()] = fn;
                }
            }
        }
    }

    // First visit class declarations (generates constructors & methods)
    for (const auto& decl : node.getDeclarations()) {
        if (const auto* cls = dynamic_cast<const ast::ClassDecl*>(decl.get())) {
            if (cls->isGeneric()) {
                // Compile all specializations of this generic class registered in sema
                for (const auto& [typeName, userType] : m_sema.getTypeContext().getUserTypes()) {
                    if (userType && userType->isClass()) {
                        auto clsType = std::static_pointer_cast<sema::ClassType>(userType);
                        if (clsType->getName().rfind(cls->getName() + "<", 0) == 0) {
                            std::string monoName = codegen::VTableBuilder::mangleGenericName(clsType->getName());
                            const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(monoName) : nullptr;
                            if (layout) {
                                size_t lt = clsType->getName().find('<');
                                size_t gt = clsType->getName().rfind('>');
                                std::vector<std::string> typeArgs = sema::TypeContext::splitTypeArgs(clsType->getName().substr(lt + 1, gt - lt - 1));

                                const ast::FunctionDecl* explicitCtor = nullptr;
                                for (const auto& member : cls->getMembers()) {
                                    if (const auto* mFn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
                                        if (mFn->getName() == cls->getName()) {
                                            explicitCtor = mFn;
                                            break;
                                        }
                                    }
                                }

                                if (explicitCtor) {
                                    compileSpecializedConstructor(explicitCtor, cls->getTypeParams(), typeArgs, *layout);
                                } else {
                                    compileDefaultConstructor(*cls, *layout);
                                }

                                for (const auto& member : cls->getMembers()) {
                                    if (const auto* mFn = dynamic_cast<const ast::FunctionDecl*>(member.get())) {
                                        if (mFn->getName() != cls->getName()) {
                                            compileSpecializedMethod(*mFn, cls->getTypeParams(), typeArgs, *layout);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                decl->accept(*this);
            }
        }
    }

    // Next visit standalone functions
    for (const auto& decl : node.getDeclarations()) {
        if (dynamic_cast<const ast::FunctionDecl*>(decl.get())) {
            decl->accept(*this);
        }
    }

    // If there are top-level statements and no explicit main function, synthesize main
    if (!topLevelStmts.empty() && !hasMainFunc) {
        auto mainFunc = m_module->createFunction("main", IRType::getInt32(), {});
        m_currentFunction = mainFunc;
        auto entryBB = mainFunc->createBasicBlock("entry");
        setInsertPoint(entryBB);
        pushScope();

        for (const auto* stmt : topLevelStmts) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }

        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)));
        }
        popScope();
    }
}

void IRBuilder::visit(const ast::FunctionDecl& node) {
    if (node.isGeneric()) {
        return; // Generic function templates are monomorphized on demand when called
    }

    IRTypePtr returnType = (node.getName() == "main") ? IRType::getInt32() : mapType(node.getReturnType());

    std::vector<std::shared_ptr<ParameterValue>> params;
    for (size_t i = 0; i < node.getParams().size(); ++i) {
        const auto& p = node.getParams()[i];
        params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i));
    }

    auto func = m_module->createFunction(node.getName(), returnType, params);
    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();

    // Allocate stack slots for parameters and store incoming values
    for (size_t i = 0; i < params.size(); ++i) {
        const auto& p = node.getParams()[i];
        const auto& paramVal = params[i];
        auto allocaInst = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaInst);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaInst));
        declareVariable(p.getName(), allocaInst, paramVal->getType());
    }

    if (node.hasBody()) {
        for (const auto& stmt : node.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (node.isShortForm()) {
        node.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    // Ensure terminator exists
    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else if (node.getName() == "main") {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)));
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::visit(const ast::BlockStmt& node) {
    pushScope();
    for (const auto& stmt : node.getStatements()) {
        stmt->accept(*this);
        if (m_currentBlock->hasTerminator()) break;
    }
    popScope();
}

void IRBuilder::visit(const ast::ExprStmt& node) {
    node.getExpr().accept(*this);
}

void IRBuilder::visit(const ast::VarDeclStmt& node) {
    IRTypePtr valType = nullptr;

    const codegen::ClassLayout* structLayout = nullptr;
    if (m_vtableBuilder && !node.getType().empty()) {
        structLayout = m_vtableBuilder->getClassLayout(node.getType());
        if (structLayout && structLayout->hasVTable) {
            structLayout = nullptr;
        }
    }

    if (structLayout) {
        valType = structLayout->structType;
    } else if (!node.getType().empty() && node.getType() != "var" && node.getType() != "let" && node.getType() != "const") {
        valType = mapType(node.getType());
    }

    IRValuePtr initVal = nullptr;
    if (node.hasInitializer()) {
        node.getInitializer()->accept(*this);
        initVal = m_lastExprValue;
        if (!valType && m_lastExprType) {
            valType = m_lastExprType;
        }
    }

    if (!valType) {
        valType = IRType::getInt32();
    }

    std::string varReg = "%" + node.getName() + "_" + std::to_string(m_varCounter++);
    auto allocaInst = std::make_shared<AllocaInst>(valType, varReg);
    m_currentFunction->getEntryBlock()->addInstruction(allocaInst);
    declareVariable(node.getName(), allocaInst, valType);

    bool isRefCounted = false;
    if (valType && valType->isPointer()) {
        if (m_vtableBuilder && !node.getType().empty()) {
            const auto* l = m_vtableBuilder->getClassLayout(node.getType());
            if (l && l->hasVTable) {
                isRefCounted = true;
            }
        }
        if (!isRefCounted && node.hasInitializer()) {
            auto semaType = m_sema.getNodeType(node.getInitializer());
            if (semaType && semaType->isReferenceType()) {
                isRefCounted = true;
            } else if (dynamic_cast<const ast::NewExpr*>(node.getInitializer())) {
                isRefCounted = true;
            }
        }
    }

    if (isRefCounted && !m_refCountedScopes.empty()) {
        m_refCountedScopes.back().push_back({node.getName(), allocaInst});
        if (initVal && !dynamic_cast<const ast::NewExpr*>(node.getInitializer())) {
            auto retainCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_retain",
                std::vector<IRValuePtr>{initVal}
            );
            m_currentBlock->addInstruction(retainCall);
        }
    }

    if (initVal) {
        if (valType && valType->isStruct() && initVal->getType()->isPointer()) {
            auto loadInst = std::make_shared<LoadInst>(valType, initVal, m_currentFunction->nextTempName("tu_val"));
            m_currentBlock->addInstruction(loadInst);
            initVal = loadInst;
        }
        initVal = emitCastIfNeeded(initVal, valType);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(initVal, allocaInst));
    }
}

void IRBuilder::visit(const ast::IfStmt& node) {
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    if (condVal && !condVal->getType()->isInt1()) {
        condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                m_currentFunction->nextTempName("bool"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
    }

    auto thenBB = createBlock("then");
    auto elseBB = node.hasElseBranch() ? createBlock("else") : nullptr;
    auto mergeBB = createBlock("merge");

    std::string falseLabel = elseBB ? elseBB->getName() : mergeBB->getName();
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, thenBB->getName(), falseLabel));

    // Then branch
    setInsertPoint(thenBB);
    node.getThenBranch().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
    }

    // Else branch
    if (elseBB) {
        setInsertPoint(elseBB);
        node.getElseBranch()->accept(*this);
        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));
        }
    }

    setInsertPoint(mergeBB);
}

void IRBuilder::visit(const ast::WhileStmt& node) {
    auto condBB = createBlock("while_cond");
    auto bodyBB = createBlock("while_body");
    auto endBB = createBlock("while_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // Condition
    setInsertPoint(condBB);
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    if (condVal && !condVal->getType()->isInt1()) {
        condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                m_currentFunction->nextTempName("bool"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
    }
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBB->getName(), endBB->getName()));

    // Body
    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(condBB->getName());

    setInsertPoint(bodyBB);
    node.getBody().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();

    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::DoWhileStmt& node) {
    auto bodyBB = createBlock("dowhile_body");
    auto condBB = createBlock("dowhile_cond");
    auto endBB = createBlock("dowhile_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(bodyBB->getName()));

    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(condBB->getName());

    setInsertPoint(bodyBB);
    node.getBody().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();

    setInsertPoint(condBB);
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    if (condVal && !condVal->getType()->isInt1()) {
        condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                m_currentFunction->nextTempName("bool"));
        m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
    }
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBB->getName(), endBB->getName()));

    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::ForStmt& node) {
    pushScope();

    if (node.hasInit()) {
        node.getInit()->accept(*this);
    }

    auto condBB = createBlock("for_cond");
    auto bodyBB = createBlock("for_body");
    auto stepBB = createBlock("for_step");
    auto endBB = createBlock("for_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // Condition
    setInsertPoint(condBB);
    if (node.hasCondition()) {
        node.getCondition()->accept(*this);
        IRValuePtr condVal = m_lastExprValue;
        if (condVal && !condVal->getType()->isInt1()) {
            condVal = std::make_shared<CompareInst>(IROpcode::ICmpNe, condVal,
                                                    std::make_shared<ConstantInt>(condVal->getType(), 0),
                                                    m_currentFunction->nextTempName("bool"));
            m_currentBlock->addInstruction(std::dynamic_pointer_cast<IRInstruction>(condVal));
        }
        m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBB->getName(), endBB->getName()));
    } else {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(bodyBB->getName()));
    }

    // Body
    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(stepBB->getName());

    setInsertPoint(bodyBB);
    node.getBody().accept(*this);
    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(stepBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();

    // Step
    setInsertPoint(stepBB);
    if (node.hasIncrement()) {
        node.getIncrement()->accept(*this);
    }
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    popScope();
    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::ForInStmt& node) {
    if (const auto* bin = dynamic_cast<const ast::BinaryExpr*>(&node.getIterable())) {
        if (bin->getOp() == lexer::TokenType::DotDot) {
            bin->getLeft().accept(*this);
            IRValuePtr startVal = m_lastExprValue;
            bin->getRight().accept(*this);
            IRValuePtr endVal = m_lastExprValue;
            if (!startVal || !endVal) return;

            startVal = emitCastIfNeeded(startVal, IRType::getInt32());
            endVal = emitCastIfNeeded(endVal, IRType::getInt32());

            std::string varReg = "%" + node.getVarName() + "_" + std::to_string(m_varCounter++);
            auto varAlloca = std::make_shared<AllocaInst>(IRType::getInt32(), varReg);
            m_currentFunction->getEntryBlock()->addInstruction(varAlloca);
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(startVal, varAlloca));

            auto condBB = createBlock("range_cond");
            auto bodyBB = createBlock("range_body");
            auto stepBB = createBlock("range_step");
            auto endBB = createBlock("range_end");

            m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

            // Condition: i < end
            setInsertPoint(condBB);
            auto curVal = std::make_shared<LoadInst>(IRType::getInt32(), varAlloca, m_currentFunction->nextTempName("range_cur"));
            condBB->addInstruction(curVal);
            auto cmpInst = std::make_shared<CompareInst>(IROpcode::ICmpSlt, curVal, endVal, m_currentFunction->nextTempName("range_cmp"));
            condBB->addInstruction(cmpInst);
            condBB->addInstruction(std::make_shared<CondBranchInst>(cmpInst, bodyBB->getName(), endBB->getName()));

            // Body
            setInsertPoint(bodyBB);
            pushScope();
            declareVariable(node.getVarName(), varAlloca, IRType::getInt32());

            m_breakStack.push_back(endBB->getName());
            m_continueStack.push_back(stepBB->getName());

            node.getBody().accept(*this);

            if (!m_currentBlock->hasTerminator()) {
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(stepBB->getName()));
            }

            m_breakStack.pop_back();
            m_continueStack.pop_back();
            popScope();

            // Step: i++
            setInsertPoint(stepBB);
            auto stepVal = std::make_shared<LoadInst>(IRType::getInt32(), varAlloca, m_currentFunction->nextTempName("range_step_load"));
            stepBB->addInstruction(stepVal);
            auto nextVal = std::make_shared<BinaryInst>(
                IROpcode::Add, stepVal, std::make_shared<ConstantInt>(IRType::getInt32(), 1),
                m_currentFunction->nextTempName("range_next"));
            stepBB->addInstruction(nextVal);
            stepBB->addInstruction(std::make_shared<StoreInst>(nextVal, varAlloca));
            stepBB->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

            setInsertPoint(endBB);
            return;
        }
    }

    node.getIterable().accept(*this);
    IRValuePtr collVal = m_lastExprValue;
    if (!collVal) return;

    auto semaIterType = m_sema.getNodeType(&node.getIterable());
    bool isList = semaIterType && semaIterType->isClass() && semaIterType->toString().rfind("List", 0) == 0;

    // 1. Query collection length/size
    std::string lenReg = m_currentFunction->nextTempName("for_len");
    auto lenCall = std::make_shared<CallInst>(
        IRType::getInt64(),
        (isList ? "sp_rt_list_size" : "sp_rt_array_length"),
        std::vector<IRValuePtr>{collVal},
        lenReg
    );
    m_currentBlock->addInstruction(lenCall);

    // 2. Allocate induction variable idx = 0 on stack
    std::string idxAllocaReg = "%for_idx_" + std::to_string(m_varCounter++);
    auto idxAlloca = std::make_shared<AllocaInst>(IRType::getInt64(), idxAllocaReg);
    m_currentFunction->getEntryBlock()->addInstruction(idxAlloca);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(
        std::make_shared<ConstantInt>(IRType::getInt64(), 0), idxAlloca));

    // 3. Determine loop element variable IR type and allocate stack slot
    IRTypePtr itemIRType = IRType::getInt32();
    if (!node.getType().empty() && node.getType() != "var" && node.getType() != "let" && node.getType() != "auto") {
        itemIRType = mapType(node.getType());
    } else if (semaIterType && semaIterType->isArray()) {
        const auto& arr = static_cast<const sema::ArrayType&>(*semaIterType);
        itemIRType = mapType(arr.getElementType().get());
    } else if (isList) {
        std::string s = semaIterType->toString();
        size_t lt = s.find('<');
        size_t gt = s.rfind('>');
        if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
            itemIRType = mapType(s.substr(lt + 1, gt - lt - 1));
        }
    }
    if (!itemIRType) itemIRType = IRType::getInt32();

    std::string varReg = "%" + node.getVarName() + "_" + std::to_string(m_varCounter++);
    auto varAlloca = std::make_shared<AllocaInst>(itemIRType, varReg);
    m_currentFunction->getEntryBlock()->addInstruction(varAlloca);

    // 4. Create blocks
    auto condBB = createBlock("for_in_cond");
    auto bodyBB = createBlock("for_in_body");
    auto stepBB = createBlock("for_in_step");
    auto endBB = createBlock("for_in_end");

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // 5. Condition block: check idx < len
    setInsertPoint(condBB);
    std::string curIdxReg = m_currentFunction->nextTempName("cur_idx");
    auto curIdx = std::make_shared<LoadInst>(IRType::getInt64(), idxAlloca, curIdxReg);
    condBB->addInstruction(curIdx);
    std::string cmpReg = m_currentFunction->nextTempName("for_cmp");
    auto cmpInst = std::make_shared<CompareInst>(IROpcode::ICmpSlt, curIdx, lenCall, cmpReg);
    condBB->addInstruction(cmpInst);
    condBB->addInstruction(std::make_shared<CondBranchInst>(cmpInst, bodyBB->getName(), endBB->getName()));

    // 6. Body block
    setInsertPoint(bodyBB);
    pushScope();
    declareVariable(node.getVarName(), varAlloca, itemIRType);

    // Fetch element at curIdx and store into varAlloca
    if (isList) {
        if (itemIRType->isFloatingPoint()) {
            auto getCall = std::make_shared<CallInst>(
                IRType::getDouble(), "sp_rt_list_get_f64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        } else {
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(), "sp_rt_list_get_i64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        }
    } else {
        if (itemIRType->isFloatingPoint()) {
            auto getCall = std::make_shared<CallInst>(
                IRType::getDouble(), "sp_rt_array_get_f64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        } else {
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(), "sp_rt_array_get_i64",
                std::vector<IRValuePtr>{collVal, curIdx},
                m_currentFunction->nextTempName("elem"));
            bodyBB->addInstruction(getCall);
            auto castVal = emitCastIfNeeded(getCall, itemIRType);
            bodyBB->addInstruction(std::make_shared<StoreInst>(castVal, varAlloca));
        }
    }

    m_breakStack.push_back(endBB->getName());
    m_continueStack.push_back(stepBB->getName());

    node.getBody().accept(*this);

    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(stepBB->getName()));
    }

    m_breakStack.pop_back();
    m_continueStack.pop_back();
    popScope();

    // 7. Step block: idx++
    setInsertPoint(stepBB);
    auto stepIdx = std::make_shared<LoadInst>(IRType::getInt64(), idxAlloca, m_currentFunction->nextTempName("step_idx"));
    stepBB->addInstruction(stepIdx);
    auto nextIdx = std::make_shared<BinaryInst>(
        IROpcode::Add, stepIdx, std::make_shared<ConstantInt>(IRType::getInt64(), 1),
        m_currentFunction->nextTempName("next_idx"));
    stepBB->addInstruction(nextIdx);
    stepBB->addInstruction(std::make_shared<StoreInst>(nextIdx, idxAlloca));
    stepBB->addInstruction(std::make_shared<BranchInst>(condBB->getName()));

    // 8. End block
    setInsertPoint(endBB);
}
void IRBuilder::visit(const ast::SwitchStmt& node) {
    if (!m_currentFunction || !m_currentBlock) return;

    node.getSubject().accept(*this);
    IRValuePtr subjVal = m_lastExprValue;
    IRTypePtr subjType = m_lastExprType;
    if (!subjVal) return;

    auto endBB = createBlock("switch_end");
    m_breakStack.push_back(endBB->getName());

    const auto& cases = node.getCases();
    size_t numCases = cases.size();

    auto defaultBB = node.getDefaultStatements().empty() ? nullptr : createBlock("switch_default");
    std::string defaultTarget = defaultBB ? defaultBB->getName() : endBB->getName();

    if (numCases == 0) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(defaultTarget));
    } else {
        std::vector<IRBasicBlock*> testBBs;
        std::vector<IRBasicBlock*> bodyBBs;
        for (size_t i = 0; i < numCases; ++i) {
            testBBs.push_back(createBlock("switch_test"));
            bodyBBs.push_back(createBlock("switch_body"));
        }

        m_currentBlock->addInstruction(std::make_shared<BranchInst>(testBBs[0]->getName()));

        auto semaSubj = m_sema.getNodeType(&node.getSubject());
        bool isSubjStr = (semaSubj && semaSubj->isString()) || (subjType && subjType->isPointer());

        for (size_t i = 0; i < numCases; ++i) {
            setInsertPoint(testBBs[i]);
            cases[i].getValue().accept(*this);
            IRValuePtr caseVal = m_lastExprValue;
            IRTypePtr caseType = m_lastExprType;

            IRValuePtr condVal = nullptr;
            if (isSubjStr || (caseType && caseType->isPointer())) {
                auto call = std::make_shared<CallInst>(
                    IRType::getInt1(), "sp_rt_str_equals",
                    std::vector<IRValuePtr>{subjVal, caseVal},
                    m_currentFunction->nextTempName("str_eq"));
                testBBs[i]->addInstruction(call);
                condVal = call;
            } else {
                bool isFloat = (subjType && subjType->isFloatingPoint()) || (caseType && caseType->isFloatingPoint());
                IRTypePtr cmpType = isFloat ? IRType::getDouble()
                                            : ((subjType && subjType->isInt64()) || (caseType && caseType->isInt64())
                                                   ? IRType::getInt64()
                                                   : IRType::getInt32());
                auto l = emitCastIfNeeded(subjVal, cmpType);
                auto r = emitCastIfNeeded(caseVal, cmpType);
                auto cmp = std::make_shared<CompareInst>(
                    isFloat ? IROpcode::FCmpEq : IROpcode::ICmpEq,
                    l, r, m_currentFunction->nextTempName("cmp"));
                testBBs[i]->addInstruction(cmp);
                condVal = cmp;
            }

            std::string nextTest = (i + 1 < numCases) ? testBBs[i + 1]->getName() : defaultTarget;
            testBBs[i]->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBBs[i]->getName(), nextTest));

            // Case body
            setInsertPoint(bodyBBs[i]);
            pushScope();
            for (const auto& s : cases[i].getStatements()) {
                if (s) s->accept(*this);
            }
            popScope();

            if (!m_currentBlock->hasTerminator()) {
                std::string fallthrough = (i + 1 < numCases) ? bodyBBs[i + 1]->getName() : defaultTarget;
                m_currentBlock->addInstruction(std::make_shared<BranchInst>(fallthrough));
            }
        }
    }

    if (defaultBB) {
        setInsertPoint(defaultBB);
        pushScope();
        for (const auto& s : node.getDefaultStatements()) {
            if (s) s->accept(*this);
        }
        popScope();
        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(endBB->getName()));
        }
    }

    m_breakStack.pop_back();
    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::MatchStmt& node) {
    if (!m_currentFunction || !m_currentBlock) return;

    node.getSubject().accept(*this);
    IRValuePtr subjVal = m_lastExprValue;
    IRTypePtr subjType = m_lastExprType;
    if (!subjVal) return;

    auto endBB = createBlock("match_end");

    const auto& arms = node.getArms();
    size_t numArms = arms.size();

    if (numArms == 0) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(endBB->getName()));
        setInsertPoint(endBB);
        return;
    }

    std::vector<IRBasicBlock*> testBBs;
    std::vector<IRBasicBlock*> bodyBBs;
    for (size_t i = 0; i < numArms; ++i) {
        testBBs.push_back(createBlock("match_test"));
        bodyBBs.push_back(createBlock("match_body"));
    }

    m_currentBlock->addInstruction(std::make_shared<BranchInst>(testBBs[0]->getName()));

    auto semaSubj = m_sema.getNodeType(&node.getSubject());
    bool isSubjStr = (semaSubj && semaSubj->isString()) || (subjType && subjType->isPointer());

    for (size_t i = 0; i < numArms; ++i) {
        setInsertPoint(testBBs[i]);
        const auto& arm = arms[i];

        if (arm.isWildcard()) {
            testBBs[i]->addInstruction(std::make_shared<BranchInst>(bodyBBs[i]->getName()));
        } else {
            arm.getPattern()->accept(*this);
            IRValuePtr patVal = m_lastExprValue;
            IRTypePtr patType = m_lastExprType;

            IRValuePtr condVal = nullptr;
            if (isSubjStr || (patType && patType->isPointer())) {
                auto call = std::make_shared<CallInst>(
                    IRType::getInt1(), "sp_rt_str_equals",
                    std::vector<IRValuePtr>{subjVal, patVal},
                    m_currentFunction->nextTempName("str_eq"));
                testBBs[i]->addInstruction(call);
                condVal = call;
            } else {
                bool isFloat = (subjType && subjType->isFloatingPoint()) || (patType && patType->isFloatingPoint());
                IRTypePtr cmpType = isFloat ? IRType::getDouble()
                                            : ((subjType && subjType->isInt64()) || (patType && patType->isInt64())
                                                   ? IRType::getInt64()
                                                   : IRType::getInt32());
                auto l = emitCastIfNeeded(subjVal, cmpType);
                auto r = emitCastIfNeeded(patVal, cmpType);
                auto cmp = std::make_shared<CompareInst>(
                    isFloat ? IROpcode::FCmpEq : IROpcode::ICmpEq,
                    l, r, m_currentFunction->nextTempName("cmp"));
                testBBs[i]->addInstruction(cmp);
                condVal = cmp;
            }

            std::string nextTest = (i + 1 < numArms) ? testBBs[i + 1]->getName() : endBB->getName();
            testBBs[i]->addInstruction(std::make_shared<CondBranchInst>(condVal, bodyBBs[i]->getName(), nextTest));
        }

        // Arm body
        setInsertPoint(bodyBBs[i]);
        pushScope();
        arm.getBody().accept(*this);
        popScope();

        if (!m_currentBlock->hasTerminator()) {
            m_currentBlock->addInstruction(std::make_shared<BranchInst>(endBB->getName()));
        }
    }

    setInsertPoint(endBB);
}

void IRBuilder::visit(const ast::ReturnStmt& node) {
    if (node.hasValue()) {
        node.getValue()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        if (m_currentFunction->getReturnType()->isStruct() && retVal && retVal->getType()->isPointer()) {
            auto loadInst = std::make_shared<LoadInst>(m_currentFunction->getReturnType(), retVal, m_currentFunction->nextTempName("tu_ret"));
            m_currentBlock->addInstruction(loadInst);
            retVal = loadInst;
        }
        retVal = emitCastIfNeeded(retVal, m_currentFunction->getReturnType());

        bool isRefRet = false;
        if (retVal && retVal->getType()->isPointer()) {
            auto semaType = m_sema.getNodeType(node.getValue());
            if (semaType && semaType->isReferenceType()) {
                isRefRet = true;
            } else if (dynamic_cast<const ast::NewExpr*>(node.getValue())) {
                isRefRet = true;
            }
        }

        if (isRefRet) {
            auto retainCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_retain",
                std::vector<IRValuePtr>{retVal}
            );
            m_currentBlock->addInstruction(retainCall);
        }

        emitAllFunctionDrops();
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    } else {
        emitAllFunctionDrops();
        if (m_currentFunction->getName() == "main") {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)));
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        }
    }
}

void IRBuilder::visit(const ast::BreakStmt& node) {
    (void)node;
    if (!m_breakStack.empty()) {
        if (!m_refCountedScopes.empty()) {
            emitScopeDrops(m_refCountedScopes.size() - 1);
        }
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_breakStack.back()));
    }
}

void IRBuilder::visit(const ast::ContinueStmt& node) {
    (void)node;
    if (!m_continueStack.empty()) {
        if (!m_refCountedScopes.empty()) {
            emitScopeDrops(m_refCountedScopes.size() - 1);
        }
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_continueStack.back()));
    }
}

void IRBuilder::visit(const ast::TryCatchStmt& node) {
    node.getTryBlock().accept(*this);
    if (node.hasFinally() && node.getFinallyBlock()) {
        node.getFinallyBlock()->accept(*this);
    }
}
void IRBuilder::visit(const ast::ThrowStmt& node) { (void)node; }

void IRBuilder::visit(const ast::ClassDecl& node) {
    if (!m_vtableBuilder) return;
    codegen::ClassGenerator classGen(*this, *m_module, *m_vtableBuilder);
    classGen.generateClass(node);
}

void IRBuilder::compileDefaultConstructor(const ast::ClassDecl& classDecl, const codegen::ClassLayout& layout) {
    (void)classDecl;
    std::string mangledName = layout.name + "_" + layout.name;
    auto thisParam = std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0);
    auto func = m_module->createFunction(mangledName, IRType::getVoid(), {thisParam});

    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
    entryBB->addInstruction(allocaThis);
    entryBB->addInstruction(std::make_shared<StoreInst>(thisParam, allocaThis));
    declareVariable("this", allocaThis, IRType::getPointer());

    if (layout.hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout.vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout.structType,
            thisParam,
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            func->nextTempName("vptr")
        );
        entryBB->addInstruction(gep);
        entryBB->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    entryBB->addInstruction(std::make_shared<ReturnInst>());

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileConstructor(const ast::ClassDecl& classDecl, const ast::FunctionDecl* ctorDecl, const codegen::ClassLayout& layout) {
    (void)classDecl;
    std::string mangledName = layout.name + "_" + layout.name;
    std::vector<std::shared_ptr<ParameterValue>> params;
    params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i + 1));
        }
    }

    auto func = m_module->createFunction(mangledName, IRType::getVoid(), params);
    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
    entryBB->addInstruction(allocaThis);
    entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
    declareVariable("this", allocaThis, IRType::getPointer());

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            const auto& paramVal = params[i + 1];
            auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
            entryBB->addInstruction(allocaParam);
            entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
            declareVariable(p.getName(), allocaParam, paramVal->getType());
        }
    }

    if (layout.hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout.vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout.structType,
            params[0],
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            func->nextTempName("vptr")
        );
        entryBB->addInstruction(gep);
        entryBB->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    if (ctorDecl && ctorDecl->hasBody()) {
        for (const auto& stmt : ctorDecl->getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    }

    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
    }

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileMethod(const ast::ClassDecl& classDecl, const ast::FunctionDecl& methodDecl, const codegen::ClassLayout& layout) {
    (void)classDecl;
    std::string mangledName = layout.name + "_" + methodDecl.getName();
    IRTypePtr returnType = mapType(methodDecl.getReturnType());

    std::vector<std::shared_ptr<ParameterValue>> params;
    size_t paramOffset = 0;
    if (!methodDecl.isStatic()) {
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));
        paramOffset = 1;
    }

    for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
        const auto& p = methodDecl.getParams()[i];
        params.push_back(std::make_shared<ParameterValue>(mapType(p.getType()), "%" + p.getName(), i + paramOffset));
    }

    auto func = m_module->createFunction(mangledName, returnType, params);
    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    if (!methodDecl.isStatic()) {
        auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
        entryBB->addInstruction(allocaThis);
        entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
        declareVariable("this", allocaThis, IRType::getPointer());
    }

    for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
        const auto& p = methodDecl.getParams()[i];
        const auto& paramVal = params[i + paramOffset];
        auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaParam);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
        declareVariable(p.getName(), allocaParam, paramVal->getType());
    }

    if (methodDecl.hasBody()) {
        for (const auto& stmt : methodDecl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (methodDecl.isShortForm()) {
        methodDecl.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileSpecializedConstructor(
    const ast::FunctionDecl* ctorDecl,
    const std::vector<std::string>& typeParams,
    const std::vector<std::string>& typeArgs,
    const codegen::ClassLayout& layout) {

    std::string mangledName = layout.name + "_" + layout.name;
    std::vector<std::shared_ptr<ParameterValue>> params;
    params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
            params.push_back(std::make_shared<ParameterValue>(mapType(subType), "%" + p.getName(), i + 1));
        }
    }

    auto func = m_module->createFunction(mangledName, IRType::getVoid(), params);
    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
    entryBB->addInstruction(allocaThis);
    entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
    declareVariable("this", allocaThis, IRType::getPointer());

    if (ctorDecl) {
        for (size_t i = 0; i < ctorDecl->getParams().size(); ++i) {
            const auto& p = ctorDecl->getParams()[i];
            const auto& paramVal = params[i + 1];
            auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
            entryBB->addInstruction(allocaParam);
            entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
            declareVariable(p.getName(), allocaParam, paramVal->getType());
        }
    }

    if (layout.hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout.vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout.structType,
            params[0],
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            func->nextTempName("vptr")
        );
        entryBB->addInstruction(gep);
        entryBB->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    if (ctorDecl && ctorDecl->hasBody()) {
        for (const auto& stmt : ctorDecl->getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    }

    if (!m_currentBlock->hasTerminator()) {
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
    }

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileSpecializedMethod(
    const ast::FunctionDecl& methodDecl,
    const std::vector<std::string>& typeParams,
    const std::vector<std::string>& typeArgs,
    const codegen::ClassLayout& layout) {

    std::string mangledName = layout.name + "_" + methodDecl.getName();
    std::string subRet = sema::TypeContext::substituteType(methodDecl.getReturnType(), typeParams, typeArgs);
    IRTypePtr returnType = mapType(subRet);

    std::vector<std::shared_ptr<ParameterValue>> params;
    size_t paramOffset = 0;
    if (!methodDecl.isStatic()) {
        params.push_back(std::make_shared<ParameterValue>(IRType::getPointer(), "%this", 0));
        paramOffset = 1;
    }

    for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
        const auto& p = methodDecl.getParams()[i];
        std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
        params.push_back(std::make_shared<ParameterValue>(mapType(subType), "%" + p.getName(), i + paramOffset));
    }

    auto func = m_module->createFunction(mangledName, returnType, params);
    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();
    m_currentClass = &layout;

    if (!methodDecl.isStatic()) {
        auto allocaThis = std::make_shared<AllocaInst>(IRType::getPointer(), "%p_this");
        entryBB->addInstruction(allocaThis);
        entryBB->addInstruction(std::make_shared<StoreInst>(params[0], allocaThis));
        declareVariable("this", allocaThis, IRType::getPointer());
    }

    for (size_t i = 0; i < methodDecl.getParams().size(); ++i) {
        const auto& p = methodDecl.getParams()[i];
        const auto& paramVal = params[i + paramOffset];
        auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaParam);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
        declareVariable(p.getName(), allocaParam, paramVal->getType());
    }

    if (methodDecl.hasBody()) {
        for (const auto& stmt : methodDecl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (methodDecl.isShortForm()) {
        methodDecl.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_currentClass = nullptr;
    m_currentFunction = nullptr;
    m_currentBlock = nullptr;
}

void IRBuilder::compileSpecializedFunction(
    const ast::FunctionDecl& fnDecl,
    const std::vector<std::string>& typeArgs) {

    std::string mangledName = fnDecl.getName();
    for (const auto& arg : typeArgs) {
        mangledName += "_" + arg;
    }

    if (m_module->getFunction(mangledName)) {
        return; // Already compiled
    }

    const auto& typeParams = fnDecl.getTypeParams();
    std::string subRet = sema::TypeContext::substituteType(fnDecl.getReturnType(), typeParams, typeArgs);
    IRTypePtr returnType = mapType(subRet);

    std::vector<std::shared_ptr<ParameterValue>> params;
    for (size_t i = 0; i < fnDecl.getParams().size(); ++i) {
        const auto& p = fnDecl.getParams()[i];
        std::string subType = sema::TypeContext::substituteType(p.getType(), typeParams, typeArgs);
        params.push_back(std::make_shared<ParameterValue>(mapType(subType), "%" + p.getName(), i));
    }

    auto func = m_module->createFunction(mangledName, returnType, params);
    auto* prevFunc = m_currentFunction;
    auto* prevBlock = m_currentBlock;
    auto prevScopes = std::move(m_scopes);
    auto prevRefScopes = std::move(m_refCountedScopes);
    m_scopes.clear();
    m_refCountedScopes.clear();

    m_currentFunction = func;
    auto entryBB = func->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();

    for (size_t i = 0; i < fnDecl.getParams().size(); ++i) {
        const auto& p = fnDecl.getParams()[i];
        const auto& paramVal = params[i];
        auto allocaParam = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.getName());
        entryBB->addInstruction(allocaParam);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaParam));
        declareVariable(p.getName(), allocaParam, paramVal->getType());
    }

    if (fnDecl.hasBody()) {
        for (const auto& stmt : fnDecl.getBody()->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
    } else if (fnDecl.isShortForm()) {
        fnDecl.getShortBody()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, returnType);
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    }

    if (!m_currentBlock->hasTerminator()) {
        if (returnType->isVoid()) {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(returnType)));
        }
    }

    popScope();
    m_scopes = std::move(prevScopes);
    m_refCountedScopes = std::move(prevRefScopes);
    m_currentFunction = prevFunc;
    m_currentBlock = prevBlock;
}
void IRBuilder::visit(const ast::InterfaceDecl& node) { (void)node; }
void IRBuilder::visit(const ast::StructDecl& node) { (void)node; }
void IRBuilder::visit(const ast::EnumDecl& node) { (void)node; }
void IRBuilder::visit(const ast::ModuleDecl& node) { (void)node; }
void IRBuilder::visit(const ast::ImportDecl& node) { (void)node; }

// --- Expressions ---

void IRBuilder::visit(const ast::LiteralExpr& node) {
    switch (node.getLiteralKind()) {
        case ast::LiteralKind::Integer: {
            int64_t val = 0;
            std::string cleanVal = node.getValue();
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), '_'), cleanVal.end());
            try {
                if (cleanVal.rfind("0b", 0) == 0 || cleanVal.rfind("0B", 0) == 0) {
                    val = static_cast<int64_t>(std::stoull(cleanVal.substr(2), nullptr, 2));
                } else if (cleanVal.rfind("0o", 0) == 0 || cleanVal.rfind("0O", 0) == 0) {
                    val = static_cast<int64_t>(std::stoull(cleanVal.substr(2), nullptr, 8));
                } else {
                    val = std::stoll(cleanVal, nullptr, 0);
                }
            } catch (...) {
                try {
                    val = static_cast<int64_t>(std::stoull(cleanVal, nullptr, 0));
                } catch (...) {
                    val = 0;
                }
            }
            if (val > 2147483647LL || val < -2147483648LL) {
                m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt64(), val);
                m_lastExprType = IRType::getInt64();
            } else {
                m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
                m_lastExprType = IRType::getInt32();
            }
            break;
        }
        case ast::LiteralKind::Float: {
            double val = 0.0;
            std::string cleanVal = node.getValue();
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), '_'), cleanVal.end());
            bool isFloat32 = (!cleanVal.empty() && (cleanVal.back() == 'f' || cleanVal.back() == 'F'));
            if (isFloat32) {
                cleanVal.pop_back();
            }
            try {
                val = std::stod(cleanVal);
            } catch (...) {
                val = 0.0;
            }
            if (isFloat32) {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getFloat(), val);
                m_lastExprType = IRType::getFloat();
            } else {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), val);
                m_lastExprType = IRType::getDouble();
            }
            break;
        }
        case ast::LiteralKind::Boolean: {
            bool val = (node.getValue() == "true");
            m_lastExprValue = std::make_shared<ConstantBool>(val);
            m_lastExprType = IRType::getInt1();
            break;
        }
        case ast::LiteralKind::String: {
            auto strVal = m_module->getOrCreateStringLiteral(node.getValue());
            m_lastExprValue = strVal;
            m_lastExprType = IRType::getPointer(IRType::getInt8());
            break;
        }
        case ast::LiteralKind::Char: {
            char c = node.getValue().empty() ? '\0' : node.getValue()[0];
            m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt8(), static_cast<int64_t>(c));
            m_lastExprType = IRType::getInt8();
            break;
        }
        case ast::LiteralKind::Null: {
            m_lastExprValue = std::make_shared<ConstantNull>();
            m_lastExprType = IRType::getPointer();
            break;
        }
    }
}

void IRBuilder::visit(const ast::IdentifierExpr& node) {
    auto [allocaPtr, valType] = lookupVariable(node.getName());
    if (allocaPtr && valType) {
        if (valType->isStruct()) {
            m_lastExprValue = allocaPtr;
            m_lastExprType = IRType::getPointer();
            return;
        }
        std::string reg = m_currentFunction->nextTempName("v");
        auto loadInst = std::make_shared<LoadInst>(valType, allocaPtr, reg);
        m_currentBlock->addInstruction(loadInst);
        m_lastExprValue = loadInst;
        m_lastExprType = valType;
        return;
    }

    if (node.getName() == "None") {
        m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
        m_lastExprType = IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
        return;
    }

    if (node.getName() == "PI") {
        m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 3.141592653589793);
        m_lastExprType = IRType::getDouble();
        return;
    }
    if (node.getName() == "E") {
        m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 2.718281828459045);
        m_lastExprType = IRType::getDouble();
        return;
    }
    if (node.getName() == "TAU") {
        m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 6.283185307179586);
        m_lastExprType = IRType::getDouble();
        return;
    }

    if (m_currentClass && m_currentClass->fieldIndexMap.count(node.getName())) {
        auto [thisAlloca, thisType] = lookupVariable("this");
        if (thisAlloca) {
            auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
            m_currentBlock->addInstruction(thisVal);

            uint32_t fieldIndex = m_currentClass->fieldIndexMap.at(node.getName());
            size_t fVecIdx = m_currentClass->hasVTable ? (fieldIndex - 1) : fieldIndex;
            if (fVecIdx < m_currentClass->fields.size()) {
                valType = m_currentClass->fields[fVecIdx].irType;

                auto gep = std::make_shared<GetElementPtrInst>(
                    m_currentClass->structType,
                    thisVal,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), fieldIndex)
                    },
                    m_currentFunction->nextTempName("fld_ptr")
                );
                m_currentBlock->addInstruction(gep);

                auto loadInst = std::make_shared<LoadInst>(valType, gep, m_currentFunction->nextTempName("val"));
                m_currentBlock->addInstruction(loadInst);
                m_lastExprValue = loadInst;
                m_lastExprType = valType;
                return;
            }
        }
    }

    // Fallback for unresolved or global variables
    m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), 0);
    m_lastExprType = IRType::getInt32();
}

void IRBuilder::visit(const ast::AssignExpr& node) {
    if (const auto* idx = dynamic_cast<const ast::IndexExpr*>(&node.getTarget())) {
        idx->getObject().accept(*this);
        IRValuePtr objVal = m_lastExprValue;

        idx->getIndex().accept(*this);
        IRValuePtr idxVal = m_lastExprValue;

        node.getValue().accept(*this);
        IRValuePtr rhsVal = m_lastExprValue;

        if (!objVal || !idxVal || !rhsVal) {
            return;
        }

        auto semaObjType = m_sema.getNodeType(&idx->getObject());
        bool isMapType = semaObjType && (semaObjType->isMap() || (semaObjType->isClass() && semaObjType->toString().rfind("Map", 0) == 0));
        bool isListType = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("List", 0) == 0;
        bool isPtrType = semaObjType && semaObjType->isPointer();

        if (isPtrType) {
            auto semaPointee = static_cast<const sema::PointerType&>(*semaObjType).getPointeeType();
            IRTypePtr elemType = semaPointee ? mapType(semaPointee.get()) : IRType::getInt64();
            std::string gepName = m_currentFunction->nextTempName("ptr_gep");
            auto gep = std::make_shared<GetElementPtrInst>(
                elemType, objVal,
                std::vector<IRValuePtr>{idxVal},
                gepName
            );
            m_currentBlock->addInstruction(gep);
            rhsVal = emitCastIfNeeded(rhsVal, elemType);
            m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhsVal, gep));
            m_lastExprValue = rhsVal;
            m_lastExprType = elemType;
            return;
        }

        if (isMapType) {
            if (idxVal->getType()->isPointer()) {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_map_set_str",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            } else {
                idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_map_set_i64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            }
        } else if (isListType) {
            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
            if (rhsVal->getType()->isFloatingPoint()) {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getDouble());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_list_set_f64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            } else {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_list_set_i64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            }
        } else {
            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
            if (rhsVal->getType()->isFloatingPoint()) {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getDouble());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_array_set_f64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            } else {
                rhsVal = emitCastIfNeeded(rhsVal, IRType::getInt64());
                auto setCall = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_array_set_i64",
                    std::vector<IRValuePtr>{objVal, idxVal, rhsVal}
                );
                m_currentBlock->addInstruction(setCall);
            }
        }

        m_lastExprValue = rhsVal;
        return;
    }

    IRValuePtr targetPtr = nullptr;
    IRTypePtr valType = nullptr;

    if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&node.getTarget())) {
        targetPtr = emitMemberPointer(*mem, valType);
    } else if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getTarget())) {
        auto [allocaPtr, type] = lookupVariable(ident->getName());
        if (allocaPtr && type) {
            targetPtr = allocaPtr;
            valType = type;
        } else if (m_currentClass && m_currentClass->fieldIndexMap.count(ident->getName())) {
            auto [thisAlloca, thisType] = lookupVariable("this");
            if (thisAlloca) {
                auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
                m_currentBlock->addInstruction(thisVal);

                uint32_t fieldIndex = m_currentClass->fieldIndexMap.at(ident->getName());
                size_t fVecIdx = m_currentClass->hasVTable ? (fieldIndex - 1) : fieldIndex;
                if (fVecIdx < m_currentClass->fields.size()) {
                    valType = m_currentClass->fields[fVecIdx].irType;
                    auto gep = std::make_shared<GetElementPtrInst>(
                        m_currentClass->structType,
                        thisVal,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), fieldIndex)
                        },
                        m_currentFunction->nextTempName("fld_ptr")
                    );
                    m_currentBlock->addInstruction(gep);
                    targetPtr = gep;
                }
            }
        }
    } else if (const auto* un = dynamic_cast<const ast::UnaryExpr*>(&node.getTarget())) {
        if (un->getOp() == lexer::TokenType::Star) {
            un->getOperand().accept(*this);
            targetPtr = m_lastExprValue;
            auto semaType = m_sema.getNodeType(&node.getTarget());
            valType = semaType ? mapType(semaType.get()) : IRType::getInt64();
        }
    }

    if (!targetPtr || !valType) {
        return;
    }

    node.getValue().accept(*this);
    IRValuePtr rhsVal = m_lastExprValue;

    // Handle compound assignment (+=, -=, etc.)
    if (node.getOp() != lexer::TokenType::Equal) {
        std::string currentValReg = m_currentFunction->nextTempName("cur");
        auto loadInst = std::make_shared<LoadInst>(valType, targetPtr, currentValReg);
        m_currentBlock->addInstruction(loadInst);

        IROpcode op = IROpcode::Add;
        switch (node.getOp()) {
            case lexer::TokenType::PlusEqual:
                op = valType->isFloatingPoint() ? IROpcode::FAdd : IROpcode::Add;
                break;
            case lexer::TokenType::MinusEqual:
                op = valType->isFloatingPoint() ? IROpcode::FSub : IROpcode::Sub;
                break;
            case lexer::TokenType::StarEqual:
                op = valType->isFloatingPoint() ? IROpcode::FMul : IROpcode::Mul;
                break;
            case lexer::TokenType::SlashEqual:
                op = valType->isFloatingPoint() ? IROpcode::FDiv : IROpcode::SDiv;
                break;
            case lexer::TokenType::PercentEqual:
                op = valType->isFloatingPoint() ? IROpcode::FRem : IROpcode::SRem;
                break;
            default:
                op = IROpcode::Add;
                break;
        }

        rhsVal = emitCastIfNeeded(rhsVal, valType);
        auto binInst = std::make_shared<BinaryInst>(op, loadInst, rhsVal, m_currentFunction->nextTempName("comp"));
        m_currentBlock->addInstruction(binInst);
        rhsVal = binInst;
    }

    if (valType && valType->isStruct() && rhsVal && rhsVal->getType()->isPointer()) {
        auto loadInst = std::make_shared<LoadInst>(valType, rhsVal, m_currentFunction->nextTempName("tu_assign"));
        m_currentBlock->addInstruction(loadInst);
        rhsVal = loadInst;
    }
    rhsVal = emitCastIfNeeded(rhsVal, valType);

    bool isRefAssignment = false;
    if (valType && valType->isPointer()) {
        auto semaType = m_sema.getNodeType(&node.getValue());
        if (semaType && semaType->isReferenceType()) {
            isRefAssignment = true;
        } else if (dynamic_cast<const ast::NewExpr*>(&node.getValue())) {
            isRefAssignment = true;
        }
    }

    if (isRefAssignment) {
        if (!dynamic_cast<const ast::NewExpr*>(&node.getValue())) {
            auto retainCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_retain",
                std::vector<IRValuePtr>{rhsVal}
            );
            m_currentBlock->addInstruction(retainCall);
        }

        std::string oldValReg = m_currentFunction->nextTempName("old_obj");
        auto loadOld = std::make_shared<LoadInst>(IRType::getPointer(), targetPtr, oldValReg);
        m_currentBlock->addInstruction(loadOld);

        m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhsVal, targetPtr));

        auto releaseCall = std::make_shared<CallInst>(
            IRType::getVoid(),
            "sp_rt_release",
            std::vector<IRValuePtr>{loadOld}
        );
        m_currentBlock->addInstruction(releaseCall);
    } else {
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhsVal, targetPtr));
    }

    m_lastExprValue = rhsVal;
    m_lastExprType = valType;
}

void IRBuilder::visit(const ast::BinaryExpr& node) {
    node.getLeft().accept(*this);
    IRValuePtr lhs = m_lastExprValue;
    IRTypePtr lhsType = m_lastExprType;

    node.getRight().accept(*this);
    IRValuePtr rhs = m_lastExprValue;
    IRTypePtr rhsType = m_lastExprType;

    if (!lhs || !rhs) {
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }

    // String concatenation if either operand is string/pointer
    if (node.getOp() == lexer::TokenType::Plus &&
        ((lhsType && lhsType->isPointer()) || (rhsType && rhsType->isPointer()))) {
        auto lhsStr = convertToStringValue(lhs, lhsType, &node.getLeft());
        auto rhsStr = convertToStringValue(rhs, rhsType, &node.getRight());
        std::string callReg = m_currentFunction ? m_currentFunction->nextTempName("str_cat") : "%str_cat";
        auto call = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()),
                                               "sp_rt_str_concat",
                                               std::vector<IRValuePtr>{lhsStr, rhsStr},
                                               callReg);
        if (m_currentBlock) m_currentBlock->addInstruction(call);
        m_lastExprValue = call;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }

    // String equality comparison
    if (node.getOp() == lexer::TokenType::EqualEqual || node.getOp() == lexer::TokenType::BangEqual) {
        auto semaL = m_sema.getNodeType(&node.getLeft());
        auto semaR = m_sema.getNodeType(&node.getRight());
        bool isString = (semaL && semaL->isString()) || (semaR && semaR->isString()) ||
                        ((lhsType && lhsType->isPointer()) && (rhsType && rhsType->isPointer()));
        if (isString) {
            std::string resName = m_currentFunction->nextTempName("str_eq");
            auto call = std::make_shared<CallInst>(
                IRType::getInt1(), "sp_rt_str_equals",
                std::vector<IRValuePtr>{lhs, rhs},
                resName);
            m_currentBlock->addInstruction(call);
            if (node.getOp() == lexer::TokenType::BangEqual) {
                std::string notName = m_currentFunction->nextTempName("str_ne");
                auto notInst = std::make_shared<BinaryInst>(
                    IROpcode::Xor, call, std::make_shared<ConstantInt>(IRType::getInt1(), 1), notName);
                m_currentBlock->addInstruction(notInst);
                m_lastExprValue = notInst;
            } else {
                m_lastExprValue = call;
            }
            m_lastExprType = IRType::getInt1();
            return;
        }
    }

    // Logical operations on booleans stay as i1 without integer promotion
    if (node.getOp() == lexer::TokenType::AmpAmp || node.getOp() == lexer::TokenType::PipePipe) {
        lhs = emitCastIfNeeded(lhs, IRType::getInt1());
        rhs = emitCastIfNeeded(rhs, IRType::getInt1());
        std::string resName = m_currentFunction->nextTempName("b");
        auto op = (node.getOp() == lexer::TokenType::AmpAmp) ? IROpcode::And : IROpcode::Or;
        auto resultInst = std::make_shared<BinaryInst>(op, lhs, rhs, resName);
        if (m_currentBlock) m_currentBlock->addInstruction(resultInst);
        m_lastExprValue = resultInst;
        m_lastExprType = IRType::getInt1();
        return;
    }

    // Type promotion / widening
    bool isFloat = lhsType->isFloatingPoint() || rhsType->isFloatingPoint();
    IRTypePtr commonType = isFloat ? IRType::getDouble() : (lhsType->isInt64() || rhsType->isInt64() ? IRType::getInt64() : IRType::getInt32());

    lhs = emitCastIfNeeded(lhs, commonType);
    rhs = emitCastIfNeeded(rhs, commonType);

    std::string resName = m_currentFunction->nextTempName("b");
    IRInstructionPtr resultInst = nullptr;

    switch (node.getOp()) {
        case lexer::TokenType::Plus:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FAdd : IROpcode::Add, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Minus:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FSub : IROpcode::Sub, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Star:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FMul : IROpcode::Mul, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Slash:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FDiv : IROpcode::SDiv, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Percent:
            resultInst = std::make_shared<BinaryInst>(isFloat ? IROpcode::FRem : IROpcode::SRem, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Amp:
            resultInst = std::make_shared<BinaryInst>(IROpcode::And, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Pipe:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Or, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::Caret:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Xor, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::LessLess:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Shl, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::GreaterGreater:
            resultInst = std::make_shared<BinaryInst>(IROpcode::AShr, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
        case lexer::TokenType::AmpAmp:
            resultInst = std::make_shared<BinaryInst>(IROpcode::And, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::PipePipe:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Or, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        // Comparisons
        case lexer::TokenType::EqualEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpEq : IROpcode::ICmpEq, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::BangEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpNe : IROpcode::ICmpNe, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::Less:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpLt : IROpcode::ICmpSlt, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::LessEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpLe : IROpcode::ICmpSle, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::Greater:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpGt : IROpcode::ICmpSgt, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::GreaterEqual:
            resultInst = std::make_shared<CompareInst>(isFloat ? IROpcode::FCmpGe : IROpcode::ICmpSge, lhs, rhs, resName);
            m_lastExprType = IRType::getInt1();
            break;
        default:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Add, lhs, rhs, resName);
            m_lastExprType = commonType;
            break;
    }

    m_currentBlock->addInstruction(resultInst);
    m_lastExprValue = resultInst;
}

void IRBuilder::visit(const ast::UnaryExpr& node) {
    // Handle prefix / postfix ++ and --
    if (node.getOp() == lexer::TokenType::PlusPlus || node.getOp() == lexer::TokenType::MinusMinus) {
        const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getOperand());
        if (ident) {
            auto [allocaPtr, valType] = lookupVariable(ident->getName());
            if (allocaPtr && valType) {
                std::string oldValReg = m_currentFunction->nextTempName("old");
                auto oldVal = std::make_shared<LoadInst>(valType, allocaPtr, oldValReg);
                m_currentBlock->addInstruction(oldVal);

                IROpcode op = (node.getOp() == lexer::TokenType::PlusPlus) ? IROpcode::Add : IROpcode::Sub;
                auto one = std::make_shared<ConstantInt>(valType, 1);
                std::string newValReg = m_currentFunction->nextTempName("inc");
                auto newVal = std::make_shared<BinaryInst>(op, oldVal, one, newValReg);
                m_currentBlock->addInstruction(newVal);

                m_currentBlock->addInstruction(std::make_shared<StoreInst>(newVal, allocaPtr));

                if (node.isPrefix()) {
                    m_lastExprValue = newVal;
                } else {
                    m_lastExprValue = oldVal;
                }
                m_lastExprType = valType;
                return;
            }
        }
    }

    // Raw pointer dereference *ptr
    if (node.getOp() == lexer::TokenType::Star) {
        node.getOperand().accept(*this);
        IRValuePtr ptrVal = m_lastExprValue;
        auto semaType = m_sema.getNodeType(&node);
        IRTypePtr targetType = semaType ? mapType(semaType.get()) : IRType::getInt64();
        std::string derefName = m_currentFunction->nextTempName("deref");
        auto loadInst = std::make_shared<LoadInst>(targetType, ptrVal, derefName);
        m_currentBlock->addInstruction(loadInst);
        m_lastExprValue = loadInst;
        m_lastExprType = targetType;
        return;
    }

    // Address-of &var
    if (node.getOp() == lexer::TokenType::Amp) {
        if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getOperand())) {
            auto [allocaPtr, valType] = lookupVariable(ident->getName());
            if (allocaPtr) {
                m_lastExprValue = allocaPtr;
                m_lastExprType = IRType::getPointer();
                return;
            }
        }
        if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&node.getOperand())) {
            mem->getObject().accept(*this);
            IRValuePtr objVal = m_lastExprValue;
            auto objSemaType = m_sema.getNodeType(&mem->getObject());
            std::string className = objSemaType ? objSemaType->toString() : "";
            int fieldIdx = m_vtableBuilder ? m_vtableBuilder->getFieldIndex(className, mem->getMember()) : -1;
            if (fieldIdx >= 0) {
                const auto* layout = m_vtableBuilder->getClassLayout(className);
                IRTypePtr structType = layout ? layout->structType : IRType::getPointer();
                std::string gepName = m_currentFunction->nextTempName("addr_gep");
                auto gep = std::make_shared<GetElementPtrInst>(
                    structType, objVal,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), fieldIdx)
                    },
                    gepName
                );
                m_currentBlock->addInstruction(gep);
                m_lastExprValue = gep;
                m_lastExprType = IRType::getPointer();
                return;
            }
        }
        node.getOperand().accept(*this);
        auto temp = std::make_shared<AllocaInst>(m_lastExprType, m_currentFunction->nextTempName("addr_tmp"));
        m_currentFunction->getEntryBlock()->addInstruction(temp);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(m_lastExprValue, temp));
        m_lastExprValue = temp;
        m_lastExprType = IRType::getPointer();
        return;
    }

    node.getOperand().accept(*this);
    IRValuePtr opVal = m_lastExprValue;
    IRTypePtr opType = m_lastExprType;

    std::string resName = m_currentFunction->nextTempName("u");
    IRInstructionPtr resultInst = nullptr;

    switch (node.getOp()) {
        case lexer::TokenType::Minus:
            if (opType->isFloatingPoint()) {
                resultInst = std::make_shared<BinaryInst>(IROpcode::FSub, std::make_shared<ConstantFloat>(opType, 0.0), opVal, resName);
            } else {
                resultInst = std::make_shared<BinaryInst>(IROpcode::Sub, std::make_shared<ConstantInt>(opType, 0), opVal, resName);
            }
            m_lastExprType = opType;
            break;
        case lexer::TokenType::Bang:
            resultInst = std::make_shared<CompareInst>(IROpcode::ICmpEq, opVal, std::make_shared<ConstantInt>(opType, 0), resName);
            m_lastExprType = IRType::getInt1();
            break;
        case lexer::TokenType::Tilde:
            resultInst = std::make_shared<BinaryInst>(IROpcode::Xor, opVal, std::make_shared<ConstantInt>(opType, -1), resName);
            m_lastExprType = opType;
            break;
        default:
            m_lastExprValue = opVal;
            return;
    }

    m_currentBlock->addInstruction(resultInst);
    m_lastExprValue = resultInst;
}

void IRBuilder::visit(const ast::CallExpr& node) {
    // 1. Check if callee is MemberExpr: obj.method(...) or super.method(...)
    if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&node.getCallee())) {
        std::string methodName = mem->getMember();

        // 1a. super.method(...)
        if (const auto* idObj = dynamic_cast<const ast::IdentifierExpr*>(&mem->getObject())) {
            if (idObj->getName() == "super") {
                if (m_currentClass && !m_currentClass->baseClass.empty()) {
                    std::string mangledName = m_currentClass->baseClass + "_" + methodName;
                    auto [thisAlloca, thisType] = lookupVariable("this");
                    if (thisAlloca) {
                        auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
                        m_currentBlock->addInstruction(thisVal);

                        std::vector<IRValuePtr> loweredArgs;
                        loweredArgs.push_back(thisVal);
                        for (const auto& arg : node.getArgs()) {
                            arg->accept(*this);
                            loweredArgs.push_back(m_lastExprValue);
                        }

                        auto* targetFunc = m_module->getFunction(mangledName);
                        IRTypePtr returnType = targetFunc ? targetFunc->getReturnType() : IRType::getVoid();
                        std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("scall");

                        auto callInst = std::make_shared<CallInst>(returnType, mangledName, loweredArgs, callReg);
                        m_currentBlock->addInstruction(callInst);

                        m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
                        m_lastExprType = returnType;
                        return;
                    }
                }
            }
        }

        // 1a2. Static type methods: int.parse(...), double.parse(...)
        if (const auto* idObj = dynamic_cast<const ast::IdentifierExpr*>(&mem->getObject())) {
            const std::string& targetName = idObj->getName();
            if (methodName == "parse") {
                if (targetName == "int" || targetName == "int32" || targetName == "int64") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_int") : "%parsed_int";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            "sp_rt_str_to_int",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
                if (targetName == "double") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_double") : "%parsed_double";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getDouble(),
                            "sp_rt_str_to_double",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getDouble();
                        return;
                    }
                }
                if (targetName == "float") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_float") : "%parsed_float";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getFloat(),
                            "sp_rt_str_to_float",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getFloat();
                        return;
                    }
                }
                if (targetName == "bool") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        IRValuePtr strVal = m_lastExprValue;
                        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("parsed_bool") : "%parsed_bool";
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt1(),
                            "sp_rt_str_to_bool",
                            std::vector<IRValuePtr>{strVal},
                            resName
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt1();
                        return;
                    }
                }
            }

            if (targetName == "IO" || targetName == "Console") {
                if (methodName == "println") {
                    if (node.getArgs().empty()) {
                        auto emptyStr = m_module->getOrCreateStringLiteral("");
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{emptyStr});
                        m_currentBlock->addInstruction(callInst);
                    } else {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "print") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "eprintln") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    } else {
                        auto emptyStr = m_module->getOrCreateStringLiteral("");
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{emptyStr});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "eprint") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprint_string", std::vector<IRValuePtr>{m_lastExprValue});
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "readLine" || methodName == "readln") {
                    std::string resName = m_currentFunction ? m_currentFunction->nextTempName("readline") : "%readline";
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_read_line",
                        std::vector<IRValuePtr>{},
                        resName
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "flush") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_flush", std::vector<IRValuePtr>{});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "eflush") {
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eflush", std::vector<IRValuePtr>{});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (targetName == "String") {
                if (methodName == "upper" || methodName == "lower" || methodName == "trim" || methodName == "reverse") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        std::string fnName = (methodName == "upper") ? "sp_rt_str_upper" :
                                             (methodName == "lower") ? "sp_rt_str_lower" :
                                             (methodName == "trim") ? "sp_rt_str_trim" : "sp_rt_str_reverse";
                        std::string resReg = m_currentFunction->nextTempName("str_res");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(IRType::getInt8()),
                            fnName,
                            std::vector<IRValuePtr>{m_lastExprValue},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer(IRType::getInt8());
                        return;
                    }
                }
                if (methodName == "length") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        std::string resReg = m_currentFunction->nextTempName("str_len");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            "sp_rt_str_len",
                            std::vector<IRValuePtr>{m_lastExprValue},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
                if (methodName == "contains") {
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto subVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_has");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt1(),
                            "sp_rt_str_contains",
                            std::vector<IRValuePtr>{sVal, subVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt1();
                        return;
                    }
                }
                if (methodName == "indexOf") {
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto subVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_idx");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            "sp_rt_str_index_of",
                            std::vector<IRValuePtr>{sVal, subVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
                if (methodName == "replace") {
                    if (node.getArgs().size() >= 3) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto fromVal = m_lastExprValue;
                        node.getArgs()[2]->accept(*this);
                        auto toVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_rep");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(IRType::getInt8()),
                            "sp_rt_str_replace",
                            std::vector<IRValuePtr>{sVal, fromVal, toVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer(IRType::getInt8());
                        return;
                    }
                }
                if (methodName == "substring") {
                    if (node.getArgs().size() >= 3) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto startVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        node.getArgs()[2]->accept(*this);
                        auto endVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        std::string resReg = m_currentFunction->nextTempName("str_sub");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(IRType::getInt8()),
                            "sp_rt_str_substring",
                            std::vector<IRValuePtr>{sVal, startVal, endVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer(IRType::getInt8());
                        return;
                    }
                }
                if (methodName == "split") {
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto sVal = m_lastExprValue;
                        node.getArgs()[1]->accept(*this);
                        auto delimVal = m_lastExprValue;
                        std::string resReg = m_currentFunction->nextTempName("str_split");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getPointer(),
                            "sp_rt_str_split",
                            std::vector<IRValuePtr>{sVal, delimVal},
                            resReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getPointer();
                        return;
                    }
                }
            }

            if (targetName == "Math") {
                if (methodName == "sqrt" || methodName == "sin" || methodName == "cos" || methodName == "tan" ||
                    methodName == "asin" || methodName == "acos" || methodName == "atan" ||
                    methodName == "floor" || methodName == "ceil" || methodName == "round" ||
                    methodName == "log" || methodName == "log10" || methodName == "log2" || methodName == "exp") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto val = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
                        std::string fn = "sp_rt_math_" + methodName;
                        std::string reg = m_currentFunction->nextTempName("math_res");
                        auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{val}, reg);
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getDouble();
                        return;
                    }
                }
                if (methodName == "pow" || methodName == "atan2") {
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto v1 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
                        node.getArgs()[1]->accept(*this);
                        auto v2 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
                        std::string fn = "sp_rt_math_" + methodName;
                        std::string reg = m_currentFunction->nextTempName("math_res");
                        auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getDouble();
                        return;
                    }
                }
                if (methodName == "abs") {
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        auto val = m_lastExprValue;
                        auto type = m_lastExprType;
                        auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
                        bool isFlt = (type && type->isFloatingPoint()) || (semaType && (semaType->isFloat() || semaType->isDouble()));
                        if (isFlt) {
                            val = emitCastIfNeeded(val, IRType::getDouble());
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), "sp_rt_math_abs_double", std::vector<IRValuePtr>{val}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getDouble();
                        } else {
                            val = emitCastIfNeeded(val, IRType::getInt64());
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_math_abs_int", std::vector<IRValuePtr>{val}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        }
                        return;
                    }
                }
                if (methodName == "min" || methodName == "max") {
                    if (node.getArgs().size() >= 2) {
                        node.getArgs()[0]->accept(*this);
                        auto v1 = m_lastExprValue;
                        auto t1 = m_lastExprType;
                        auto semaT1 = m_sema.getNodeType(node.getArgs()[0].get());
                        node.getArgs()[1]->accept(*this);
                        auto v2 = m_lastExprValue;
                        auto t2 = m_lastExprType;
                        auto semaT2 = m_sema.getNodeType(node.getArgs()[1].get());
                        bool isFlt = (t1 && t1->isFloatingPoint()) || (t2 && t2->isFloatingPoint()) ||
                                     (semaT1 && (semaT1->isFloat() || semaT1->isDouble())) ||
                                     (semaT2 && (semaT2->isFloat() || semaT2->isDouble()));
                        if (isFlt) {
                            v1 = emitCastIfNeeded(v1, IRType::getDouble());
                            v2 = emitCastIfNeeded(v2, IRType::getDouble());
                            std::string fn = (methodName == "min") ? "sp_rt_math_min_double" : "sp_rt_math_max_double";
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getDouble();
                        } else {
                            v1 = emitCastIfNeeded(v1, IRType::getInt64());
                            v2 = emitCastIfNeeded(v2, IRType::getInt64());
                            std::string fn = (methodName == "min") ? "sp_rt_math_min_int" : "sp_rt_math_max_int";
                            std::string reg = m_currentFunction->nextTempName("math_res");
                            auto callInst = std::make_shared<CallInst>(IRType::getInt64(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        }
                        return;
                    }
                }
            }

            if (targetName == "FS" || targetName == "File") {
                if (methodName == "readText" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_fs_read_text", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if ((methodName == "writeText" || methodName == "appendText") && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto c = m_lastExprValue;
                    std::string fn = (methodName == "writeText") ? "sp_rt_fs_write_text" : "sp_rt_fs_append_text";
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), fn, std::vector<IRValuePtr>{p, c}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "exists" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_exists", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if ((methodName == "delete" || methodName == "deleteFile") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_delete", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "createDir" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_create_dir", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "listDir" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_fs_list_dir", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
                if (methodName == "isFile" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_file", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "isDir" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto p = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("fs_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_dir", std::vector<IRValuePtr>{p}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }

            if (targetName == "Time") {
                if (methodName == "now") {
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ms", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "nowSeconds") {
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_secs", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "nowNanos") {
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ns", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if ((methodName == "sleep" || methodName == "sleepSeconds") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string fn = (methodName == "sleep") ? "sp_rt_time_sleep_ms" : "sp_rt_time_sleep_secs";
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), fn, std::vector<IRValuePtr>{ms});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (methodName == "format" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_time_format_iso", std::vector<IRValuePtr>{ms}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "parse" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto s = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("time_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_parse_iso", std::vector<IRValuePtr>{s}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }

            if (targetName == "Duration") {
                if ((methodName == "fromMillis" || methodName == "fromSeconds" ||
                     methodName == "fromMinutes" || methodName == "fromHours") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto val = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string fn = "sp_rt_duration_from_millis";
                    if (methodName == "fromSeconds") fn = "sp_rt_duration_from_seconds";
                    else if (methodName == "fromMinutes") fn = "sp_rt_duration_from_minutes";
                    else if (methodName == "fromHours") fn = "sp_rt_duration_from_hours";
                    std::string reg = m_currentFunction->nextTempName("dur");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(), fn, std::vector<IRValuePtr>{val}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
            }

            if (targetName == "OS") {
                if (methodName == "getEnv" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto n = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_get_env", std::vector<IRValuePtr>{n}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "setEnv" && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto n = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto v = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_os_set_env", std::vector<IRValuePtr>{n, v}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
                if (methodName == "platform") {
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_platform", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "arch") {
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_arch", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "args") {
                    std::string reg = m_currentFunction->nextTempName("os_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_os_args", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
                if (methodName == "exit" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto code = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_os_exit", std::vector<IRValuePtr>{code});
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            }

            if (targetName == "Process") {
                if (methodName == "exec" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto cmd = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_exec", std::vector<IRValuePtr>{cmd}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if (methodName == "spawn" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto cmd = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_spawn", std::vector<IRValuePtr>{cmd}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
                if ((methodName == "run" || methodName == "execCapture") && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto cmd = m_lastExprValue;
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_process_run", std::vector<IRValuePtr>{cmd}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
                if (methodName == "pid") {
                    std::string reg = m_currentFunction->nextTempName("proc_res");
                    auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_pid", std::vector<IRValuePtr>{}, reg);
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }

            if (targetName == "Result" || targetName.rfind("Result<", 0) == 0) {
                if (methodName == "Ok" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    m_lastExprValue = emitCreateTaggedUnion("Result", 0, m_lastExprValue);
                    m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
                if (methodName == "Err" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    m_lastExprValue = emitCreateTaggedUnion("Result", 1, m_lastExprValue);
                    m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            }
            if (targetName == "Option" || targetName.rfind("Option<", 0) == 0) {
                if (methodName == "Some" && !node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    m_lastExprValue = emitCreateTaggedUnion("Option", 0, m_lastExprValue);
                    m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
                if (methodName == "None") {
                    m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
                    m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            }

            auto enumUserType = m_sema.getTypeContext().findUserType(targetName);
            if (enumUserType && enumUserType->isEnum()) {
                auto en = std::static_pointer_cast<sema::EnumType>(enumUserType);
                if (en && en->isTaggedUnion() && en->hasVariant(methodName)) {
                    int64_t variantVal = en->getVariantValue(methodName);
                    IRValuePtr payloadVal = nullptr;
                    if (!node.getArgs().empty()) {
                        node.getArgs()[0]->accept(*this);
                        payloadVal = m_lastExprValue;
                    }
                    m_lastExprValue = emitCreateTaggedUnion(targetName, static_cast<int32_t>(variantVal), payloadVal);
                    m_lastExprType = ir::IRType::getStruct("struct." + targetName, {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            }
        }

        // 1b. obj.method(...)
        mem->getObject().accept(*this);
        IRValuePtr objVal = m_lastExprValue;
        if (!objVal) {
            m_lastExprValue = nullptr;
            m_lastExprType = IRType::getVoid();
            return;
        }

        auto semaObjType = m_sema.getNodeType(&mem->getObject());
        std::string objTypeName = semaObjType ? semaObjType->toString() : "";
        bool isTaggedUnionObj = false;
        std::string tuBaseName;
        if (objTypeName == "Result" || objTypeName.rfind("Result<", 0) == 0) {
            isTaggedUnionObj = true;
            tuBaseName = "Result";
        } else if (objTypeName == "Option" || objTypeName.rfind("Option<", 0) == 0) {
            isTaggedUnionObj = true;
            tuBaseName = "Option";
        } else if (semaObjType && semaObjType->isEnum()) {
            auto en = std::static_pointer_cast<sema::EnumType>(semaObjType);
            if (en && en->isTaggedUnion()) {
                isTaggedUnionObj = true;
                tuBaseName = en->getName();
            }
        }

        if (isTaggedUnionObj) {
            IRValuePtr objPtr = objVal;
            auto structType = ir::IRType::getStruct("struct." + tuBaseName, {IRType::getInt32(), IRType::getInt64()});
            if (objVal->getType()->isStruct()) {
                auto tempAlloca = std::make_shared<AllocaInst>(objVal->getType(), m_currentFunction->nextTempName("tu_tmp"));
                m_currentFunction->getEntryBlock()->addInstruction(tempAlloca);
                m_currentBlock->addInstruction(std::make_shared<StoreInst>(objVal, tempAlloca));
                objPtr = tempAlloca;
            }

            if (methodName == "isOk" || methodName == "isSome") {
                auto gepTag = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                    },
                    m_currentFunction->nextTempName("tag_ptr")
                );
                m_currentBlock->addInstruction(gepTag);
                auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("tag"));
                m_currentBlock->addInstruction(loadTag);

                auto cmp = std::make_shared<CompareInst>(IROpcode::ICmpEq, loadTag,
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    m_currentFunction->nextTempName("is_ok"));
                m_currentBlock->addInstruction(cmp);
                m_lastExprValue = cmp;
                m_lastExprType = IRType::getInt1();
                return;
            }

            if (methodName == "isErr" || methodName == "isNone") {
                auto gepTag = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                    },
                    m_currentFunction->nextTempName("tag_ptr")
                );
                m_currentBlock->addInstruction(gepTag);
                auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("tag"));
                m_currentBlock->addInstruction(loadTag);

                auto cmp = std::make_shared<CompareInst>(IROpcode::ICmpNe, loadTag,
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    m_currentFunction->nextTempName("is_err"));
                m_currentBlock->addInstruction(cmp);
                m_lastExprValue = cmp;
                m_lastExprType = IRType::getInt1();
                return;
            }

            if (methodName == "unwrap") {
                auto gepPayload = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 1)
                    },
                    m_currentFunction->nextTempName("payload_ptr")
                );
                m_currentBlock->addInstruction(gepPayload);
                auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("payload"));
                m_currentBlock->addInstruction(loadPayload);

                auto callSemaType = m_sema.getNodeType(&node);
                IRTypePtr targetType = callSemaType ? mapType(callSemaType.get()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(loadPayload, targetType);
                m_lastExprType = targetType;
                return;
            }

            if (methodName == "unwrapErr") {
                auto gepPayload = std::make_shared<GetElementPtrInst>(
                    structType,
                    objPtr,
                    std::vector<IRValuePtr>{
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        std::make_shared<ConstantInt>(IRType::getInt32(), 1)
                    },
                    m_currentFunction->nextTempName("payload_ptr")
                );
                m_currentBlock->addInstruction(gepPayload);
                auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("payload"));
                m_currentBlock->addInstruction(loadPayload);

                auto callSemaType = m_sema.getNodeType(&node);
                IRTypePtr targetType = callSemaType ? mapType(callSemaType.get()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(loadPayload, targetType);
                m_lastExprType = targetType;
                return;
            }

            if (methodName == "unwrapOr") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    IRValuePtr defVal = m_lastExprValue;

                    auto gepTag = std::make_shared<GetElementPtrInst>(
                        structType,
                        objPtr,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                        },
                        m_currentFunction->nextTempName("tag_ptr")
                    );
                    m_currentBlock->addInstruction(gepTag);
                    auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("tag"));
                    m_currentBlock->addInstruction(loadTag);

                    auto gepPayload = std::make_shared<GetElementPtrInst>(
                        structType,
                        objPtr,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), 1)
                        },
                        m_currentFunction->nextTempName("payload_ptr")
                    );
                    m_currentBlock->addInstruction(gepPayload);
                    auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("payload"));
                    m_currentBlock->addInstruction(loadPayload);

                    auto callSemaType = m_sema.getNodeType(&node);
                    IRTypePtr targetType = callSemaType ? mapType(callSemaType.get()) : (defVal ? defVal->getType() : IRType::getInt64());
                    auto unwrapped = emitCastIfNeeded(loadPayload, targetType);
                    defVal = emitCastIfNeeded(defVal, targetType);

                    auto okBB = createBlock("unwrap_ok");
                    auto defBB = createBlock("unwrap_def");
                    auto mergeBB = createBlock("unwrap_merge");
                    auto resAlloca = std::make_shared<AllocaInst>(targetType, m_currentFunction->nextTempName("unwrapped_res"));
                    m_currentFunction->getEntryBlock()->addInstruction(resAlloca);

                    auto cmp = std::make_shared<CompareInst>(IROpcode::ICmpEq, loadTag,
                        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                        m_currentFunction->nextTempName("is_ok"));
                    m_currentBlock->addInstruction(cmp);
                    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(cmp, okBB->getName(), defBB->getName()));

                    setInsertPoint(okBB);
                    okBB->addInstruction(std::make_shared<StoreInst>(unwrapped, resAlloca));
                    okBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    setInsertPoint(defBB);
                    defBB->addInstruction(std::make_shared<StoreInst>(defVal, resAlloca));
                    defBB->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

                    setInsertPoint(mergeBB);
                    auto loadRes = std::make_shared<LoadInst>(targetType, resAlloca, m_currentFunction->nextTempName("unwrapped_val"));
                    mergeBB->addInstruction(loadRes);
                    m_lastExprValue = loadRes;
                    m_lastExprType = targetType;
                    return;
                }
            }
        }
        if (semaObjType && semaObjType->isClass() && semaObjType->toString() == "Duration") {
            if (methodName == "toMillis") {
                std::string reg = m_currentFunction->nextTempName("dur_ms");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_duration_to_millis",
                    std::vector<IRValuePtr>{objVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "toSeconds") {
                std::string reg = m_currentFunction->nextTempName("dur_s");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_duration_to_seconds",
                    std::vector<IRValuePtr>{objVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
        }

        if (semaObjType && semaObjType->isClass() && semaObjType->toString() == "Arena") {
            if (methodName == "alloc" && !node.getArgs().empty()) {
                node.getArgs()[0]->accept(*this);
                auto szVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                auto alignVal = std::make_shared<ConstantInt>(IRType::getInt64(), 8);
                std::string reg = m_currentFunction->nextTempName("arena_ptr");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_arena_alloc",
                    std::vector<IRValuePtr>{objVal, szVal, alignVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "allocAligned" && node.getArgs().size() >= 2) {
                node.getArgs()[0]->accept(*this);
                auto szVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                node.getArgs()[1]->accept(*this);
                auto alignVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                std::string reg = m_currentFunction->nextTempName("arena_ptr");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_arena_alloc",
                    std::vector<IRValuePtr>{objVal, szVal, alignVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "reset") {
                auto callInst = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_arena_reset",
                    std::vector<IRValuePtr>{objVal}
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = nullptr;
                m_lastExprType = IRType::getVoid();
                return;
            }
            if (methodName == "destroy") {
                auto callInst = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    "sp_rt_arena_destroy",
                    std::vector<IRValuePtr>{objVal}
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = nullptr;
                m_lastExprType = IRType::getVoid();
                return;
            }
            if (methodName == "allocatedBytes") {
                std::string reg = m_currentFunction->nextTempName("arena_bytes");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_arena_allocated_bytes",
                    std::vector<IRValuePtr>{objVal},
                    reg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
        }

        if (semaObjType && semaObjType->isString()) {
            if (methodName == "length" || methodName == "size") {
                std::string lenReg = m_currentFunction->nextTempName("slen");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_str_len",
                    std::vector<IRValuePtr>{objVal},
                    lenReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
                return;
            }
            if (methodName == "upper") {
                std::string upReg = m_currentFunction->nextTempName("supper");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_upper",
                    std::vector<IRValuePtr>{objVal},
                    upReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "lower") {
                std::string lowReg = m_currentFunction->nextTempName("slower");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_lower",
                    std::vector<IRValuePtr>{objVal},
                    lowReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "trim") {
                std::string trimReg = m_currentFunction->nextTempName("strim");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_trim",
                    std::vector<IRValuePtr>{objVal},
                    trimReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "contains") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string contReg = m_currentFunction->nextTempName("scont");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt1(),
                        "sp_rt_str_contains",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        contReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }
            if (methodName == "startsWith") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string swReg = m_currentFunction->nextTempName("ssw");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt1(),
                        "sp_rt_str_starts_with",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        swReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }
            if (methodName == "endsWith") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string ewReg = m_currentFunction->nextTempName("sew");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt1(),
                        "sp_rt_str_ends_with",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        ewReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            }
            if (methodName == "indexOf") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string idxReg = m_currentFunction->nextTempName("sidx");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt64(),
                        "sp_rt_str_index_of",
                        std::vector<IRValuePtr>{objVal, m_lastExprValue},
                        idxReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }
            if (methodName == "replace") {
                if (node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto fromVal = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto toVal = m_lastExprValue;
                    std::string repReg = m_currentFunction->nextTempName("srep");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_str_replace",
                        std::vector<IRValuePtr>{objVal, fromVal, toVal},
                        repReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
            }
            if (methodName == "substring") {
                if (node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto startVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    node.getArgs()[1]->accept(*this);
                    auto endVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string subReg = m_currentFunction->nextTempName("ssub");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_str_substring",
                        std::vector<IRValuePtr>{objVal, startVal, endVal},
                        subReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
            }
            if (methodName == "charAt") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string charReg = m_currentFunction->nextTempName("schar");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt8(),
                        "sp_rt_str_char_at",
                        std::vector<IRValuePtr>{objVal, idxVal},
                        charReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt8();
                    return;
                }
            }
            if (methodName == "split") {
                IRValuePtr delimVal = nullptr;
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    delimVal = m_lastExprValue;
                } else {
                    delimVal = m_module->getOrCreateStringLiteral("");
                }
                std::string splitReg = m_currentFunction->nextTempName("ssplit");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(),
                    "sp_rt_str_split",
                    std::vector<IRValuePtr>{objVal, delimVal},
                    splitReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer();
                return;
            }
            if (methodName == "reverse") {
                std::string revReg = m_currentFunction->nextTempName("srev");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getPointer(IRType::getInt8()),
                    "sp_rt_str_reverse",
                    std::vector<IRValuePtr>{objVal},
                    revReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getPointer(IRType::getInt8());
                return;
            }
            if (methodName == "repeat") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto countVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string repReg = m_currentFunction->nextTempName("srepeat");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(IRType::getInt8()),
                        "sp_rt_str_repeat",
                        std::vector<IRValuePtr>{objVal, countVal},
                        repReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer(IRType::getInt8());
                    return;
                }
            }
            if (methodName == "isEmpty") {
                std::string emptyReg = m_currentFunction->nextTempName("sempty");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt1(),
                    "sp_rt_str_is_empty",
                    std::vector<IRValuePtr>{objVal},
                    emptyReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt1();
                return;
            }
        }

        bool isList = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("List", 0) == 0;
        bool isMap = semaObjType && (semaObjType->isMap() || (semaObjType->isClass() && semaObjType->toString().rfind("Map", 0) == 0));
        bool isSet = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("Set", 0) == 0;
        bool isStack = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("Stack", 0) == 0;
        bool isQueue = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("Queue", 0) == 0;
        bool isPQ = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("PriorityQueue", 0) == 0;

        if (semaObjType && (semaObjType->isArray() || isList || isMap || isSet || isStack || isQueue || isPQ)) {
            if (methodName == "length" || methodName == "size") {
                std::string fnName = isSet ? "sp_rt_set_size"
                                     : isStack ? "sp_rt_stack_size"
                                     : isQueue ? "sp_rt_queue_size"
                                     : isPQ ? "sp_rt_pq_size"
                                     : semaObjType->isArray() ? "sp_rt_array_length"
                                     : isMap ? "sp_rt_map_size"
                                     : "sp_rt_list_size";
                std::string lenReg = m_currentFunction->nextTempName("len");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    lenReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = emitCastIfNeeded(callInst, IRType::getInt32());
                m_lastExprType = IRType::getInt32();
                return;
            } else if (methodName == "isEmpty") {
                std::string fnName = isSet ? "sp_rt_set_is_empty"
                                     : isStack ? "sp_rt_stack_is_empty"
                                     : isQueue ? "sp_rt_queue_is_empty"
                                     : isPQ ? "sp_rt_pq_is_empty"
                                     : semaObjType->isArray() ? "sp_rt_array_is_empty"
                                     : isMap ? "sp_rt_map_is_empty"
                                     : "sp_rt_list_is_empty";
                std::string emptyReg = m_currentFunction->nextTempName("is_empty");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt1(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    emptyReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt1();
                return;
            } else if (methodName == "clear") {
                std::string fnName = isSet ? "sp_rt_set_clear"
                                     : isStack ? "sp_rt_stack_clear"
                                     : isQueue ? "sp_rt_queue_clear"
                                     : isPQ ? "sp_rt_pq_clear"
                                     : isMap ? "sp_rt_map_clear"
                                     : "sp_rt_list_clear";
                auto callInst = std::make_shared<CallInst>(
                    IRType::getVoid(),
                    fnName,
                    std::vector<IRValuePtr>{objVal}
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = nullptr;
                m_lastExprType = IRType::getVoid();
                return;
            } else if (methodName == "removeAt") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getVoid(),
                        "sp_rt_list_remove_at",
                        std::vector<IRValuePtr>{objVal, idxVal}
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "push" || methodName == "add" || methodName == "enqueue") {
                if (isPQ && node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto itemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    node.getArgs()[1]->accept(*this);
                    auto prioVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getVoid(),
                        "sp_rt_pq_push",
                        std::vector<IRValuePtr>{objVal, itemVal, prioVal}
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    if (isSet) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_add_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_add_i64",
                                std::vector<IRValuePtr>{objVal, elemVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else if (isStack) {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            "sp_rt_stack_push",
                            std::vector<IRValuePtr>{objVal, elemVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    } else if (isQueue) {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            "sp_rt_queue_enqueue",
                            std::vector<IRValuePtr>{objVal, elemVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    } else {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            "sp_rt_list_push_i64",
                            std::vector<IRValuePtr>{objVal, elemVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "contains" || methodName == "containsKey") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    std::string hasReg = m_currentFunction->nextTempName("has_item");
                    if (isSet) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_set_contains_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_set_contains_i64",
                                std::vector<IRValuePtr>{objVal, elemVal},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_map_contains_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt1(),
                                "sp_rt_map_contains_i64",
                                std::vector<IRValuePtr>{objVal, elemVal},
                                hasReg
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else {
                        auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt1(),
                            "sp_rt_list_contains_i64",
                            std::vector<IRValuePtr>{objVal, elemVal},
                            hasReg
                        );
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = m_currentBlock->getInstructions().back();
                    m_lastExprType = IRType::getInt1();
                    return;
                }
            } else if (methodName == "remove") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    if (isSet) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_remove_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_set_remove_i64",
                                std::vector<IRValuePtr>{objVal, elemVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        if (m_lastExprType && m_lastExprType->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_remove_str",
                                std::vector<IRValuePtr>{objVal, m_lastExprValue}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_remove_i64",
                                std::vector<IRValuePtr>{objVal, elemVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "pop" || methodName == "dequeue") {
                std::string popReg = m_currentFunction->nextTempName("pop_val");
                std::string fnName = isPQ ? "sp_rt_pq_pop"
                                     : isQueue ? "sp_rt_queue_dequeue"
                                     : isStack ? "sp_rt_stack_pop"
                                     : "sp_rt_list_pop_i64";
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    popReg
                );
                m_currentBlock->addInstruction(callInst);
                auto semaCallType = m_sema.getNodeType(&node);
                IRTypePtr retIRType = semaCallType ? mapType(semaCallType->toString()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(callInst, retIRType);
                m_lastExprType = retIRType;
                return;
            } else if (methodName == "peek") {
                std::string peekReg = m_currentFunction->nextTempName("peek_val");
                std::string fnName = isPQ ? "sp_rt_pq_peek"
                                     : isQueue ? "sp_rt_queue_peek"
                                     : "sp_rt_stack_peek";
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    peekReg
                );
                m_currentBlock->addInstruction(callInst);
                auto semaCallType = m_sema.getNodeType(&node);
                IRTypePtr retIRType = semaCallType ? mapType(semaCallType->toString()) : IRType::getInt64();
                m_lastExprValue = emitCastIfNeeded(callInst, retIRType);
                m_lastExprType = retIRType;
                return;
            } else if (methodName == "peekPriority") {
                std::string prioReg = m_currentFunction->nextTempName("prio_val");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_pq_peek_priority",
                    std::vector<IRValuePtr>{objVal},
                    prioReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = emitCastIfNeeded(callInst, IRType::getInt32());
                m_lastExprType = IRType::getInt32();
                return;
            } else if (methodName == "get") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = m_lastExprValue;
                    if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        std::string getReg = m_currentFunction->nextTempName("map_get");
                        if (idxVal->getType()->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt64(),
                                "sp_rt_map_get_str",
                                std::vector<IRValuePtr>{objVal, idxVal},
                                getReg
                            );
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        } else {
                            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getInt64(),
                                "sp_rt_map_get_i64",
                                std::vector<IRValuePtr>{objVal, idxVal},
                                getReg
                            );
                            m_currentBlock->addInstruction(callInst);
                            m_lastExprValue = callInst;
                            m_lastExprType = IRType::getInt64();
                        }
                        return;
                    } else {
                        idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                        std::string getReg = m_currentFunction->nextTempName("list_get");
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getInt64(),
                            semaObjType->isArray() ? "sp_rt_array_get_i64" : "sp_rt_list_get_i64",
                            std::vector<IRValuePtr>{objVal, idxVal},
                            getReg
                        );
                        m_currentBlock->addInstruction(callInst);
                        m_lastExprValue = callInst;
                        m_lastExprType = IRType::getInt64();
                        return;
                    }
                }
            } else if (methodName == "set") {
                if (node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto idxVal = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto valVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    if (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) {
                        if (idxVal->getType()->isPointer()) {
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_set_str",
                                std::vector<IRValuePtr>{objVal, idxVal, valVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        } else {
                            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                            auto callInst = std::make_shared<CallInst>(
                                IRType::getVoid(),
                                "sp_rt_map_set_i64",
                                std::vector<IRValuePtr>{objVal, idxVal, valVal}
                            );
                            m_currentBlock->addInstruction(callInst);
                        }
                    } else {
                        idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
                        auto callInst = std::make_shared<CallInst>(
                            IRType::getVoid(),
                            semaObjType->isArray() ? "sp_rt_array_set_i64" : "sp_rt_list_set_i64",
                            std::vector<IRValuePtr>{objVal, idxVal, valVal}
                        );
                        m_currentBlock->addInstruction(callInst);
                    }
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "isEmpty") {
                std::string fnName = isSet ? "sp_rt_set_size"
                                     : semaObjType->isArray() ? "sp_rt_array_length"
                                     : (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) ? "sp_rt_map_size"
                                     : "sp_rt_list_size";
                std::string lenReg = m_currentFunction->nextTempName("len");
                auto lenCall = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    fnName,
                    std::vector<IRValuePtr>{objVal},
                    lenReg
                );
                m_currentBlock->addInstruction(lenCall);
                std::string cmpReg = m_currentFunction->nextTempName("is_empty");
                auto cmpInst = std::make_shared<CompareInst>(
                    IROpcode::ICmpEq,
                    lenCall,
                    std::make_shared<ConstantInt>(IRType::getInt64(), 0),
                    cmpReg
                );
                m_currentBlock->addInstruction(cmpInst);
                m_lastExprValue = cmpInst;
                m_lastExprType = IRType::getInt1();
                return;
            } else if (methodName == "map") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    std::string mapReg = m_currentFunction->nextTempName("map_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(),
                        semaObjType->isArray() ? "sp_rt_array_map" : "sp_rt_list_map",
                        std::vector<IRValuePtr>{objVal, fnVal},
                        mapReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
            } else if (methodName == "filter") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    std::string fltReg = m_currentFunction->nextTempName("flt_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getPointer(),
                        semaObjType->isArray() ? "sp_rt_array_filter" : "sp_rt_list_filter",
                        std::vector<IRValuePtr>{objVal, fnVal},
                        fltReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getPointer();
                    return;
                }
            } else if (methodName == "reduce") {
                if (node.getArgs().size() >= 2) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    node.getArgs()[1]->accept(*this);
                    auto initVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    std::string redReg = m_currentFunction->nextTempName("red_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt64(),
                        semaObjType->isArray() ? "sp_rt_array_reduce" : "sp_rt_list_reduce",
                        std::vector<IRValuePtr>{objVal, fnVal, initVal},
                        redReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                } else if (node.getArgs().size() == 1) {
                    node.getArgs()[0]->accept(*this);
                    auto fnVal = m_lastExprValue;
                    auto initVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
                    std::string redReg = m_currentFunction->nextTempName("red_res");
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getInt64(),
                        semaObjType->isArray() ? "sp_rt_array_reduce" : "sp_rt_list_reduce",
                        std::vector<IRValuePtr>{objVal, fnVal, initVal},
                        redReg
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = callInst;
                    m_lastExprType = IRType::getInt64();
                    return;
                }
            }
        }

        std::string className;
        if (const auto* identObj = dynamic_cast<const ast::IdentifierExpr*>(&mem->getObject())) {
            if (identObj->getName() == "this" && m_currentClass) {
                className = m_currentClass->name;
            }
        }
        if (className.empty()) {
            auto semaType = m_sema.getNodeType(&mem->getObject());
            if (semaType && semaType->isNullable()) {
                const auto* nullT = static_cast<const sema::NullableType*>(semaType.get());
                if (nullT && nullT->getUnderlyingType()) {
                    semaType = nullT->getUnderlyingType();
                }
            }
            if (semaType && semaType->isClass()) {
                className = codegen::VTableBuilder::mangleGenericName(semaType->toString());
            }
        }
        if (className.empty() && m_currentClass) {
            className = m_currentClass->name;
        }

        const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(className) : nullptr;
        int slotIdx = (m_vtableBuilder && !className.empty()) ? m_vtableBuilder->getMethodVTableSlot(className, methodName) : -1;

        if (layout && slotIdx >= 0) {
            // Polymorphic virtual dispatch via vtable!
            auto gepVTable = std::make_shared<GetElementPtrInst>(
                layout->structType,
                objVal,
                std::vector<IRValuePtr>{
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                },
                m_currentFunction->nextTempName("vt_slot")
            );
            m_currentBlock->addInstruction(gepVTable);

            auto loadVTable = std::make_shared<LoadInst>(
                IRType::getPointer(),
                gepVTable,
                m_currentFunction->nextTempName("vtable")
            );
            m_currentBlock->addInstruction(loadVTable);

            auto gepSlot = std::make_shared<GetElementPtrInst>(
                IRType::getPointer(),
                loadVTable,
                std::vector<IRValuePtr>{
                    std::make_shared<ConstantInt>(IRType::getInt32(), slotIdx)
                },
                m_currentFunction->nextTempName("slot_ptr")
            );
            m_currentBlock->addInstruction(gepSlot);

            auto loadFnPtr = std::make_shared<LoadInst>(
                IRType::getPointer(),
                gepSlot,
                m_currentFunction->nextTempName("fn_ptr")
            );
            m_currentBlock->addInstruction(loadFnPtr);

            IRTypePtr returnType = IRType::getVoid();
            const auto& slotInfo = layout->vtableSlots[slotIdx];
            if (slotInfo.functionType && slotInfo.functionType->isFunction()) {
                auto fnType = std::dynamic_pointer_cast<IRFunctionType>(slotInfo.functionType);
                if (fnType) returnType = fnType->getReturnType();
            } else {
                auto* existing = m_module->getFunction(slotInfo.mangledName);
                if (existing) returnType = existing->getReturnType();
            }

            std::vector<IRValuePtr> loweredArgs;
            loweredArgs.push_back(objVal);
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }

            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("vcall");
            auto callInst = std::make_shared<CallInst>(returnType, loadFnPtr, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        } else {
            // Direct/static call
            std::string mangledName = (className.empty() ? "" : className + "_") + methodName;
            auto* targetFunc = m_module->getFunction(mangledName);
            IRTypePtr returnType = targetFunc ? targetFunc->getReturnType() : IRType::getInt32();

            std::vector<IRValuePtr> loweredArgs;
            if (targetFunc && !targetFunc->getParams().empty() && targetFunc->getParams()[0]->getName() == "%this") {
                loweredArgs.push_back(objVal);
            }
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }

            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("c");
            auto callInst = std::make_shared<CallInst>(returnType, mangledName, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        }
    }

    const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getCallee());
    std::string calleeName = ident ? ident->getName() : "";
    if (node.isGeneric() && !calleeName.empty()) {
        std::string monoName = calleeName;
        for (const auto& arg : node.getTypeArgs()) {
            monoName += "_" + arg;
        }
        if (!m_module->getFunction(monoName) && m_genericFunctions.count(calleeName)) {
            compileSpecializedFunction(*m_genericFunctions.at(calleeName), node.getTypeArgs());
        }
        calleeName = monoName;
    }

    // Super constructor delegation
    if (calleeName == "super") {
        if (m_currentClass && !m_currentClass->baseClass.empty()) {
            std::string baseCtor = m_currentClass->baseClass + "_" + m_currentClass->baseClass;
            auto [thisAlloca, thisType] = lookupVariable("this");
            if (thisAlloca) {
                auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
                m_currentBlock->addInstruction(thisVal);

                std::vector<IRValuePtr> loweredArgs;
                loweredArgs.push_back(thisVal);
                for (const auto& arg : node.getArgs()) {
                    arg->accept(*this);
                    loweredArgs.push_back(m_lastExprValue);
                }

                auto callInst = std::make_shared<CallInst>(IRType::getVoid(), baseCtor, loweredArgs);
                m_currentBlock->addInstruction(callInst);

                // Restore derived class vtable pointer into slot 0
                if (m_currentClass->hasVTable) {
                    auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), m_currentClass->vtableName);
                    auto gep = std::make_shared<GetElementPtrInst>(
                        m_currentClass->structType,
                        thisVal,
                        std::vector<IRValuePtr>{
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                        },
                        m_currentFunction->nextTempName("vptr_restore")
                    );
                    m_currentBlock->addInstruction(gep);
                    m_currentBlock->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
                }
            }
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "Ok" && node.getArgs().size() == 1) {
        node.getArgs()[0]->accept(*this);
        m_lastExprValue = emitCreateTaggedUnion("Result", 0, m_lastExprValue);
        m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        return;
    }
    if (calleeName == "Err" && node.getArgs().size() == 1) {
        node.getArgs()[0]->accept(*this);
        m_lastExprValue = emitCreateTaggedUnion("Result", 1, m_lastExprValue);
        m_lastExprType = ir::IRType::getStruct("struct.Result", {IRType::getInt32(), IRType::getInt64()});
        return;
    }
    if (calleeName == "Some" && node.getArgs().size() == 1) {
        node.getArgs()[0]->accept(*this);
        m_lastExprValue = emitCreateTaggedUnion("Option", 0, m_lastExprValue);
        m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
        return;
    }
    if (calleeName == "None" && node.getArgs().empty()) {
        m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
        m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
        return;
    }

    // Built-in standard functions: println, print, assert, panic
    if (calleeName == "println") {
        if (node.getArgs().empty()) {
            auto emptyStr = m_module->getOrCreateStringLiteral("");
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{emptyStr});
            m_currentBlock->addInstruction(call);
        } else {
            node.getArgs()[0]->accept(*this);
            IRValuePtr argVal = m_lastExprValue;
            IRTypePtr argType = m_lastExprType;

            auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
            if (argType && argType->isPointer()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isFloat()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_float", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isDouble()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_double", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isInt1()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_bool", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isChar()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_char", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isUnsignedInteger()) {
                if (!argType->isInt64()) {
                    std::string zextName = m_currentFunction ? m_currentFunction->nextTempName("zext") : "%zext";
                    auto zext = std::make_shared<CastInst>(IROpcode::ZExt, argVal, IRType::getInt64(), zextName);
                    if (m_currentBlock) m_currentBlock->addInstruction(zext);
                    argVal = zext;
                }
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_uint", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else {
                argVal = emitCastIfNeeded(argVal, IRType::getInt64());
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_int", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            }
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "print") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            IRValuePtr argVal = m_lastExprValue;
            IRTypePtr argType = m_lastExprType;

            auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
            if (argType && argType->isPointer()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_string", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isFloat()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_float", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isDouble()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_double", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isInt1()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_bool", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isChar()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_char", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (semaType && semaType->isUnsignedInteger()) {
                if (!argType->isInt64()) {
                    std::string zextName = m_currentFunction ? m_currentFunction->nextTempName("zext") : "%zext";
                    auto zext = std::make_shared<CastInst>(IROpcode::ZExt, argVal, IRType::getInt64(), zextName);
                    if (m_currentBlock) m_currentBlock->addInstruction(zext);
                    argVal = zext;
                }
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_uint", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else {
                argVal = emitCastIfNeeded(argVal, IRType::getInt64());
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_int", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            }
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "readLine" || calleeName == "readln") {
        std::string resName = m_currentFunction ? m_currentFunction->nextTempName("readline") : "%readline";
        auto call = std::make_shared<CallInst>(
            IRType::getPointer(IRType::getInt8()),
            "sp_rt_read_line",
            std::vector<IRValuePtr>{},
            resName
        );
        m_currentBlock->addInstruction(call);
        m_lastExprValue = call;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }

    if (calleeName == "eprintln") {
        if (node.getArgs().empty()) {
            auto emptyStr = m_module->getOrCreateStringLiteral("");
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{emptyStr});
            m_currentBlock->addInstruction(call);
        } else {
            node.getArgs()[0]->accept(*this);
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprintln_string", std::vector<IRValuePtr>{m_lastExprValue});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "eprint") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eprint_string", std::vector<IRValuePtr>{m_lastExprValue});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "flush") {
        auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_flush", std::vector<IRValuePtr>{});
        m_currentBlock->addInstruction(call);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "eflush") {
        auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_eflush", std::vector<IRValuePtr>{});
        m_currentBlock->addInstruction(call);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "assert") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            IRValuePtr condVal = m_lastExprValue;
            condVal = emitCastIfNeeded(condVal, IRType::getInt1());

            IRValuePtr msgVal = nullptr;
            if (node.getArgs().size() >= 2) {
                node.getArgs()[1]->accept(*this);
                msgVal = m_lastExprValue;
            } else {
                msgVal = m_module->getOrCreateStringLiteral("assertion failed");
            }

            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_assert", std::vector<IRValuePtr>{condVal, msgVal});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "panic") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            IRValuePtr msgVal = m_lastExprValue;
            auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_panic", std::vector<IRValuePtr>{msgVal});
            m_currentBlock->addInstruction(call);
        }
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "sqrt" || calleeName == "sin" || calleeName == "cos" || calleeName == "tan" ||
        calleeName == "asin" || calleeName == "acos" || calleeName == "atan" ||
        calleeName == "floor" || calleeName == "ceil" || calleeName == "round" ||
        calleeName == "log" || calleeName == "log10" || calleeName == "log2" || calleeName == "exp") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            auto val = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
            std::string fn = "sp_rt_math_" + calleeName;
            std::string reg = m_currentFunction->nextTempName("math_res");
            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{val}, reg);
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getDouble();
            return;
        }
    }

    if (calleeName == "pow" || calleeName == "atan2") {
        if (node.getArgs().size() >= 2) {
            node.getArgs()[0]->accept(*this);
            auto v1 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
            node.getArgs()[1]->accept(*this);
            auto v2 = emitCastIfNeeded(m_lastExprValue, IRType::getDouble());
            std::string fn = "sp_rt_math_" + calleeName;
            std::string reg = m_currentFunction->nextTempName("math_res");
            auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getDouble();
            return;
        }
    }

    if (calleeName == "abs") {
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            auto val = m_lastExprValue;
            auto type = m_lastExprType;
            auto semaType = m_sema.getNodeType(node.getArgs()[0].get());
            bool isFlt = (type && type->isFloatingPoint()) || (semaType && (semaType->isFloat() || semaType->isDouble()));
            if (isFlt) {
                val = emitCastIfNeeded(val, IRType::getDouble());
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getDouble(), "sp_rt_math_abs_double", std::vector<IRValuePtr>{val}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getDouble();
            } else {
                val = emitCastIfNeeded(val, IRType::getInt64());
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_math_abs_int", std::vector<IRValuePtr>{val}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
            }
            return;
        }
    }

    if (calleeName == "min" || calleeName == "max") {
        if (node.getArgs().size() >= 2) {
            node.getArgs()[0]->accept(*this);
            auto v1 = m_lastExprValue;
            auto t1 = m_lastExprType;
            auto semaT1 = m_sema.getNodeType(node.getArgs()[0].get());
            node.getArgs()[1]->accept(*this);
            auto v2 = m_lastExprValue;
            auto t2 = m_lastExprType;
            auto semaT2 = m_sema.getNodeType(node.getArgs()[1].get());
            bool isFlt = (t1 && t1->isFloatingPoint()) || (t2 && t2->isFloatingPoint()) ||
                         (semaT1 && (semaT1->isFloat() || semaT1->isDouble())) ||
                         (semaT2 && (semaT2->isFloat() || semaT2->isDouble()));
            if (isFlt) {
                v1 = emitCastIfNeeded(v1, IRType::getDouble());
                v2 = emitCastIfNeeded(v2, IRType::getDouble());
                std::string fn = (calleeName == "min") ? "sp_rt_math_min_double" : "sp_rt_math_max_double";
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getDouble(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getDouble();
            } else {
                v1 = emitCastIfNeeded(v1, IRType::getInt64());
                v2 = emitCastIfNeeded(v2, IRType::getInt64());
                std::string fn = (calleeName == "min") ? "sp_rt_math_min_int" : "sp_rt_math_max_int";
                std::string reg = m_currentFunction->nextTempName("math_res");
                auto callInst = std::make_shared<CallInst>(IRType::getInt64(), fn, std::vector<IRValuePtr>{v1, v2}, reg);
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
            }
            return;
        }
    }

    if (calleeName == "readText" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_fs_read_text", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if ((calleeName == "writeText" || calleeName == "appendText") && node.getArgs().size() >= 2) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        node.getArgs()[1]->accept(*this);
        auto c = m_lastExprValue;
        std::string fn = (calleeName == "writeText") ? "sp_rt_fs_write_text" : "sp_rt_fs_append_text";
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), fn, std::vector<IRValuePtr>{p, c}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "exists" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_exists", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if ((calleeName == "delete" || calleeName == "deleteFile") && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_delete", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "createDir" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_create_dir", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "listDir" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_fs_list_dir", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer();
        return;
    }
    if (calleeName == "isFile" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_file", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "isDir" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto p = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("fs_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_fs_is_dir", std::vector<IRValuePtr>{p}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }

    if (calleeName == "now") {
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ms", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "nowSeconds") {
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_secs", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "nowNanos") {
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_now_ns", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if ((calleeName == "sleep" || calleeName == "sleepSeconds") && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        std::string fn = (calleeName == "sleep") ? "sp_rt_time_sleep_ms" : "sp_rt_time_sleep_secs";
        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), fn, std::vector<IRValuePtr>{ms});
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }
    if (calleeName == "formatTime" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto ms = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_time_format_iso", std::vector<IRValuePtr>{ms}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "parseTime" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto s = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("time_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_time_parse_iso", std::vector<IRValuePtr>{s}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }

    if (calleeName == "getEnv" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto n = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_get_env", std::vector<IRValuePtr>{n}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "setEnv" && node.getArgs().size() >= 2) {
        node.getArgs()[0]->accept(*this);
        auto n = m_lastExprValue;
        node.getArgs()[1]->accept(*this);
        auto v = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt1(), "sp_rt_os_set_env", std::vector<IRValuePtr>{n, v}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt1();
        return;
    }
    if (calleeName == "platform") {
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_platform", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "arch") {
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_os_arch", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "args") {
        std::string reg = m_currentFunction->nextTempName("os_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(), "sp_rt_os_args", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer();
        return;
    }
    if (calleeName == "exit" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto code = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        auto callInst = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_os_exit", std::vector<IRValuePtr>{code});
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getVoid();
        return;
    }

    if (calleeName == "exec" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto cmd = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_exec", std::vector<IRValuePtr>{cmd}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "spawn" && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto cmd = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_spawn", std::vector<IRValuePtr>{cmd}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if ((calleeName == "runCommand" || calleeName == "execCapture") && !node.getArgs().empty()) {
        node.getArgs()[0]->accept(*this);
        auto cmd = m_lastExprValue;
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getPointer(IRType::getInt8()), "sp_rt_process_run", std::vector<IRValuePtr>{cmd}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getPointer(IRType::getInt8());
        return;
    }
    if (calleeName == "pid") {
        std::string reg = m_currentFunction->nextTempName("proc_res");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_process_pid", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }

    if (calleeName == "totalAllocatedBytes") {
        std::string reg = m_currentFunction->nextTempName("mem_bytes");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_mem_allocated_bytes", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }
    if (calleeName == "totalLiveObjects") {
        std::string reg = m_currentFunction->nextTempName("mem_objs");
        auto callInst = std::make_shared<CallInst>(IRType::getInt64(), "sp_rt_mem_live_objects", std::vector<IRValuePtr>{}, reg);
        m_currentBlock->addInstruction(callInst);
        m_lastExprValue = callInst;
        m_lastExprType = IRType::getInt64();
        return;
    }

    // Check if callee is a method on 'this'
    if (m_currentClass && m_currentClass->methodSlotMap.count(calleeName)) {
        int slotIdx = m_currentClass->methodSlotMap.at(calleeName);
        auto [thisAlloca, thisType] = lookupVariable("this");
        if (thisAlloca) {
            auto thisVal = std::make_shared<LoadInst>(IRType::getPointer(), thisAlloca, m_currentFunction->nextTempName("this"));
            m_currentBlock->addInstruction(thisVal);

            auto gepVTable = std::make_shared<GetElementPtrInst>(
                m_currentClass->structType,
                thisVal,
                std::vector<IRValuePtr>{
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                    std::make_shared<ConstantInt>(IRType::getInt32(), 0)
                },
                m_currentFunction->nextTempName("vt_slot")
            );
            m_currentBlock->addInstruction(gepVTable);

            auto loadVTable = std::make_shared<LoadInst>(
                IRType::getPointer(),
                gepVTable,
                m_currentFunction->nextTempName("vtable")
            );
            m_currentBlock->addInstruction(loadVTable);

            auto gepSlot = std::make_shared<GetElementPtrInst>(
                IRType::getPointer(),
                loadVTable,
                std::vector<IRValuePtr>{std::make_shared<ConstantInt>(IRType::getInt32(), slotIdx)},
                m_currentFunction->nextTempName("slot_ptr")
            );
            m_currentBlock->addInstruction(gepSlot);

            auto loadFnPtr = std::make_shared<LoadInst>(IRType::getPointer(), gepSlot, m_currentFunction->nextTempName("fn_ptr"));
            m_currentBlock->addInstruction(loadFnPtr);

            IRTypePtr returnType = IRType::getVoid();
            const auto& slotInfo = m_currentClass->vtableSlots[slotIdx];
            if (slotInfo.functionType && slotInfo.functionType->isFunction()) {
                auto fnType = std::dynamic_pointer_cast<IRFunctionType>(slotInfo.functionType);
                if (fnType) returnType = fnType->getReturnType();
            } else {
                auto* existing = m_module->getFunction(slotInfo.mangledName);
                if (existing) returnType = existing->getReturnType();
            }

            std::vector<IRValuePtr> loweredArgs;
            loweredArgs.push_back(thisVal);
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }

            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("vcall");
            auto callInst = std::make_shared<CallInst>(returnType, loadFnPtr, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        }
    }

    // Check if callee is a local variable holding a function pointer
    if (!calleeName.empty()) {
        auto [localAlloca, localType] = lookupVariable(calleeName);
        if (localAlloca != nullptr) {
            std::vector<IRValuePtr> loweredArgs;
            for (const auto& arg : node.getArgs()) {
                arg->accept(*this);
                loweredArgs.push_back(m_lastExprValue);
            }
            auto loadFn = std::make_shared<LoadInst>(
                IRType::getPointer(),
                localAlloca,
                m_currentFunction->nextTempName("fn_ptr")
            );
            m_currentBlock->addInstruction(loadFn);

            auto semaType = m_sema.getNodeType(&node);
            IRTypePtr returnType = semaType ? mapType(semaType.get()) : IRType::getInt64();
            std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("icall");

            auto callInst = std::make_shared<CallInst>(returnType, loadFn, loweredArgs, callReg);
            m_currentBlock->addInstruction(callInst);

            m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
            m_lastExprType = returnType;
            return;
        }
    }

    if (calleeName.empty()) {
        std::vector<IRValuePtr> loweredArgs;
        for (const auto& arg : node.getArgs()) {
            arg->accept(*this);
            loweredArgs.push_back(m_lastExprValue);
        }
        node.getCallee().accept(*this);
        IRValuePtr calleeVal = m_lastExprValue;

        auto semaType = m_sema.getNodeType(&node);
        IRTypePtr returnType = semaType ? mapType(semaType.get()) : IRType::getInt64();
        std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("icall");

        auto callInst = std::make_shared<CallInst>(returnType, calleeVal, loweredArgs, callReg);
        m_currentBlock->addInstruction(callInst);

        m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
        m_lastExprType = returnType;
        return;
    }

    // General user function call
    std::vector<IRValuePtr> loweredArgs;
    auto* targetFunc = m_module->getFunction(calleeName);
    for (size_t i = 0; i < node.getArgs().size(); ++i) {
        node.getArgs()[i]->accept(*this);
        IRValuePtr argVal = m_lastExprValue;
        if (targetFunc && i < targetFunc->getParams().size()) {
            auto expectedType = targetFunc->getParams()[i]->getType();
            if (expectedType && expectedType->isStruct() && argVal && argVal->getType()->isPointer()) {
                auto loadInst = std::make_shared<LoadInst>(expectedType, argVal, m_currentFunction->nextTempName("tu_arg"));
                m_currentBlock->addInstruction(loadInst);
                argVal = loadInst;
            }
        }
        loweredArgs.push_back(argVal);
    }
    if (m_astFunctions.count(calleeName)) {
        const auto* fnDecl = m_astFunctions.at(calleeName);
        const auto& params = fnDecl->getParams();
        for (size_t i = loweredArgs.size(); i < params.size(); ++i) {
            if (params[i].hasDefaultValue()) {
                params[i].getDefaultValue()->accept(*this);
                IRValuePtr defVal = m_lastExprValue;
                if (targetFunc && i < targetFunc->getParams().size()) {
                    defVal = emitCastIfNeeded(defVal, targetFunc->getParams()[i]->getType());
                }
                loweredArgs.push_back(defVal);
            }
        }
    }

    IRTypePtr returnType = IRType::getInt32();
    if (targetFunc) {
        returnType = targetFunc->getReturnType();
    }

    std::string callReg = returnType->isVoid() ? "" : m_currentFunction->nextTempName("c");
    auto callInst = std::make_shared<CallInst>(returnType, calleeName, loweredArgs, callReg);
    m_currentBlock->addInstruction(callInst);

    m_lastExprValue = returnType->isVoid() ? nullptr : callInst;
    m_lastExprType = returnType;
}

void IRBuilder::visit(const ast::TernaryExpr& node) {
    node.getCondition().accept(*this);
    IRValuePtr condVal = m_lastExprValue;
    condVal = emitCastIfNeeded(condVal, IRType::getInt1());

    auto thenBB = createBlock("tern_then");
    auto elseBB = createBlock("tern_else");
    auto mergeBB = createBlock("tern_merge");

    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(condVal, thenBB->getName(), elseBB->getName()));

    // Then
    setInsertPoint(thenBB);
    node.getThenExpr().accept(*this);
    IRValuePtr thenVal = m_lastExprValue;
    IRTypePtr resType = m_lastExprType ? m_lastExprType : IRType::getInt32();
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

    // Else
    setInsertPoint(elseBB);
    node.getElseExpr().accept(*this);
    IRValuePtr elseVal = m_lastExprValue;
    m_currentBlock->addInstruction(std::make_shared<BranchInst>(mergeBB->getName()));

    // Result alloca in entry block
    auto resAlloca = std::make_shared<AllocaInst>(resType, m_currentFunction->nextTempName("tern_res"));
    m_currentFunction->getEntryBlock()->addInstruction(resAlloca);

    // Store in both branches
    thenBB->addInstruction(std::make_shared<StoreInst>(thenVal, resAlloca));
    elseBB->addInstruction(std::make_shared<StoreInst>(elseVal, resAlloca));

    setInsertPoint(mergeBB);
    auto loadInst = std::make_shared<LoadInst>(resType, resAlloca, m_currentFunction->nextTempName("tern_val"));
    mergeBB->addInstruction(loadInst);

    m_lastExprValue = loadInst;
    m_lastExprType = resType;
}

IRValuePtr IRBuilder::emitMemberPointer(const ast::MemberExpr& node, IRTypePtr& outFieldType) {
    // 1. Evaluate the base object
    node.getObject().accept(*this);
    IRValuePtr objPtr = m_lastExprValue;
    if (!objPtr) return nullptr;

    // 2. Identify the class or struct of the object
    std::string className;
    if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getObject())) {
        if (ident->getName() == "this" && m_currentClass) {
            className = m_currentClass->name;
        } else {
            auto [varAlloca, varType] = lookupVariable(ident->getName());
            if (varType && varType->isStruct()) {
                auto st = std::dynamic_pointer_cast<IRStructType>(varType);
                if (st) {
                    className = st->getName();
                    if (className.rfind("struct.", 0) == 0) {
                        className = className.substr(7);
                    }
                }
            }
        }
    }

    if (className.empty()) {
        auto semaType = m_sema.getNodeType(&node.getObject());
        if (semaType && semaType->isNullable()) {
            const auto* nullT = static_cast<const sema::NullableType*>(semaType.get());
            if (nullT && nullT->getUnderlyingType()) {
                semaType = nullT->getUnderlyingType();
            }
        }
        if (semaType && (semaType->isClass() || semaType->isStruct() || semaType->isEnum())) {
            className = semaType->toString();
        }
    }

    if (className.empty() && m_currentClass) {
        className = m_currentClass->name;
    }

    if (className.empty() || !m_vtableBuilder) return nullptr;
    const auto* layout = m_vtableBuilder->getClassLayout(className);
    if (!layout) return nullptr;

    // 3. Find field index in class layout
    auto it = layout->fieldIndexMap.find(node.getMember());
    if (it == layout->fieldIndexMap.end()) return nullptr;

    uint32_t fieldIndex = it->second;
    size_t fVecIdx = layout->hasVTable ? (fieldIndex - 1) : fieldIndex;
    if (fVecIdx >= layout->fields.size()) return nullptr;

    outFieldType = layout->fields[fVecIdx].irType;

    // 4. Emit GEP
    auto gep = std::make_shared<GetElementPtrInst>(
        layout->structType,
        objPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), fieldIndex)
        },
        m_currentFunction->nextTempName("fld_ptr")
    );
    m_currentBlock->addInstruction(gep);
    return gep;
}

void IRBuilder::visit(const ast::MemberExpr& node) {
    // 1. Check for Enum variant access (e.g. Direction.North, HttpStatus.OK)
    std::string objIdentName;
    if (const auto* ident = dynamic_cast<const ast::IdentifierExpr*>(&node.getObject())) {
        objIdentName = ident->getName();
    }
    if (!objIdentName.empty()) {
        auto enumUserType = m_sema.getTypeContext().findUserType(objIdentName);
        if (enumUserType && enumUserType->isEnum()) {
            auto enumType = std::static_pointer_cast<sema::EnumType>(enumUserType);
            if (enumType->isTaggedUnion()) {
                if (objIdentName == "Option" && node.getMember() == "None") {
                    m_lastExprValue = emitCreateTaggedUnion("Option", 1, nullptr);
                    m_lastExprType = ir::IRType::getStruct("struct.Option", {IRType::getInt32(), IRType::getInt64()});
                    return;
                }
            } else {
                int64_t val = enumType->getVariantValue(node.getMember());
                m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
                m_lastExprType = IRType::getInt32();
                return;
            }
        }
        if (objIdentName == "Math") {
            if (node.getMember() == "PI") {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 3.141592653589793);
                m_lastExprType = IRType::getDouble();
                return;
            }
            if (node.getMember() == "E") {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 2.718281828459045);
                m_lastExprType = IRType::getDouble();
                return;
            }
            if (node.getMember() == "TAU") {
                m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), 6.283185307179586);
                m_lastExprType = IRType::getDouble();
                return;
            }
        }
    }
    auto semaType = m_sema.getNodeType(&node.getObject());
    if (semaType && semaType->isEnum()) {
        auto enumType = std::static_pointer_cast<sema::EnumType>(semaType);
        if (!enumType->isTaggedUnion()) {
            int64_t val = enumType->getVariantValue(node.getMember());
            m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
            m_lastExprType = IRType::getInt32();
            return;
        }
    }

    // 2. Built-in collection property access (.length / .size)
    if (semaType && (semaType->isArray() || semaType->isString() || semaType->isMap() ||
                    (semaType->isClass() && (semaType->toString().rfind("List", 0) == 0 || semaType->toString().rfind("Map", 0) == 0)))) {
        if (node.getMember() == "length" || node.getMember() == "size") {
            node.getObject().accept(*this);
            auto objVal = m_lastExprValue;
            std::string fnName = semaType->isString() ? "sp_rt_str_len"
                                 : semaType->isArray() ? "sp_rt_array_length"
                                 : (semaType->isMap() || semaType->toString().rfind("Map", 0) == 0) ? "sp_rt_map_size"
                                 : "sp_rt_list_size";
            std::string lenReg = m_currentFunction->nextTempName("len");
            auto callInst = std::make_shared<CallInst>(
                IRType::getInt64(),
                fnName,
                std::vector<IRValuePtr>{objVal},
                lenReg
            );
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getInt64();
            return;
        }
    }

    if (semaType && semaType->isClass() && semaType->toString() == "Duration") {
        if (node.getMember() == "millis") {
            node.getObject().accept(*this);
            auto objVal = m_lastExprValue;
            std::string reg = m_currentFunction->nextTempName("dur_ms");
            auto callInst = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_duration_to_millis",
                std::vector<IRValuePtr>{objVal},
                reg
            );
            m_currentBlock->addInstruction(callInst);
            m_lastExprValue = callInst;
            m_lastExprType = IRType::getInt64();
            return;
        }
    }

    // 3. Class/Struct member field access
    IRTypePtr fieldType = nullptr;
    auto fieldPtr = emitMemberPointer(node, fieldType);
    if (fieldPtr && fieldType) {
        auto loadVal = std::make_shared<LoadInst>(
            fieldType,
            fieldPtr,
            m_currentFunction->nextTempName("mem_val")
        );
        m_currentBlock->addInstruction(loadVal);
        m_lastExprValue = loadVal;
        m_lastExprType = fieldType;
    } else {
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
    }
}

void IRBuilder::visit(const ast::IndexExpr& node) {
    node.getObject().accept(*this);
    IRValuePtr objVal = m_lastExprValue;

    node.getIndex().accept(*this);
    IRValuePtr idxVal = m_lastExprValue;

    if (!objVal || !idxVal) {
        m_lastExprValue = nullptr;
        m_lastExprType = nullptr;
        return;
    }

    auto semaObjType = m_sema.getNodeType(&node.getObject());
    auto semaResultType = m_sema.getNodeType(&node);
    if (semaResultType && semaResultType->isNullable()) {
        const auto* nullT = static_cast<const sema::NullableType*>(semaResultType.get());
        if (nullT && nullT->getUnderlyingType()) {
            semaResultType = nullT->getUnderlyingType();
        }
    }
    IRTypePtr targetIRType = mapType(semaResultType.get());

    bool isMapType = semaObjType && (semaObjType->isMap() || (semaObjType->isClass() && semaObjType->toString().rfind("Map", 0) == 0));
    bool isListType = semaObjType && semaObjType->isClass() && semaObjType->toString().rfind("List", 0) == 0;
    bool isPtrType = semaObjType && semaObjType->isPointer();

    if (isPtrType) {
        auto semaPointee = static_cast<const sema::PointerType&>(*semaObjType).getPointeeType();
        IRTypePtr elemType = semaPointee ? mapType(semaPointee.get()) : IRType::getInt64();
        std::string gepName = m_currentFunction->nextTempName("ptr_gep");
        auto gep = std::make_shared<GetElementPtrInst>(
            elemType, objVal,
            std::vector<IRValuePtr>{idxVal},
            gepName
        );
        m_currentBlock->addInstruction(gep);
        auto loadInst = std::make_shared<LoadInst>(elemType, gep, m_currentFunction->nextTempName("ptr_elem"));
        m_currentBlock->addInstruction(loadInst);
        m_lastExprValue = loadInst;
        m_lastExprType = elemType;
        return;
    }

    if (isMapType) {
        if (idxVal->getType()->isPointer()) {
            std::string getReg = m_currentFunction->nextTempName("map_val");
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_map_get_str",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        } else {
            idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());
            std::string getReg = m_currentFunction->nextTempName("map_val");
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_map_get_i64",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        }
        return;
    }

    idxVal = emitCastIfNeeded(idxVal, IRType::getInt64());

    if (isListType) {
        if (targetIRType && targetIRType->isFloatingPoint()) {
            std::string getReg = m_currentFunction->nextTempName("list_elem");
            auto getCall = std::make_shared<CallInst>(
                IRType::getDouble(),
                "sp_rt_list_get_f64",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        } else {
            std::string getReg = m_currentFunction->nextTempName("list_elem");
            auto getCall = std::make_shared<CallInst>(
                IRType::getInt64(),
                "sp_rt_list_get_i64",
                std::vector<IRValuePtr>{objVal, idxVal},
                getReg
            );
            m_currentBlock->addInstruction(getCall);
            m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
            m_lastExprType = targetIRType;
        }
        return;
    }

    if (targetIRType && targetIRType->isFloatingPoint()) {
        std::string getReg = m_currentFunction->nextTempName("arr_elem");
        auto getCall = std::make_shared<CallInst>(
            IRType::getDouble(),
            "sp_rt_array_get_f64",
            std::vector<IRValuePtr>{objVal, idxVal},
            getReg
        );
        m_currentBlock->addInstruction(getCall);
        m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
        m_lastExprType = targetIRType;
    } else {
        std::string getReg = m_currentFunction->nextTempName("arr_elem");
        auto getCall = std::make_shared<CallInst>(
            IRType::getInt64(),
            "sp_rt_array_get_i64",
            std::vector<IRValuePtr>{objVal, idxVal},
            getReg
        );
        m_currentBlock->addInstruction(getCall);
        m_lastExprValue = emitCastIfNeeded(getCall, targetIRType);
        m_lastExprType = targetIRType;
    }
}

void IRBuilder::visit(const ast::ArrayExpr& node) {
    size_t count = node.getElements().size();
    auto countVal = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(count));
    auto elemSizeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 8);

    std::string arrReg = m_currentFunction->nextTempName("arr");
    auto allocCall = std::make_shared<CallInst>(
        IRType::getPointer(),
        "sp_rt_array_alloc",
        std::vector<IRValuePtr>{countVal, elemSizeVal},
        arrReg
    );
    m_currentBlock->addInstruction(allocCall);

    for (size_t i = 0; i < count; ++i) {
        node.getElements()[i]->accept(*this);
        IRValuePtr elemVal = m_lastExprValue;
        auto idxVal = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(i));

        if (m_lastExprType && m_lastExprType->isFloatingPoint()) {
            if (m_lastExprType->isFloat()) {
                elemVal = emitCastIfNeeded(elemVal, IRType::getDouble());
            }
            auto setCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_array_set_f64",
                std::vector<IRValuePtr>{allocCall, idxVal, elemVal}
            );
            m_currentBlock->addInstruction(setCall);
        } else {
            elemVal = emitCastIfNeeded(elemVal, IRType::getInt64());
            auto setCall = std::make_shared<CallInst>(
                IRType::getVoid(),
                "sp_rt_array_set_i64",
                std::vector<IRValuePtr>{allocCall, idxVal, elemVal}
            );
            m_currentBlock->addInstruction(setCall);
        }
    }

    m_lastExprValue = allocCall;
    m_lastExprType = IRType::getPointer();
}
void IRBuilder::visit(const ast::MapExpr& node) { (void)node; }
void IRBuilder::visit(const ast::LambdaExpr& node) {
    std::string lambdaName = "__sp_lambda_" + std::to_string(m_lambdaCounter++);

    // Determine return type
    auto semaType = m_sema.getNodeType(&node);
    IRTypePtr retIRType = IRType::getInt64();
    if (semaType && semaType->isFunction()) {
        auto fnType = std::dynamic_pointer_cast<sema::FunctionType>(semaType);
        if (fnType && fnType->getReturnType()) {
            retIRType = mapType(fnType->getReturnType().get());
        }
    } else if (!node.getReturnType().empty()) {
        retIRType = mapType(node.getReturnType());
    }

    // Determine parameter types
    std::vector<std::shared_ptr<ParameterValue>> lambdaParams;
    for (size_t i = 0; i < node.getParams().size(); ++i) {
        const auto& p = node.getParams()[i];
        IRTypePtr pType = IRType::getInt64();
        if (!p.type.empty()) {
            pType = mapType(p.type);
        } else if (semaType && semaType->isFunction()) {
            auto fnType = std::dynamic_pointer_cast<sema::FunctionType>(semaType);
            if (fnType && i < fnType->getParamTypes().size() && fnType->getParamTypes()[i]) {
                pType = mapType(fnType->getParamTypes()[i].get());
            }
        }
        lambdaParams.push_back(std::make_shared<ParameterValue>(pType, "%" + p.name, i));
    }

    // Save previous function context and scope stack
    auto prevFunc = m_currentFunction;
    auto prevBlock = m_currentBlock;
    auto prevScopes = std::move(m_scopes);
    m_scopes.clear();

    auto lambdaFunc = m_module->createFunction(lambdaName, retIRType, lambdaParams);
    m_currentFunction = lambdaFunc;
    auto entryBB = lambdaFunc->createBasicBlock("entry");
    setInsertPoint(entryBB);
    pushScope();

    // Stack allocas for parameters
    for (size_t i = 0; i < node.getParams().size(); ++i) {
        const auto& p = node.getParams()[i];
        const auto& paramVal = lambdaParams[i];
        auto allocaInst = std::make_shared<AllocaInst>(paramVal->getType(), "%p_" + p.name);
        entryBB->addInstruction(allocaInst);
        entryBB->addInstruction(std::make_shared<StoreInst>(paramVal, allocaInst));
        declareVariable(p.name, allocaInst, paramVal->getType());
    }

    // Lower body
    if (const auto* exprBody = dynamic_cast<const ast::ExpressionNode*>(&node.getBody())) {
        exprBody->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        if (retVal && !retIRType->isVoid()) {
            retVal = emitCastIfNeeded(retVal, retIRType);
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
        } else {
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
        }
    } else if (const auto* blockBody = dynamic_cast<const ast::BlockStmt*>(&node.getBody())) {
        for (const auto& stmt : blockBody->getStatements()) {
            stmt->accept(*this);
            if (m_currentBlock->hasTerminator()) break;
        }
        if (!m_currentBlock->hasTerminator()) {
            if (retIRType->isVoid()) {
                m_currentBlock->addInstruction(std::make_shared<ReturnInst>());
            } else {
                m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                    std::make_shared<ConstantInt>(retIRType, 0)));
            }
        }
    }

    popScope();
    m_scopes = std::move(prevScopes);
    m_currentFunction = prevFunc;
    m_currentBlock = prevBlock;

    auto fnPtr = std::make_shared<GlobalVariable>(IRType::getInt8(), "@" + lambdaName);
    m_lastExprValue = fnPtr;
    m_lastExprType = IRType::getPointer();
}
void IRBuilder::visit(const ast::NewExpr& node) {
    std::string typeName = node.getTypeName();
    if (typeName == "List") {
        auto elemSizeVal = std::make_shared<ConstantInt>(IRType::getInt64(), 8);
        auto initCapVal = std::make_shared<ConstantInt>(IRType::getInt64(), 16);
        std::string listReg = m_currentFunction->nextTempName("list");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_list_alloc",
            std::vector<IRValuePtr>{elemSizeVal, initCapVal},
            listReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Map") {
        std::string mapReg = m_currentFunction->nextTempName("map");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_map_alloc",
            std::vector<IRValuePtr>{},
            mapReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Set") {
        std::string setReg = m_currentFunction->nextTempName("set");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_set_alloc",
            std::vector<IRValuePtr>{},
            setReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Stack") {
        std::string reg = m_currentFunction->nextTempName("stack");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_stack_alloc",
            std::vector<IRValuePtr>{},
            reg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Queue") {
        std::string reg = m_currentFunction->nextTempName("queue");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_queue_alloc",
            std::vector<IRValuePtr>{},
            reg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "PriorityQueue") {
        std::string reg = m_currentFunction->nextTempName("pq");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_pq_alloc",
            std::vector<IRValuePtr>{},
            reg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Duration") {
        std::string durReg = m_currentFunction->nextTempName("dur");
        auto zeroVal = std::make_shared<ConstantInt>(IRType::getInt64(), 0);
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_duration_from_millis",
            std::vector<IRValuePtr>{zeroVal},
            durReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (typeName == "Arena") {
        IRValuePtr capVal = nullptr;
        if (!node.getArgs().empty()) {
            node.getArgs()[0]->accept(*this);
            capVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
        } else {
            capVal = std::make_shared<ConstantInt>(IRType::getInt64(), 65536);
        }
        std::string arenaReg = m_currentFunction->nextTempName("arena");
        auto allocCall = std::make_shared<CallInst>(
            IRType::getPointer(),
            "sp_rt_arena_create",
            std::vector<IRValuePtr>{capVal},
            arenaReg
        );
        m_currentBlock->addInstruction(allocCall);
        m_lastExprValue = allocCall;
        m_lastExprType = IRType::getPointer();
        return;
    }

    if (node.isGeneric()) {
        typeName = node.getTypeName();
        for (const auto& arg : node.getTypeArgs()) {
            typeName += "_" + arg;
        }
    }

    const auto* layout = m_vtableBuilder ? m_vtableBuilder->getClassLayout(typeName) : nullptr;
    if (!layout) {
        m_lastExprValue = std::make_shared<ConstantNull>();
        m_lastExprType = IRType::getPointer();
        return;
    }

    // 1. Allocate object on heap via sp_rt_arc_alloc
    uint64_t size = layout->objectSize > 0 ? layout->objectSize : 8;
    auto sizeVal = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(size));

    std::string effectiveName = layout ? layout->name : typeName;
    std::string dtorName = effectiveName + "_destructor";
    IRValuePtr dtorPtr = nullptr;
    if (m_module->getFunction(dtorName)) {
        dtorPtr = std::make_shared<GlobalVariable>(IRType::getInt8(), "@" + dtorName);
    } else {
        dtorPtr = std::make_shared<ConstantNull>();
    }

    std::string rawPtrReg = m_currentFunction->nextTempName("obj_arc");
    auto allocCall = std::make_shared<CallInst>(
        IRType::getPointer(),
        "sp_rt_arc_alloc",
        std::vector<IRValuePtr>{sizeVal, dtorPtr},
        rawPtrReg
    );
    m_currentBlock->addInstruction(allocCall);

    // 2. Set vtable pointer in slot 0
    if (layout->hasVTable) {
        auto vtableGlobal = std::make_shared<GlobalVariable>(IRType::getInt8(), layout->vtableName);
        auto gep = std::make_shared<GetElementPtrInst>(
            layout->structType,
            allocCall,
            std::vector<IRValuePtr>{
                std::make_shared<ConstantInt>(IRType::getInt32(), 0),
                std::make_shared<ConstantInt>(IRType::getInt32(), 0)
            },
            m_currentFunction->nextTempName("vptr")
        );
        m_currentBlock->addInstruction(gep);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(vtableGlobal, gep));
    }

    // 3. Call constructor @ClassName_ClassName(ptr %raw, args...)
    std::string ctorName = effectiveName + "_" + effectiveName;
    auto* ctorFunc = m_module->getFunction(ctorName);

    std::vector<IRValuePtr> loweredArgs;
    loweredArgs.push_back(allocCall);
    for (size_t i = 0; i < node.getArgs().size(); ++i) {
        node.getArgs()[i]->accept(*this);
        IRValuePtr argVal = m_lastExprValue;
        if (ctorFunc && i + 1 < ctorFunc->getParams().size()) {
            argVal = emitCastIfNeeded(argVal, ctorFunc->getParams()[i + 1]->getType());
        }
        loweredArgs.push_back(argVal);
    }

    auto ctorCall = std::make_shared<CallInst>(IRType::getVoid(), ctorName, loweredArgs);
    m_currentBlock->addInstruction(ctorCall);

    m_lastExprValue = allocCall;
    m_lastExprType = IRType::getPointer();
}

void IRBuilder::visit(const ast::TryExpr& node) {
    node.getExpr().accept(*this);
    IRValuePtr operandVal = m_lastExprValue;
    if (!operandVal) {
        m_lastExprValue = nullptr;
        m_lastExprType = IRType::getInt32();
        return;
    }

    auto semaOpType = m_sema.getNodeType(&node.getExpr());
    std::string opTypeName = semaOpType ? semaOpType->toString() : "";
    std::string tuBaseName = (opTypeName.rfind("Option", 0) == 0) ? "Option" : "Result";
    auto structType = ir::IRType::getStruct("struct." + tuBaseName, {IRType::getInt32(), IRType::getInt64()});

    IRValuePtr objPtr = operandVal;
    if (operandVal->getType()->isStruct()) {
        auto tempAlloca = std::make_shared<AllocaInst>(operandVal->getType(), m_currentFunction->nextTempName("try_tmp"));
        m_currentFunction->getEntryBlock()->addInstruction(tempAlloca);
        m_currentBlock->addInstruction(std::make_shared<StoreInst>(operandVal, tempAlloca));
        objPtr = tempAlloca;
    }

    // 1. Load tag (field 0)
    auto gepTag = std::make_shared<GetElementPtrInst>(
        structType,
        objPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 0)
        },
        m_currentFunction->nextTempName("try_tag_ptr")
    );
    m_currentBlock->addInstruction(gepTag);
    auto loadTag = std::make_shared<LoadInst>(IRType::getInt32(), gepTag, m_currentFunction->nextTempName("try_tag"));
    m_currentBlock->addInstruction(loadTag);

    // 2. Load payload (field 1)
    auto gepPayload = std::make_shared<GetElementPtrInst>(
        structType,
        objPtr,
        std::vector<IRValuePtr>{
            std::make_shared<ConstantInt>(IRType::getInt32(), 0),
            std::make_shared<ConstantInt>(IRType::getInt32(), 1)
        },
        m_currentFunction->nextTempName("try_payload_ptr")
    );
    m_currentBlock->addInstruction(gepPayload);
    auto loadPayload = std::make_shared<LoadInst>(IRType::getInt64(), gepPayload, m_currentFunction->nextTempName("try_payload"));
    m_currentBlock->addInstruction(loadPayload);

    // 3. Compare tag != 0 (error / none)
    auto cmpErr = std::make_shared<CompareInst>(IROpcode::ICmpNe, loadTag,
        std::make_shared<ConstantInt>(IRType::getInt32(), 0),
        m_currentFunction->nextTempName("is_err"));
    m_currentBlock->addInstruction(cmpErr);

    auto errBB = createBlock("try_err");
    auto okBB = createBlock("try_ok");
    m_currentBlock->addInstruction(std::make_shared<CondBranchInst>(cmpErr, errBB->getName(), okBB->getName()));

    // 4. Error block: early return!
    setInsertPoint(errBB);
    emitAllFunctionDrops();
    IRTypePtr retType = m_currentFunction->getReturnType();
    if (retType->isStruct()) {
        auto earlyRetVal = std::make_shared<LoadInst>(
            retType,
            objPtr,
            m_currentFunction->nextTempName("early_ret")
        );
        errBB->addInstruction(earlyRetVal);
        errBB->addInstruction(std::make_shared<ReturnInst>(earlyRetVal));
    } else if (retType->isPointer()) {
        errBB->addInstruction(std::make_shared<ReturnInst>(objPtr));
    } else {
        errBB->addInstruction(std::make_shared<ReturnInst>(makeDefaultValue(retType)));
    }

    // 5. Ok block: unwrapped payload
    setInsertPoint(okBB);
    auto semaNodeType = m_sema.getNodeType(&node);
    IRTypePtr targetType = semaNodeType ? mapType(semaNodeType.get()) : IRType::getInt64();
    auto unwrapped = emitCastIfNeeded(loadPayload, targetType);

    m_lastExprValue = unwrapped;
    m_lastExprType = targetType;
}

} // namespace sp::ir
