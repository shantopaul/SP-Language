#include "sp/sema/TypeChecker.h"

namespace sp::sema {

void TypeChecker::check(const ast::Program& program) {
    visit(program);
}

TypePtr TypeChecker::checkExpr(const ast::ExpressionNode& expr) {
    expr.accept(*this);
    if (!m_currentExprType) {
        m_currentExprType = m_typeCtx.getError();
    }
    m_nodeTypes[&expr] = m_currentExprType;
    return m_currentExprType;
}

void TypeChecker::checkStmt(const ast::StatementNode& stmt) {
    stmt.accept(*this);
}

void TypeChecker::visit(const ast::LiteralExpr& node) {
    switch (node.getLiteralKind()) {
        case ast::LiteralKind::Integer:
            m_currentExprType = m_typeCtx.getInt();
            break;
        case ast::LiteralKind::Float: {
            const std::string& val = node.getValue();
            if (!val.empty() && (val.back() == 'f' || val.back() == 'F')) {
                m_currentExprType = m_typeCtx.getFloat();
            } else {
                m_currentExprType = m_typeCtx.getDouble();
            }
            break;
        }
        case ast::LiteralKind::Boolean:
            m_currentExprType = m_typeCtx.getBool();
            break;
        case ast::LiteralKind::String:
            m_currentExprType = m_typeCtx.getString();
            break;
        case ast::LiteralKind::Char:
            m_currentExprType = m_typeCtx.getChar();
            break;
        case ast::LiteralKind::Null:
            m_currentExprType = m_typeCtx.getNull();
            break;
        default:
            m_currentExprType = m_typeCtx.getError();
            break;
    }
}

void TypeChecker::visit(const ast::IdentifierExpr& node) {
    const std::string& name = node.getName();

    if (name == "this") {
        if (!m_currentEnclosingClass) {
            m_diagnostics.reportError("E1001", "'this' cannot be used outside of an instance method", node.getLocation());
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_currentEnclosingClass;
        return;
    }

    if (name == "super") {
        if (!m_currentEnclosingClass || !m_currentEnclosingClass->getSuperclass()) {
            m_diagnostics.reportError("E1001", "'super' cannot be used outside of a subclass", node.getLocation());
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_currentEnclosingClass->getSuperclass();
        return;
    }

    auto sym = m_symbolTable.lookup(name);
    if (sym) {
        m_currentExprType = sym->getType();
        return;
    }

    // Check if it's a field in enclosing class or inherited from superclass
    if (m_currentEnclosingClass) {
        auto fieldOpt = m_currentEnclosingClass->findField(name);
        if (fieldOpt.has_value()) {
            m_currentExprType = fieldOpt->type;
            return;
        }
    }

    // Check if it's a registered user type (class, enum, etc.)
    auto userType = m_typeCtx.findUserType(name);
    if (userType) {
        m_currentExprType = userType;
        return;
    }

    auto primType = m_typeCtx.resolveType(name);
    if (primType && !primType->isError()) {
        m_currentExprType = primType;
        return;
    }

    m_diagnostics.reportError("E1002", "Undefined variable or symbol '" + name + "'", node.getLocation(),
                              "Ensure the variable is declared before use");
    m_currentExprType = m_typeCtx.getError();
}

void TypeChecker::visit(const ast::BinaryExpr& node) {
    auto leftType = checkExpr(node.getLeft());
    auto rightType = checkExpr(node.getRight());

    if (leftType->isError() || rightType->isError()) {
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    lexer::TokenType op = node.getOp();

    // String concatenation: if either operand is string and op is '+', result is string
    if (op == lexer::TokenType::Plus) {
        if (leftType->isString() || rightType->isString()) {
            m_currentExprType = m_typeCtx.getString();
            return;
        }
    }

    // Range operator: ..
    if (op == lexer::TokenType::DotDot) {
        if (!leftType->isInteger() || !rightType->isInteger()) {
            m_diagnostics.reportError("E1001", "Range operator '..' requires integer operands, got '" +
                                      leftType->toString() + "' and '" + rightType->toString() + "'", node.getLocation());
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_typeCtx.getArrayType(m_typeCtx.getInt());
        return;
    }

    // Arithmetic operators: +, -, *, /, %
    if (op == lexer::TokenType::Plus || op == lexer::TokenType::Minus ||
        op == lexer::TokenType::Star || op == lexer::TokenType::Slash || op == lexer::TokenType::Percent) {
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            m_diagnostics.reportError("E1001", "Operator '" + std::string(lexer::tokenTypeToString(op)) +
                                      "' cannot be applied to types '" + leftType->toString() + "' and '" +
                                      rightType->toString() + "'", node.getLocation(), "Operands must be numeric");
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_typeCtx.getCommonType(leftType, rightType);
        return;
    }

    // Relational operators: <, <=, >, >=
    if (op == lexer::TokenType::Less || op == lexer::TokenType::LessEqual ||
        op == lexer::TokenType::Greater || op == lexer::TokenType::GreaterEqual) {
        if (!leftType->isNumeric() || !rightType->isNumeric()) {
            m_diagnostics.reportError("E1001", "Comparison operator '" + std::string(lexer::tokenTypeToString(op)) +
                                      "' cannot be applied to types '" + leftType->toString() + "' and '" +
                                      rightType->toString() + "'", node.getLocation(), "Both operands must be numeric");
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_typeCtx.getBool();
        return;
    }

    // Equality operators: ==, !=
    if (op == lexer::TokenType::EqualEqual || op == lexer::TokenType::BangEqual) {
        if (!leftType->isAssignableTo(*rightType) && !rightType->isAssignableTo(*leftType) &&
            !(leftType->isNull() && rightType->isNullable()) && !(rightType->isNull() && leftType->isNullable())) {
            m_diagnostics.reportError("E1001", "Cannot compare incompatible types '" + leftType->toString() +
                                      "' and '" + rightType->toString() + "'", node.getLocation());
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_typeCtx.getBool();
        return;
    }

    // Logical operators: &&, ||
    if (op == lexer::TokenType::AmpAmp || op == lexer::TokenType::PipePipe) {
        if (!leftType->isBoolean() || !rightType->isBoolean()) {
            m_diagnostics.reportError("E1001", "Logical operator '" + std::string(lexer::tokenTypeToString(op)) +
                                      "' requires boolean operands, got '" + leftType->toString() + "' and '" +
                                      rightType->toString() + "'", node.getLocation());
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_typeCtx.getBool();
        return;
    }

    // Null coalescing: ??
    if (op == lexer::TokenType::QuestionQuestion) {
        if (leftType->isNullable()) {
            const auto& nullable = static_cast<const NullableType&>(*leftType);
            m_currentExprType = m_typeCtx.getCommonType(nullable.getUnderlyingType(), rightType);
        } else {
            m_currentExprType = m_typeCtx.getCommonType(leftType, rightType);
        }
        return;
    }

    // Bitwise operators: &, |, ^, <<, >>
    if (op == lexer::TokenType::Amp || op == lexer::TokenType::Pipe || op == lexer::TokenType::Caret ||
        op == lexer::TokenType::LessLess || op == lexer::TokenType::GreaterGreater) {
        if (!leftType->isInteger() || !rightType->isInteger()) {
            m_diagnostics.reportError("E1001", "Bitwise operator requires integer operands, got '" +
                                      leftType->toString() + "' and '" + rightType->toString() + "'", node.getLocation());
            m_currentExprType = m_typeCtx.getError();
            return;
        }
        m_currentExprType = m_typeCtx.getCommonType(leftType, rightType);
        return;
    }

    m_currentExprType = m_typeCtx.getCommonType(leftType, rightType);
}

void TypeChecker::visit(const ast::UnaryExpr& node) {
    auto operandType = checkExpr(node.getOperand());
    if (operandType->isError()) {
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    lexer::TokenType op = node.getOp();

    switch (op) {
        case lexer::TokenType::Plus:
        case lexer::TokenType::Minus:
            if (!operandType->isNumeric()) {
                m_diagnostics.reportError("E1001", "Unary operator '" + std::string(lexer::tokenTypeToString(op)) +
                                          "' requires numeric operand, got '" + operandType->toString() + "'", node.getLocation());
                m_currentExprType = m_typeCtx.getError();
                return;
            }
            m_currentExprType = operandType;
            break;
        case lexer::TokenType::Bang:
            if (!operandType->isBoolean()) {
                m_diagnostics.reportError("E1001", "Logical negation '!' requires boolean operand, got '" +
                                          operandType->toString() + "'", node.getLocation());
                m_currentExprType = m_typeCtx.getError();
                return;
            }
            m_currentExprType = m_typeCtx.getBool();
            break;
        case lexer::TokenType::Tilde:
            if (!operandType->isInteger()) {
                m_diagnostics.reportError("E1001", "Bitwise NOT '~' requires integer operand, got '" +
                                          operandType->toString() + "'", node.getLocation());
                m_currentExprType = m_typeCtx.getError();
                return;
            }
            m_currentExprType = operandType;
            break;
        case lexer::TokenType::PlusPlus:
        case lexer::TokenType::MinusMinus:
            if (!operandType->isNumeric()) {
                m_diagnostics.reportError("E1001", "Increment/decrement operator requires numeric operand, got '" +
                                          operandType->toString() + "'", node.getLocation());
                m_currentExprType = m_typeCtx.getError();
                return;
            }
            m_currentExprType = operandType;
            break;
        default:
            m_currentExprType = operandType;
            break;
    }
}

std::shared_ptr<FunctionSymbol> TypeChecker::resolveOverload(
    const std::vector<std::shared_ptr<FunctionSymbol>>& candidates,
    const std::vector<TypePtr>& argTypes,
    const diagnostics::SourceLocation& loc,
    const std::string& name
) {
    if (candidates.empty()) {
        m_diagnostics.reportError("E1003", "Undefined function or method '" + name + "'", loc);
        return nullptr;
    }

    std::vector<std::shared_ptr<FunctionSymbol>> viable;
    for (const auto& candidate : candidates) {
        const auto& paramTypes = candidate->getFunctionType()->getParamTypes();
        const auto* astDecl = candidate->getASTDecl();
        size_t minParams = paramTypes.size();
        size_t maxParams = paramTypes.size();
        if (astDecl) {
            minParams = 0;
            for (const auto& p : astDecl->getParams()) {
                if (!p.hasDefaultValue()) {
                    minParams++;
                }
            }
        }

        if (argTypes.size() < minParams || argTypes.size() > maxParams) continue;

        bool match = true;
        for (size_t i = 0; i < argTypes.size(); ++i) {
            if (!argTypes[i]->isAssignableTo(*paramTypes[i])) {
                match = false;
                break;
            }
        }
        if (match) {
            viable.push_back(candidate);
        }
    }

    if (viable.empty()) {
        std::string argList;
        for (size_t i = 0; i < argTypes.size(); ++i) {
            if (i > 0) argList += ", ";
            argList += argTypes[i]->toString();
        }
        m_diagnostics.reportError("E1001", "No matching overload for function '" + name + "' with argument types (" +
                                  argList + ")", loc);
        return nullptr;
    }

    // If multiple match, prefer exact matches with identical arg count
    for (const auto& candidate : viable) {
        const auto& paramTypes = candidate->getFunctionType()->getParamTypes();
        if (paramTypes.size() != argTypes.size()) continue;
        bool exact = true;
        for (size_t i = 0; i < argTypes.size(); ++i) {
            if (!argTypes[i]->equals(*paramTypes[i])) {
                exact = false;
                break;
            }
        }
        if (exact) return candidate;
    }

    return viable.front();
}

void TypeChecker::visit(const ast::CallExpr& node) {
    std::vector<TypePtr> argTypes;
    for (const auto& argPtr : node.getArgs()) {
        argTypes.push_back(checkExpr(*argPtr));
    }

    const auto& callee = node.getCallee();

    // Direct function call: fn(...)
    if (callee.getKind() == ast::ASTNodeKind::IdentifierExpr) {
        const auto& idExpr = static_cast<const ast::IdentifierExpr&>(callee);
        const std::string& fnName = idExpr.getName();

        auto candidates = m_symbolTable.lookupFunctions(fnName);
        if (!candidates.empty()) {
            auto resolved = resolveOverload(candidates, argTypes, node.getLocation(), fnName);
            if (resolved) {
                if (resolved->isGeneric() && resolved->getASTDecl()) {
                    const auto* astDecl = resolved->getASTDecl();
                    std::vector<std::string> typeArgs = node.getTypeArgs();
                    if (typeArgs.empty() && !argTypes.empty()) {
                        for (const auto& argT : argTypes) {
                            typeArgs.push_back(argT->toString());
                        }
                    }
                    const_cast<ast::CallExpr&>(node).setTypeArgs(typeArgs);
                    std::string specRetStr = TypeContext::substituteType(astDecl->getReturnType(), astDecl->getTypeParams(), typeArgs);
                    auto retT = m_typeCtx.resolveType(specRetStr);
                    m_currentExprType = retT ? retT : resolved->getFunctionType()->getReturnType();
                } else {
                    m_currentExprType = resolved->getFunctionType()->getReturnType();
                }
            } else {
                m_currentExprType = m_typeCtx.getError();
            }
            return;
        }

        // Check super constructor call: super(...)
        if (fnName == "super") {
            if (!m_currentEnclosingClass || !m_currentEnclosingClass->getSuperclass()) {
                m_diagnostics.reportError("E1001", "Cannot call 'super' constructor outside of a subclass", node.getLocation());
                m_currentExprType = m_typeCtx.getError();
            } else {
                m_currentExprType = m_typeCtx.getVoid();
            }
            return;
        }

        // Check method on enclosing class: method(...)
        if (m_currentEnclosingClass) {
            auto methods = m_currentEnclosingClass->findMethods(fnName);
            if (!methods.empty()) {
                std::vector<std::shared_ptr<FunctionSymbol>> methodCandidates;
                for (const auto& mm : methods) {
                    methodCandidates.push_back(std::make_shared<FunctionSymbol>(mm.name, mm.functionType, node.getLocation()));
                }
                auto resolved = resolveOverload(methodCandidates, argTypes, node.getLocation(), fnName);
                if (resolved) {
                    m_currentExprType = resolved->getFunctionType()->getReturnType();
                } else {
                    m_currentExprType = m_typeCtx.getError();
                }
                return;
            }
        }

        // Check constructor call: ClassName(...)
        auto userType = m_typeCtx.findUserType(fnName);
        if (userType && userType->isClass()) {
            m_currentExprType = userType;
            return;
        }

        // Check if identifier is a variable holding a function
        auto sym = m_symbolTable.lookup(fnName);
        if (sym && sym->getType() && sym->getType()->isFunction()) {
            auto fnType = std::static_pointer_cast<FunctionType>(sym->getType());
            m_currentExprType = fnType->getReturnType();
            return;
        }

        m_diagnostics.reportError("E1003", "Undefined function or callable symbol '" + fnName + "'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    // Method call: obj.method(...) or obj?.method(...)
    if (callee.getKind() == ast::ASTNodeKind::MemberExpr) {
        const auto& memberExpr = static_cast<const ast::MemberExpr&>(callee);
        auto objType = checkExpr(memberExpr.getObject());
        const std::string& methodName = memberExpr.getMember();

        if (objType->isNullable()) {
            if (!memberExpr.isSafeNav()) {
                m_diagnostics.reportError("E1008", "Null safety violation: call on nullable type '" + objType->toString() +
                                          "' requires safe navigation '?.'", node.getLocation(), "Use obj?." + methodName + "(...)");
            }
            const auto& nullable = static_cast<const NullableType&>(*objType);
            objType = nullable.getUnderlyingType();
        }

        if (objType->isClass()) {
            auto clsType = std::static_pointer_cast<ClassType>(objType);
            auto methods = clsType->findMethods(methodName);

            std::vector<std::shared_ptr<FunctionSymbol>> candidates;
            for (const auto& mm : methods) {
                // Check visibility
                if (mm.visibility == "private" && (!m_currentEnclosingClass || !m_currentEnclosingClass->equals(*clsType))) {
                    m_diagnostics.reportError("E1006", "Cannot access private method '" + methodName + "' of class '" +
                                              clsType->getName() + "'", node.getLocation());
                    m_currentExprType = m_typeCtx.getError();
                    return;
                }
                candidates.push_back(std::make_shared<FunctionSymbol>(mm.name, mm.functionType, node.getLocation()));
            }

            auto resolved = resolveOverload(candidates, argTypes, node.getLocation(), methodName);
            if (resolved) {
                m_currentExprType = resolved->getFunctionType()->getReturnType();
            } else {
                m_currentExprType = m_typeCtx.getError();
            }
            return;
        }

        if (objType->isInterface()) {
            auto ifaceType = std::static_pointer_cast<InterfaceType>(objType);
            auto methods = ifaceType->findMethods(methodName);

            std::vector<std::shared_ptr<FunctionSymbol>> candidates;
            for (const auto& mm : methods) {
                candidates.push_back(std::make_shared<FunctionSymbol>(mm.name, mm.functionType, node.getLocation()));
            }

            auto resolved = resolveOverload(candidates, argTypes, node.getLocation(), methodName);
            if (resolved) {
                m_currentExprType = resolved->getFunctionType()->getReturnType();
            } else {
                m_currentExprType = m_typeCtx.getError();
            }
            return;
        }

        if (objType->isString()) {
            if (methodName == "length" || methodName == "size") {
                m_currentExprType = m_typeCtx.getInt();
                return;
            }
            if (methodName == "upper" || methodName == "lower" || methodName == "trim") {
                m_currentExprType = m_typeCtx.getString();
                return;
            }
            if (methodName == "contains" || methodName == "startsWith" || methodName == "endsWith" || methodName == "isEmpty") {
                m_currentExprType = m_typeCtx.getBool();
                return;
            }
            if (methodName == "indexOf") {
                m_currentExprType = m_typeCtx.getInt();
                return;
            }
            if (methodName == "replace" || methodName == "substring") {
                m_currentExprType = m_typeCtx.getString();
                return;
            }
            if (methodName == "charAt") {
                m_currentExprType = m_typeCtx.getChar();
                return;
            }
            if (methodName == "split") {
                m_currentExprType = m_typeCtx.getArrayType(m_typeCtx.getString());
                return;
            }
            if (methodName == "reverse" || methodName == "repeat") {
                m_currentExprType = m_typeCtx.getString();
                return;
            }
        }

        if (objType->isArray()) {
            if (methodName == "length" || methodName == "size") {
                m_currentExprType = m_typeCtx.getInt();
                return;
            }
            if (methodName == "isEmpty") {
                m_currentExprType = m_typeCtx.getBool();
                return;
            }
            const auto& arr = static_cast<const ArrayType&>(*objType);
            if (methodName == "push" || methodName == "add") {
                m_currentExprType = m_typeCtx.getVoid();
                return;
            }
            if (methodName == "pop" || methodName == "removeLast") {
                m_currentExprType = arr.getElementType();
                return;
            }
            if (methodName == "contains") {
                m_currentExprType = m_typeCtx.getBool();
                return;
            }
            if (methodName == "get") {
                m_currentExprType = arr.getElementType();
                return;
            }
            if (methodName == "set") {
                m_currentExprType = m_typeCtx.getVoid();
                return;
            }
            if (methodName == "map" || methodName == "filter") {
                m_currentExprType = objType;
                return;
            }
            if (methodName == "reduce") {
                m_currentExprType = arr.getElementType();
                return;
            }
            if (methodName == "clear" || methodName == "removeAt") {
                m_currentExprType = m_typeCtx.getVoid();
                return;
            }
        }

        if (objType->isMap()) {
            const auto& mapType = static_cast<const MapType&>(*objType);
            if (methodName == "size" || methodName == "length") {
                m_currentExprType = m_typeCtx.getInt();
                return;
            }
            if (methodName == "isEmpty") {
                m_currentExprType = m_typeCtx.getBool();
                return;
            }
            if (methodName == "get") {
                m_currentExprType = m_typeCtx.getNullableType(mapType.getValueType());
                return;
            }
            if (methodName == "set" || methodName == "remove" || methodName == "clear") {
                m_currentExprType = m_typeCtx.getVoid();
                return;
            }
            if (methodName == "contains" || methodName == "containsKey") {
                m_currentExprType = m_typeCtx.getBool();
                return;
            }
        }
        if (objType->isInteger() && methodName == "parse") {
            m_currentExprType = m_typeCtx.getInt();
            return;
        }
        if (objType->isDouble() && methodName == "parse") {
            m_currentExprType = m_typeCtx.getDouble();
            return;
        }
        if (objType->isFloat() && methodName == "parse") {
            m_currentExprType = m_typeCtx.getFloat();
            return;
        }
        if (objType->isBoolean() && methodName == "parse") {
            m_currentExprType = m_typeCtx.getBool();
            return;
        }

        m_diagnostics.reportError("E1003", "Type '" + objType->toString() + "' has no method named '" + methodName + "'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    if (callee.getKind() == ast::ASTNodeKind::IdentifierExpr) {
        const auto& idExpr = static_cast<const ast::IdentifierExpr&>(callee);
        auto sym = m_symbolTable.lookup(idExpr.getName());
        if (sym && sym->getKind() == SymbolKind::Function) {
            auto fnSym = std::static_pointer_cast<FunctionSymbol>(sym);
            if (fnSym->isGeneric() && fnSym->getASTDecl()) {
                const auto* astDecl = fnSym->getASTDecl();
                std::vector<std::string> typeArgs = node.getTypeArgs();
                if (typeArgs.empty() && !argTypes.empty()) {
                    for (const auto& argT : argTypes) {
                        typeArgs.push_back(argT->toString());
                    }
                }
                std::string specRetStr = TypeContext::substituteType(astDecl->getReturnType(), astDecl->getTypeParams(), typeArgs);
                auto retT = m_typeCtx.resolveType(specRetStr);
                if (retT) {
                    m_currentExprType = retT;
                    return;
                }
            }
        }
    }

    auto calleeType = checkExpr(callee);
    if (calleeType->isFunction()) {
        auto fnType = std::static_pointer_cast<FunctionType>(calleeType);
        m_currentExprType = fnType->getReturnType();
        return;
    }

    m_diagnostics.reportError("E1001", "Expression is not callable, has type '" + calleeType->toString() + "'", node.getLocation());
    m_currentExprType = m_typeCtx.getError();
}

void TypeChecker::visit(const ast::MemberExpr& node) {
    auto objType = checkExpr(node.getObject());
    const std::string& member = node.getMember();

    if (objType->isNullable()) {
        if (!node.isSafeNav()) {
            m_diagnostics.reportError("E1008", "Null safety violation: access on nullable type '" + objType->toString() +
                                      "' requires safe navigation '?.'", node.getLocation(), "Use obj?." + member);
        }
        const auto& nullable = static_cast<const NullableType&>(*objType);
        objType = nullable.getUnderlyingType();
    }

    if (objType->isClass()) {
        auto clsType = std::static_pointer_cast<ClassType>(objType);
        auto fieldOpt = clsType->findField(member);
        if (fieldOpt.has_value()) {
            const auto& field = fieldOpt.value();
            if (field.visibility == "private" && (!m_currentEnclosingClass || !m_currentEnclosingClass->equals(*clsType))) {
                m_diagnostics.reportError("E1006", "Cannot access private field '" + member + "' of class '" + clsType->getName() + "'", node.getLocation());
                m_currentExprType = m_typeCtx.getError();
                return;
            }
            m_currentExprType = field.type;
            return;
        }

        auto methods = clsType->findMethods(member);
        if (!methods.empty()) {
            m_currentExprType = methods.front().functionType;
            return;
        }

        m_diagnostics.reportError("E1002", "Class '" + clsType->getName() + "' has no member named '" + member + "'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    if (objType->isStruct()) {
        auto structType = std::static_pointer_cast<StructType>(objType);
        auto fieldOpt = structType->findField(member);
        if (fieldOpt.has_value()) {
            m_currentExprType = fieldOpt.value().type;
            return;
        }

        m_diagnostics.reportError("E1002", "Struct '" + structType->getName() + "' has no field named '" + member + "'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    if (objType->isEnum()) {
        auto enumType = std::static_pointer_cast<EnumType>(objType);
        if (enumType->hasVariant(member)) {
            m_currentExprType = enumType;
            return;
        }

        m_diagnostics.reportError("E1002", "Enum '" + enumType->getName() + "' has no variant named '" + member + "'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    // Built-in properties: array length
    if (objType->isArray() && (member == "length" || member == "size")) {
        m_currentExprType = m_typeCtx.getInt();
        return;
    }

    // Built-in properties: string length
    if (objType->isString() && (member == "length" || member == "size")) {
        m_currentExprType = m_typeCtx.getInt();
        return;
    }

    // Built-in static methods / members: type.parse
    if (objType->isInteger() && member == "parse") {
        m_currentExprType = m_typeCtx.getFunctionType({m_typeCtx.getString()}, m_typeCtx.getInt());
        return;
    }
    if (objType->isDouble() && member == "parse") {
        m_currentExprType = m_typeCtx.getFunctionType({m_typeCtx.getString()}, m_typeCtx.getDouble());
        return;
    }
    if (objType->isFloat() && member == "parse") {
        m_currentExprType = m_typeCtx.getFunctionType({m_typeCtx.getString()}, m_typeCtx.getFloat());
        return;
    }
    if (objType->isBoolean() && member == "parse") {
        m_currentExprType = m_typeCtx.getFunctionType({m_typeCtx.getString()}, m_typeCtx.getBool());
        return;
    }

    m_diagnostics.reportError("E1002", "Type '" + objType->toString() + "' has no member '" + member + "'", node.getLocation());
    m_currentExprType = m_typeCtx.getError();
}

void TypeChecker::visit(const ast::IndexExpr& node) {
    auto objType = checkExpr(node.getObject());
    auto indexType = checkExpr(node.getIndex());

    if (objType->isArray()) {
        if (!indexType->isInteger()) {
            m_diagnostics.reportError("E1001", "Array index must be an integer, got '" + indexType->toString() + "'", node.getLocation());
        }
        const auto& arr = static_cast<const ArrayType&>(*objType);
        m_currentExprType = arr.getElementType();
        return;
    }

    if (objType->isClass() && objType->toString().rfind("List<", 0) == 0) {
        if (!indexType->isInteger()) {
            m_diagnostics.reportError("E1001", "List index must be an integer, got '" + indexType->toString() + "'", node.getLocation());
        }
        std::string s = objType->toString();
        size_t lt = s.find('<');
        size_t gt = s.rfind('>');
        std::string elemStr = s.substr(lt + 1, gt - lt - 1);
        auto elemType = m_typeCtx.resolveType(elemStr);
        m_currentExprType = elemType ? elemType : m_typeCtx.getAny();
        return;
    }

    if (objType->isMap()) {
        const auto& mapType = static_cast<const MapType&>(*objType);
        if (!indexType->isAssignableTo(*mapType.getKeyType())) {
            m_diagnostics.reportError("E1001", "Map key type mismatch: expected '" + mapType.getKeyType()->toString() +
                                      "', got '" + indexType->toString() + "'", node.getLocation());
        }
        m_currentExprType = m_typeCtx.getNullableType(mapType.getValueType());
        return;
    }

    m_diagnostics.reportError("E1001", "Type '" + objType->toString() + "' does not support indexing", node.getLocation());
    m_currentExprType = m_typeCtx.getError();
}

void TypeChecker::visit(const ast::ArrayExpr& node) {
    if (node.getElements().empty()) {
        m_currentExprType = m_typeCtx.getArrayType(m_typeCtx.getAny());
        return;
    }

    TypePtr elemType = checkExpr(*node.getElements().front());
    for (size_t i = 1; i < node.getElements().size(); ++i) {
        auto nextType = checkExpr(*node.getElements()[i]);
        elemType = m_typeCtx.getCommonType(elemType, nextType);
    }

    m_currentExprType = m_typeCtx.getArrayType(elemType);
}

void TypeChecker::visit(const ast::MapExpr& node) {
    if (node.getEntries().empty()) {
        m_currentExprType = m_typeCtx.getMapType(m_typeCtx.getString(), m_typeCtx.getAny());
        return;
    }

    TypePtr keyType = checkExpr(*node.getEntries().front().first);
    TypePtr valType = checkExpr(*node.getEntries().front().second);

    for (size_t i = 1; i < node.getEntries().size(); ++i) {
        auto nextK = checkExpr(*node.getEntries()[i].first);
        auto nextV = checkExpr(*node.getEntries()[i].second);
        keyType = m_typeCtx.getCommonType(keyType, nextK);
        valType = m_typeCtx.getCommonType(valType, nextV);
    }

    m_currentExprType = m_typeCtx.getMapType(keyType, valType);
}

void TypeChecker::visit(const ast::LambdaExpr& node) {
    m_symbolTable.enterScope(ScopeKind::Function);

    std::vector<TypePtr> paramTypes;
    for (const auto& param : node.getParams()) {
        auto pType = m_typeCtx.resolveType(param.type);
        if (!pType) pType = m_typeCtx.getInt();
        paramTypes.push_back(pType);

        if (!param.name.empty()) {
            auto paramSym = std::make_shared<VariableSymbol>(param.name, pType, node.getLocation(), false, true);
            m_symbolTable.define(paramSym);
        }
    }

    TypePtr returnType = m_typeCtx.resolveType(node.getReturnType());
    TypePtr prevReturnType = m_currentFunctionReturnType;

    if (!returnType) {
        if (const auto* exprBody = dynamic_cast<const ast::ExpressionNode*>(&node.getBody())) {
            returnType = checkExpr(*exprBody);
        } else if (const auto* blockBody = dynamic_cast<const ast::BlockStmt*>(&node.getBody())) {
            m_currentFunctionReturnType = m_typeCtx.getUnknown();
            checkStmt(*blockBody);
            returnType = (m_currentFunctionReturnType && !m_currentFunctionReturnType->isUnknown() && !m_currentFunctionReturnType->isAny())
                             ? m_currentFunctionReturnType
                             : m_typeCtx.getInt();
        } else {
            returnType = m_typeCtx.getInt();
        }
    } else {
        m_currentFunctionReturnType = returnType;
        if (const auto* exprBody = dynamic_cast<const ast::ExpressionNode*>(&node.getBody())) {
            checkExpr(*exprBody);
        } else if (const auto* blockBody = dynamic_cast<const ast::BlockStmt*>(&node.getBody())) {
            checkStmt(*blockBody);
        }
    }

    m_currentFunctionReturnType = prevReturnType;
    m_symbolTable.exitScope();

    if (!returnType || returnType->isError()) returnType = m_typeCtx.getInt();
    m_currentExprType = m_typeCtx.getFunctionType(std::move(paramTypes), std::move(returnType));
}

void TypeChecker::visit(const ast::TernaryExpr& node) {
    auto condType = checkExpr(node.getCondition());
    if (!condType->isBoolean()) {
        m_diagnostics.reportError("E1001", "Ternary condition must be of type bool, got '" + condType->toString() + "'", node.getLocation());
    }

    auto thenType = checkExpr(node.getThenExpr());
    auto elseType = checkExpr(node.getElseExpr());

    m_currentExprType = m_typeCtx.getCommonType(thenType, elseType);
}

void TypeChecker::visit(const ast::AssignExpr& node) {
    if (const auto* id = dynamic_cast<const ast::IdentifierExpr*>(&node.getTarget())) {
        auto sym = m_symbolTable.lookup(id->getName());
        if (sym && (sym->getKind() == SymbolKind::Constant ||
                    (std::dynamic_pointer_cast<VariableSymbol>(sym) && std::dynamic_pointer_cast<VariableSymbol>(sym)->isConst()))) {
            m_diagnostics.reportError("E1005", "Cannot assign to constant '" + id->getName() + "'", node.getLocation(),
                                      "Constants are immutable and cannot be reassigned");
            m_currentExprType = m_typeCtx.getError();
            return;
        }
    }

    auto targetType = checkExpr(node.getTarget());
    auto valueType = checkExpr(node.getValue());

    if (targetType->getKind() == TypeKind::Float && valueType->getKind() == TypeKind::Double) {
        if (dynamic_cast<const ast::LiteralExpr*>(&node.getValue())) {
            valueType = m_typeCtx.getFloat();
        }
    }
    if (targetType->isInteger() && valueType->isInteger()) {
        if (const auto* lit = dynamic_cast<const ast::LiteralExpr*>(&node.getValue())) {
            if (lit->getLiteralKind() == ast::LiteralKind::Integer) {
                valueType = targetType;
            }
        }
    }

    // Check null safety (E1008)
    if (!targetType->isNullable() && valueType->isNull()) {
        m_diagnostics.reportError("E1008", "Null safety violation: cannot assign null to non-nullable type '" +
                                  targetType->toString() + "'", node.getLocation(), "Change variable type to '" +
                                  targetType->toString() + "?' to allow null");
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    if (!valueType->isAssignableTo(*targetType)) {
        m_diagnostics.reportError("E1001", "Cannot assign value of type '" + valueType->toString() +
                                  "' to variable of type '" + targetType->toString() + "'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    m_currentExprType = targetType;
}

void TypeChecker::visit(const ast::NewExpr& node) {
    const std::string& typeName = node.getTypeName();

    auto resolveArgOrAny = [&](const std::string& argStr) -> TypePtr {
        auto t = m_typeCtx.resolveType(argStr);
        if (t) return t;
        if (m_currentEnclosingClass) {
            for (const auto& tp : m_currentEnclosingClass->getTypeParams()) {
                if (tp == argStr) return m_typeCtx.getAny();
            }
        }
        return nullptr;
    };

    // Check built-in collections instantiation: new List<T>() or new Map<K, V>()
    if (typeName == "Array") {
        TypePtr elem = node.isGeneric() ? resolveArgOrAny(node.getTypeArgs()[0]) : m_typeCtx.getAny();
        if (!elem) elem = m_typeCtx.getAny();
        for (const auto& argPtr : node.getArgs()) {
            checkExpr(*argPtr);
        }
        m_currentExprType = m_typeCtx.getArrayType(elem);
        return;
    }
    if (typeName == "List") {
        TypePtr elem = node.isGeneric() ? resolveArgOrAny(node.getTypeArgs()[0]) : m_typeCtx.getAny();
        if (!elem) elem = m_typeCtx.getAny();
        for (const auto& argPtr : node.getArgs()) {
            checkExpr(*argPtr);
        }
        m_currentExprType = m_typeCtx.resolveType("List<" + elem->toString() + ">");
        return;
    }
    if (typeName == "Map") {
        TypePtr k = node.getTypeArgs().size() > 0 ? resolveArgOrAny(node.getTypeArgs()[0]) : m_typeCtx.getString();
        TypePtr v = node.getTypeArgs().size() > 1 ? resolveArgOrAny(node.getTypeArgs()[1]) : m_typeCtx.getAny();
        if (!k) k = m_typeCtx.getAny();
        if (!v) v = m_typeCtx.getAny();
        for (const auto& argPtr : node.getArgs()) {
            checkExpr(*argPtr);
        }
        m_currentExprType = m_typeCtx.getMapType(k, v);
        return;
    }
    if (typeName == "Set") {
        TypePtr elem = node.isGeneric() ? resolveArgOrAny(node.getTypeArgs()[0]) : m_typeCtx.getAny();
        if (!elem) elem = m_typeCtx.getAny();
        for (const auto& argPtr : node.getArgs()) {
            checkExpr(*argPtr);
        }
        m_currentExprType = m_typeCtx.resolveType("Set<" + elem->toString() + ">");
        return;
    }

    std::string resolvedName = typeName;
    if (node.isGeneric()) {
        resolvedName += "<";
        for (size_t i = 0; i < node.getTypeArgs().size(); ++i) {
            if (i > 0) resolvedName += ", ";
            resolvedName += node.getTypeArgs()[i];
        }
        resolvedName += ">";
    }

    auto userType = m_typeCtx.resolveType(resolvedName);
    if (!userType) {
        userType = m_typeCtx.findUserType(typeName);
    }

    if (!userType) {
        m_diagnostics.reportError("E1002", "Undefined class '" + typeName + "' in 'new' expression", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    if (!userType->isClass()) {
        m_diagnostics.reportError("E1001", "Cannot instantiate non-class type '" + typeName + "' with 'new'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    auto clsType = std::static_pointer_cast<ClassType>(userType);
    if (clsType->isAbstract()) {
        m_diagnostics.reportError("E1001", "Cannot instantiate abstract class '" + typeName + "'", node.getLocation());
        m_currentExprType = m_typeCtx.getError();
        return;
    }

    for (const auto& argPtr : node.getArgs()) {
        checkExpr(*argPtr);
    }

    m_currentExprType = clsType;
}

// Statements
void TypeChecker::visit(const ast::BlockStmt& node) {
    m_symbolTable.enterScope(ScopeKind::Block);
    for (const auto& stmtPtr : node.getStatements()) {
        if (stmtPtr) checkStmt(*stmtPtr);
    }
    m_symbolTable.exitScope();
}

void TypeChecker::visit(const ast::ExprStmt& node) {
    checkExpr(node.getExpr());
}

void TypeChecker::visit(const ast::VarDeclStmt& node) {
    const std::string& name = node.getName();
    TypePtr varType = nullptr;

    if (!node.getType().empty()) {
        varType = m_typeCtx.resolveType(node.getType());
        if (!varType) {
            m_diagnostics.reportError("E1002", "Undefined type '" + node.getType() + "' in declaration of variable '" + name + "'", node.getLocation());
            varType = m_typeCtx.getError();
        }

        if (node.hasInitializer()) {
            auto initType = checkExpr(*node.getInitializer());

            if (varType->getKind() == TypeKind::Float && initType->getKind() == TypeKind::Double) {
                if (dynamic_cast<const ast::LiteralExpr*>(node.getInitializer())) {
                    initType = m_typeCtx.getFloat();
                }
            }
            if (varType->isInteger() && initType->isInteger()) {
                if (const auto* lit = dynamic_cast<const ast::LiteralExpr*>(node.getInitializer())) {
                    if (lit->getLiteralKind() == ast::LiteralKind::Integer) {
                        initType = varType;
                    }
                }
            }

            // Null safety check (E1008)
            if (!varType->isNullable() && initType->isNull()) {
                m_diagnostics.reportError("E1008", "Null safety violation: cannot assign null to non-nullable variable '" +
                                          name + "' of type '" + varType->toString() + "'", node.getLocation(),
                                          "Use '" + varType->toString() + "?' to declare a nullable variable");
            } else if (!initType->isAssignableTo(*varType)) {
                m_diagnostics.reportError("E1001", "Type mismatch: cannot initialize variable of type '" +
                                          varType->toString() + "' with value of type '" + initType->toString() + "'", node.getLocation());
            }
        }
    } else if (node.isVar() || node.isConst()) {
        // Type inference: var x = ... or let x = ...
        if (!node.hasInitializer()) {
            m_diagnostics.reportError("E1001", "Variable '" + name + "' declared with 'var' or 'let' must have an initializer for type inference", node.getLocation());
            varType = m_typeCtx.getError();
        } else {
            varType = checkExpr(*node.getInitializer());
            if (varType->isNull()) {
                m_diagnostics.reportError("E1001", "Cannot infer type from 'null' without explicit type annotation", node.getLocation());
                varType = m_typeCtx.getError();
            }
        }
    } else {
        varType = m_typeCtx.resolveType(node.getType());
        if (!varType) {
            m_diagnostics.reportError("E1002", "Undefined type '" + node.getType() + "' in declaration of variable '" + name + "'", node.getLocation());
            varType = m_typeCtx.getError();
        }

        if (node.hasInitializer()) {
            auto initType = checkExpr(*node.getInitializer());

            if (varType->getKind() == TypeKind::Float && initType->getKind() == TypeKind::Double) {
                if (dynamic_cast<const ast::LiteralExpr*>(node.getInitializer())) {
                    initType = m_typeCtx.getFloat();
                }
            }
            if (varType->isInteger() && initType->isInteger()) {
                if (const auto* lit = dynamic_cast<const ast::LiteralExpr*>(node.getInitializer())) {
                    if (lit->getLiteralKind() == ast::LiteralKind::Integer) {
                        initType = varType;
                    }
                }
            }

            // Null safety check (E1008)
            if (!varType->isNullable() && initType->isNull()) {
                m_diagnostics.reportError("E1008", "Null safety violation: cannot assign null to non-nullable variable '" +
                                          name + "' of type '" + varType->toString() + "'", node.getLocation(),
                                          "Use '" + varType->toString() + "?' to declare a nullable variable");
            } else if (!initType->isAssignableTo(*varType)) {
                m_diagnostics.reportError("E1001", "Type mismatch: cannot initialize variable of type '" +
                                          varType->toString() + "' with value of type '" + initType->toString() + "'", node.getLocation());
            }
        }
    }

    if (node.isWeak() && varType && !varType->isError()) {
        bool isRef = varType->isReferenceType() || varType->isAny();
        if (varType->isNullable()) {
            auto nullUnderlying = std::static_pointer_cast<NullableType>(varType)->getUnderlyingType();
            if (nullUnderlying && (nullUnderlying->isReferenceType() || nullUnderlying->isAny())) {
                isRef = true;
            }
        }
        if (!isRef) {
            m_diagnostics.reportError("E1015", "Modifier 'weak' can only be applied to reference types (classes or interfaces), but found value type '" + varType->toString() + "'", node.getLocation());
        }
    }

    auto sym = std::make_shared<VariableSymbol>(name, varType, node.getLocation(), node.isConst());
    if (!m_symbolTable.define(sym)) {
        m_diagnostics.reportError("E1004", "Duplicate declaration of variable '" + name + "' in this scope", node.getLocation());
    }
}

void TypeChecker::visit(const ast::IfStmt& node) {
    auto condType = checkExpr(node.getCondition());
    if (!condType->isBoolean()) {
        m_diagnostics.reportError("E1001", "If condition must be of type bool, got '" + condType->toString() + "'", node.getLocation());
    }
    checkStmt(node.getThenBranch());
    if (node.hasElseBranch()) {
        checkStmt(*node.getElseBranch());
    }
}

void TypeChecker::visit(const ast::WhileStmt& node) {
    auto condType = checkExpr(node.getCondition());
    if (!condType->isBoolean()) {
        m_diagnostics.reportError("E1001", "While condition must be of type bool, got '" + condType->toString() + "'", node.getLocation());
    }
    m_loopDepth++;
    checkStmt(node.getBody());
    m_loopDepth--;
}

void TypeChecker::visit(const ast::DoWhileStmt& node) {
    m_loopDepth++;
    checkStmt(node.getBody());
    m_loopDepth--;
    auto condType = checkExpr(node.getCondition());
    if (!condType->isBoolean()) {
        m_diagnostics.reportError("E1001", "Do-while condition must be of type bool, got '" + condType->toString() + "'", node.getLocation());
    }
}

void TypeChecker::visit(const ast::ForStmt& node) {
    m_symbolTable.enterScope(ScopeKind::Block);
    if (node.hasInit()) {
        checkStmt(*node.getInit());
    }
    if (node.hasCondition()) {
        auto condType = checkExpr(*node.getCondition());
        if (!condType->isBoolean()) {
            m_diagnostics.reportError("E1001", "For loop condition must be of type bool, got '" + condType->toString() + "'", node.getLocation());
        }
    }
    if (node.hasIncrement()) {
        checkExpr(*node.getIncrement());
    }
    m_loopDepth++;
    checkStmt(node.getBody());
    m_loopDepth--;
    m_symbolTable.exitScope();
}

void TypeChecker::visit(const ast::ForInStmt& node) {
    auto iterType = checkExpr(node.getIterable());

    TypePtr itemType = m_typeCtx.getAny();
    if (iterType->isArray()) {
        const auto& arr = static_cast<const ArrayType&>(*iterType);
        itemType = arr.getElementType();
    } else if (iterType->isClass() && iterType->toString().rfind("List<", 0) == 0) {
        std::string s = iterType->toString();
        size_t lt = s.find('<');
        size_t gt = s.rfind('>');
        itemType = m_typeCtx.resolveType(s.substr(lt + 1, gt - lt - 1));
        if (!itemType) itemType = m_typeCtx.getAny();
    } else if (iterType->isMap()) {
        const auto& map = static_cast<const MapType&>(*iterType);
        itemType = map.getKeyType();
    } else {
        m_diagnostics.reportError("E1001", "For-in loop requires an iterable collection or array, got '" + iterType->toString() + "'", node.getLocation());
    }

    if (!node.getType().empty() && node.getType() != "var" && node.getType() != "let") {
        auto annotType = m_typeCtx.resolveType(node.getType());
        if (annotType) {
            itemType = annotType;
        }
    }

    m_symbolTable.enterScope(ScopeKind::Block);
    auto varSym = std::make_shared<VariableSymbol>(node.getVarName(), itemType, node.getLocation());
    m_symbolTable.define(varSym);

    m_loopDepth++;
    checkStmt(node.getBody());
    m_loopDepth--;

    m_symbolTable.exitScope();
}

void TypeChecker::visit(const ast::SwitchStmt& node) {
    auto subjType = checkExpr(node.getSubject());

    m_switchDepth++;
    for (const auto& sc : node.getCases()) {
        auto caseValType = checkExpr(sc.getValue());
        if (!caseValType->isAssignableTo(*subjType)) {
            m_diagnostics.reportError("E1001", "Case value type '" + caseValType->toString() +
                                      "' does not match switch expression type '" + subjType->toString() + "'", sc.getValue().getLocation());
        }
        for (const auto& s : sc.getStatements()) {
            if (s) checkStmt(*s);
        }
    }

    for (const auto& defStmt : node.getDefaultStatements()) {
        if (defStmt) checkStmt(*defStmt);
    }
    m_switchDepth--;
}

void TypeChecker::visit(const ast::MatchStmt& node) {
    auto subjType = checkExpr(node.getSubject());

    for (const auto& arm : node.getArms()) {
        if (!arm.isWildcard()) {
            auto patType = checkExpr(*arm.getPattern());
            if (!patType->isAssignableTo(*subjType)) {
                m_diagnostics.reportError("E1001", "Match pattern type '" + patType->toString() +
                                          "' does not match subject type '" + subjType->toString() + "'", arm.getPattern()->getLocation());
            }
        }
        if (arm.getBody().getKind() == ast::ASTNodeKind::BlockStmt) {
            checkStmt(static_cast<const ast::StatementNode&>(arm.getBody()));
        } else {
            checkExpr(static_cast<const ast::ExpressionNode&>(arm.getBody()));
        }
    }
}

void TypeChecker::visit(const ast::ReturnStmt& node) {
    if (!m_currentFunctionReturnType) {
        m_diagnostics.reportError("E1005", "Return statement outside of any function", node.getLocation());
        return;
    }

    if (m_currentFunctionReturnType->isVoid()) {
        if (node.hasValue()) {
            m_diagnostics.reportError("E1005", "Void function cannot return a value", node.getLocation());
        }
        return;
    }

    if (!node.hasValue()) {
        m_diagnostics.reportError("E1005", "Non-void function must return a value of type '" +
                                  m_currentFunctionReturnType->toString() + "'", node.getLocation());
        return;
    }

    auto valType = checkExpr(*node.getValue());
    if (m_currentFunctionReturnType->isUnknown() || m_currentFunctionReturnType->isAny()) {
        m_currentFunctionReturnType = valType;
    } else if (!valType->isAssignableTo(*m_currentFunctionReturnType)) {
        m_diagnostics.reportError("E1005", "Return type mismatch: expected '" + m_currentFunctionReturnType->toString() +
                                  "', got '" + valType->toString() + "'", node.getLocation());
    }
}

void TypeChecker::visit(const ast::BreakStmt& node) {
    if (m_loopDepth <= 0 && m_switchDepth <= 0) {
        m_diagnostics.reportError("E1001", "'break' statement outside of loop or switch", node.getLocation());
    }
}

void TypeChecker::visit(const ast::ContinueStmt& node) {
    if (m_loopDepth <= 0) {
        m_diagnostics.reportError("E1001", "'continue' statement outside of loop", node.getLocation());
    }
}

void TypeChecker::visit(const ast::TryCatchStmt& node) {
    checkStmt(node.getTryBlock());

    for (const auto& catchClause : node.getCatches()) {
        m_symbolTable.enterScope(ScopeKind::Block);
        TypePtr excType = m_typeCtx.resolveType(catchClause.getType());
        if (!excType) excType = m_typeCtx.getAny();

        auto excSym = std::make_shared<VariableSymbol>(catchClause.getName(), excType, node.getLocation());
        m_symbolTable.define(excSym);

        checkStmt(catchClause.getBody());
        m_symbolTable.exitScope();
    }

    if (node.hasFinally()) {
        checkStmt(*node.getFinallyBlock());
    }
}

void TypeChecker::visit(const ast::ThrowStmt& node) {
    checkExpr(node.getExpr());
}

bool TypeChecker::hasGuaranteedReturn(const ast::StatementNode& stmt) {
    switch (stmt.getKind()) {
        case ast::ASTNodeKind::ReturnStmt:
        case ast::ASTNodeKind::ThrowStmt:
            return true;
        case ast::ASTNodeKind::BlockStmt: {
            const auto& block = static_cast<const ast::BlockStmt&>(stmt);
            for (const auto& s : block.getStatements()) {
                if (s && hasGuaranteedReturn(*s)) return true;
            }
            return false;
        }
        case ast::ASTNodeKind::IfStmt: {
            const auto& ifStmt = static_cast<const ast::IfStmt&>(stmt);
            if (!ifStmt.hasElseBranch()) return false;
            return hasGuaranteedReturn(ifStmt.getThenBranch()) && hasGuaranteedReturn(*ifStmt.getElseBranch());
        }
        default:
            return false;
    }
}

void TypeChecker::visit(const ast::FunctionDecl& node) {
    TypePtr returnType = m_typeCtx.getVoid();
    if (!node.getReturnType().empty()) {
        auto r = m_typeCtx.resolveType(node.getReturnType());
        if (r) {
            returnType = r;
        } else {
            std::vector<std::string> activeParams = node.getTypeParams();
            if (m_currentEnclosingClass) {
                for (const auto& tp : m_currentEnclosingClass->getTypeParams()) {
                    activeParams.push_back(tp);
                }
            }
            if (!activeParams.empty()) {
                std::vector<std::string> anyArgs(activeParams.size(), "any");
                std::string subStr = TypeContext::substituteType(node.getReturnType(), activeParams, anyArgs);
                auto subR = m_typeCtx.resolveType(subStr);
                if (subR) {
                    returnType = subR;
                }
            }
        }
    }

    TypePtr prevReturnType = m_currentFunctionReturnType;
    m_currentFunctionReturnType = returnType;

    m_symbolTable.enterScope(ScopeKind::Function);

    for (const auto& param : node.getParams()) {
        auto pType = m_typeCtx.resolveType(param.getType());
        if (!pType) {
            std::vector<std::string> activeParams = node.getTypeParams();
            if (m_currentEnclosingClass) {
                for (const auto& tp : m_currentEnclosingClass->getTypeParams()) {
                    activeParams.push_back(tp);
                }
            }
            if (!activeParams.empty()) {
                std::vector<std::string> anyArgs(activeParams.size(), "any");
                std::string subStr = TypeContext::substituteType(param.getType(), activeParams, anyArgs);
                pType = m_typeCtx.resolveType(subStr);
            }
        }
        if (!pType) pType = m_typeCtx.getAny();

        auto paramSym = std::make_shared<VariableSymbol>(param.getName(), pType, node.getLocation(), false, true);
        if (!m_symbolTable.define(paramSym)) {
            m_diagnostics.reportError("E1004", "Duplicate parameter name '" + param.getName() + "' in function '" + node.getName() + "'", node.getLocation());
        }

        if (param.hasDefaultValue()) {
            auto defType = checkExpr(*param.getDefaultValue());
            if (!defType->isAssignableTo(*pType)) {
                m_diagnostics.reportError("E1001", "Default value type '" + defType->toString() + "' does not match parameter type '" + pType->toString() + "'", node.getLocation());
            }
        }
    }

    if (node.hasBody()) {
        checkStmt(*node.getBody());

        // Return path analysis (E1005)
        if (!returnType->isVoid() && !node.isAbstract()) {
            if (!hasGuaranteedReturn(*node.getBody())) {
                m_diagnostics.reportError("E1005", "Missing return statement: function '" + node.getName() +
                                          "' must return a value of type '" + returnType->toString() + "' along all code paths", node.getLocation());
            }
        }
    } else if (node.isShortForm()) {
        auto shortType = checkExpr(*node.getShortBody());
        if (returnType->isVoid() && (node.getReturnType().empty() || node.getReturnType() == "void")) {
            returnType = shortType;
            const_cast<ast::FunctionDecl&>(node).setReturnType(shortType->toString());
            auto syms = m_symbolTable.lookupFunctions(node.getName());
            for (auto& s : syms) {
                if (s->getASTDecl() == &node) {
                    std::vector<TypePtr> pTypes = s->getFunctionType()->getParamTypes();
                    auto newFnType = m_typeCtx.getFunctionType(std::move(pTypes), shortType);
                    s->setFunctionType(newFnType);
                    break;
                }
            }
        } else if (!shortType->isAssignableTo(*returnType)) {
            m_diagnostics.reportError("E1005", "Short function body return type mismatch: expected '" +
                                      returnType->toString() + "', got '" + shortType->toString() + "'", node.getLocation());
        }
    }

    m_symbolTable.exitScope();
    m_currentFunctionReturnType = prevReturnType;
}

void TypeChecker::verifyInterfaceImplementations(const ast::ClassDecl& decl, const std::shared_ptr<ClassType>& clsType) {
    if (decl.isAbstract()) return; // Abstract classes do not need to implement all interface methods immediately

    for (const auto& iface : clsType->getInterfaces()) {
        for (const auto& ifaceMethod : iface->getMethods()) {
            auto clsMethods = clsType->findMethods(ifaceMethod.name);
            bool implemented = false;
            for (const auto& cm : clsMethods) {
                if (cm.functionType->equals(*ifaceMethod.functionType)) {
                    implemented = true;
                    break;
                }
            }
            if (!implemented) {
                m_diagnostics.reportError("E1001", "Class '" + decl.getName() + "' does not implement interface method '" +
                                          ifaceMethod.name + "': " + ifaceMethod.functionType->toString() +
                                          " from interface '" + iface->getName() + "'", decl.getLocation());
            }
        }
    }
}

void TypeChecker::visit(const ast::ClassDecl& node) {
    auto userType = m_typeCtx.findUserType(node.getName());
    if (!userType || !userType->isClass()) return;
    auto clsType = std::static_pointer_cast<ClassType>(userType);

    auto prevClass = m_currentEnclosingClass;
    m_currentEnclosingClass = clsType;

    m_symbolTable.enterScope(ScopeKind::Class);

    // Define 'this' in class scope
    auto thisSym = std::make_shared<VariableSymbol>("this", clsType, node.getLocation());
    m_symbolTable.define(thisSym);

    // Define fields in class scope
    for (const auto& [name, field] : clsType->getFields()) {
        auto fieldSym = std::make_shared<VariableSymbol>(field.name, field.type, node.getLocation(), field.isConst, false, field.visibility, field.isStatic);
        m_symbolTable.define(fieldSym);
    }

    // Check interface implementations
    verifyInterfaceImplementations(node, clsType);

    // Check member definitions
    for (const auto& member : node.getMembers()) {
        if (!member) continue;
        if (member->getKind() == ast::ASTNodeKind::FunctionDecl) {
            visit(static_cast<const ast::FunctionDecl&>(*member));
        } else if (member->getKind() == ast::ASTNodeKind::VarDeclStmt) {
            const auto& varDecl = static_cast<const ast::VarDeclStmt&>(*member);
            if (varDecl.hasInitializer()) {
                auto fieldOpt = clsType->findField(varDecl.getName());
                TypePtr fieldType = fieldOpt ? fieldOpt->type : m_typeCtx.getAny();
                auto initType = checkExpr(*varDecl.getInitializer());
                if (!initType->isAssignableTo(*fieldType)) {
                    m_diagnostics.reportError("E1001", "Type mismatch in field initializer for '" + varDecl.getName() +
                                              "': expected '" + fieldType->toString() + "', got '" + initType->toString() + "'", varDecl.getLocation());
                }
            }
        }
    }

    m_symbolTable.exitScope();
    m_currentEnclosingClass = prevClass;
}

void TypeChecker::visit(const ast::InterfaceDecl& /*node*/) {
    // Interface method signatures are verified in NameResolver Pass 2
}

void TypeChecker::visit(const ast::StructDecl& /*node*/) {
    // Struct fields are verified in NameResolver Pass 2
}

void TypeChecker::visit(const ast::EnumDecl& /*node*/) {
    // Enum variants are verified in NameResolver Pass 2
}

void TypeChecker::visit(const ast::ModuleDecl& /*node*/) {
}

void TypeChecker::visit(const ast::ImportDecl& /*node*/) {
}

void TypeChecker::visit(const ast::Program& node) {
    // Pre-pass: Infer return types of short-form functions
    for (const auto& decl : node.getDeclarations()) {
        if (decl && decl->getKind() == ast::ASTNodeKind::FunctionDecl) {
            const auto& fn = static_cast<const ast::FunctionDecl&>(*decl);
            if (fn.isShortForm() && (fn.getReturnType().empty() || fn.getReturnType() == "void")) {
                visit(fn);
            }
        }
    }

    for (const auto& decl : node.getDeclarations()) {
        if (decl) {
            switch (decl->getKind()) {
                case ast::ASTNodeKind::FunctionDecl:
                    visit(static_cast<const ast::FunctionDecl&>(*decl));
                    break;
                case ast::ASTNodeKind::ClassDecl:
                    visit(static_cast<const ast::ClassDecl&>(*decl));
                    break;
                case ast::ASTNodeKind::InterfaceDecl:
                    visit(static_cast<const ast::InterfaceDecl&>(*decl));
                    break;
                case ast::ASTNodeKind::StructDecl:
                    visit(static_cast<const ast::StructDecl&>(*decl));
                    break;
                case ast::ASTNodeKind::EnumDecl:
                    visit(static_cast<const ast::EnumDecl&>(*decl));
                    break;
                case ast::ASTNodeKind::BlockStmt: {
                    const auto& block = static_cast<const ast::BlockStmt&>(*decl);
                    for (const auto& inner : block.getStatements()) {
                        if (inner && inner->getKind() == ast::ASTNodeKind::FunctionDecl) {
                            visit(static_cast<const ast::FunctionDecl&>(*inner));
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

} // namespace sp::sema
