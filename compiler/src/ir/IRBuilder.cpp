#include "sp/ir/IRBuilder.h"
#include "sp/codegen/VTableBuilder.h"
#include "sp/codegen/ClassGenerator.h"
#include <iostream>
#include <stdexcept>

namespace sp::ir {

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
}

void IRBuilder::popScope() {
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
        auto cast = std::make_shared<CastInst>(IROpcode::SIToFP, val, targetType, tempName);
        if (m_currentBlock) m_currentBlock->addInstruction(cast);
        return cast;
    }

    if (srcType->isFloatingPoint() && targetType->isInteger()) {
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

    return val;
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
        case sema::TypeKind::Enum: return IRType::getInt32();
        case sema::TypeKind::Array:
        case sema::TypeKind::Map:
        case sema::TypeKind::Class:
        case sema::TypeKind::Interface:
        case sema::TypeKind::Struct:
        case sema::TypeKind::Nullable: return IRType::getPointer();
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

    auto userType = m_sema.getTypeContext().findUserType(typeName);
    if (userType && userType->isEnum()) {
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

    // Index generic classes and functions for monomorphization
    for (const auto& decl : node.getDeclarations()) {
        if (const auto* cls = dynamic_cast<const ast::ClassDecl*>(decl.get())) {
            if (cls->isGeneric()) {
                m_genericClasses[cls->getName()] = cls;
            }
        } else if (const auto* fn = dynamic_cast<const ast::FunctionDecl*>(decl.get())) {
            if (fn->isGeneric()) {
                m_genericFunctions[fn->getName()] = fn;
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
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(returnType, 0)));
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

    if (initVal) {
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

void IRBuilder::visit(const ast::ForInStmt& node) { (void)node; }
void IRBuilder::visit(const ast::SwitchStmt& node) { (void)node; }
void IRBuilder::visit(const ast::MatchStmt& node) { (void)node; }

void IRBuilder::visit(const ast::ReturnStmt& node) {
    if (node.hasValue()) {
        node.getValue()->accept(*this);
        IRValuePtr retVal = m_lastExprValue;
        retVal = emitCastIfNeeded(retVal, m_currentFunction->getReturnType());
        m_currentBlock->addInstruction(std::make_shared<ReturnInst>(retVal));
    } else {
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
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_breakStack.back()));
    }
}

void IRBuilder::visit(const ast::ContinueStmt& node) {
    (void)node;
    if (!m_continueStack.empty()) {
        m_currentBlock->addInstruction(std::make_shared<BranchInst>(m_continueStack.back()));
    }
}

void IRBuilder::visit(const ast::TryCatchStmt& node) { (void)node; }
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
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(std::make_shared<ConstantInt>(returnType, 0)));
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
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(returnType, 0)));
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
            m_currentBlock->addInstruction(std::make_shared<ReturnInst>(
                std::make_shared<ConstantInt>(returnType, 0)));
        }
    }

    popScope();
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
            try {
                val = std::stoll(node.getValue(), nullptr, 0);
            } catch (...) {
                val = 0;
            }
            m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
            m_lastExprType = IRType::getInt32();
            break;
        }
        case ast::LiteralKind::Float: {
            double val = 0.0;
            try {
                val = std::stod(node.getValue());
            } catch (...) {
                val = 0.0;
            }
            m_lastExprValue = std::make_shared<ConstantFloat>(IRType::getDouble(), val);
            m_lastExprType = IRType::getDouble();
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
        if (semaObjType && semaObjType->isMap()) {
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

    rhsVal = emitCastIfNeeded(rhsVal, valType);
    m_currentBlock->addInstruction(std::make_shared<StoreInst>(rhsVal, targetPtr));
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

        // 1b. obj.method(...)
        mem->getObject().accept(*this);
        IRValuePtr objVal = m_lastExprValue;
        if (!objVal) {
            m_lastExprValue = nullptr;
            m_lastExprType = IRType::getVoid();
            return;
        }

        auto semaObjType = m_sema.getNodeType(&mem->getObject());
        if (semaObjType && (semaObjType->isArray() || semaObjType->isMap() ||
                            (semaObjType->isClass() && (semaObjType->toString().rfind("List", 0) == 0 || semaObjType->toString().rfind("Map", 0) == 0)))) {
            if (methodName == "length" || methodName == "size") {
                std::string fnName = semaObjType->isArray() ? "sp_rt_array_length"
                                     : (semaObjType->isMap() || semaObjType->toString().rfind("Map", 0) == 0) ? "sp_rt_map_size"
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
            } else if (methodName == "push" || methodName == "add") {
                if (!node.getArgs().empty()) {
                    node.getArgs()[0]->accept(*this);
                    auto elemVal = emitCastIfNeeded(m_lastExprValue, IRType::getInt64());
                    auto callInst = std::make_shared<CallInst>(
                        IRType::getVoid(),
                        "sp_rt_list_push_i64",
                        std::vector<IRValuePtr>{objVal, elemVal}
                    );
                    m_currentBlock->addInstruction(callInst);
                    m_lastExprValue = nullptr;
                    m_lastExprType = IRType::getVoid();
                    return;
                }
            } else if (methodName == "pop") {
                std::string popReg = m_currentFunction->nextTempName("pop_val");
                auto callInst = std::make_shared<CallInst>(
                    IRType::getInt64(),
                    "sp_rt_list_pop_i64",
                    std::vector<IRValuePtr>{objVal},
                    popReg
                );
                m_currentBlock->addInstruction(callInst);
                m_lastExprValue = callInst;
                m_lastExprType = IRType::getInt64();
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
                std::string fnName = semaObjType->isArray() ? "sp_rt_array_length"
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

            if (argType && argType->isPointer()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_string", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isFloatingPoint()) {
                argVal = emitCastIfNeeded(argVal, IRType::getDouble());
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_double", std::vector<IRValuePtr>{argVal});
                m_currentBlock->addInstruction(call);
            } else if (argType && argType->isInt1()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_println_bool", std::vector<IRValuePtr>{argVal});
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

            if (argType && argType->isPointer()) {
                auto call = std::make_shared<CallInst>(IRType::getVoid(), "sp_rt_print_string", std::vector<IRValuePtr>{argVal});
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

    if (calleeName == "assert") {
        if (node.getArgs().size() >= 2) {
            node.getArgs()[0]->accept(*this);
            IRValuePtr condVal = m_lastExprValue;
            condVal = emitCastIfNeeded(condVal, IRType::getInt1());

            node.getArgs()[1]->accept(*this);
            IRValuePtr msgVal = m_lastExprValue;

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

    // General user function call
    std::vector<IRValuePtr> loweredArgs;
    for (const auto& arg : node.getArgs()) {
        arg->accept(*this);
        loweredArgs.push_back(m_lastExprValue);
    }

    IRTypePtr returnType = IRType::getInt32();
    auto* targetFunc = m_module->getFunction(calleeName);
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
        if (semaType && (semaType->isClass() || semaType->isStruct())) {
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
            int64_t val = enumType->getVariantValue(node.getMember());
            m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
            m_lastExprType = IRType::getInt32();
            return;
        }
    }
    auto semaType = m_sema.getNodeType(&node.getObject());
    if (semaType && semaType->isEnum()) {
        auto enumType = std::static_pointer_cast<sema::EnumType>(semaType);
        int64_t val = enumType->getVariantValue(node.getMember());
        m_lastExprValue = std::make_shared<ConstantInt>(IRType::getInt32(), val);
        m_lastExprType = IRType::getInt32();
        return;
    }

    // 2. Built-in collection property access (.length / .size)
    if (semaType && (semaType->isArray() || semaType->isString() || semaType->isMap() ||
                    (semaType->isClass() && (semaType->toString().rfind("List", 0) == 0 || semaType->toString().rfind("Map", 0) == 0)))) {
        if (node.getMember() == "length" || node.getMember() == "size") {
            node.getObject().accept(*this);
            auto objVal = m_lastExprValue;
            std::string fnName = (semaType->isArray() || semaType->isString()) ? "sp_rt_array_length"
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
    IRTypePtr targetIRType = mapType(semaResultType.get());

    if (semaObjType && semaObjType->isMap()) {
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
void IRBuilder::visit(const ast::LambdaExpr& node) { (void)node; }
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

    // 1. Allocate object on heap via sp_rt_alloc
    uint64_t size = layout->objectSize > 0 ? layout->objectSize : 8;
    auto sizeVal = std::make_shared<ConstantInt>(IRType::getInt64(), static_cast<int64_t>(size));
    std::string rawPtrReg = m_currentFunction->nextTempName("obj_raw");
    auto allocCall = std::make_shared<CallInst>(
        IRType::getPointer(),
        "sp_rt_alloc",
        std::vector<IRValuePtr>{sizeVal},
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
    std::string effectiveName = layout ? layout->name : typeName;
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

} // namespace sp::ir
