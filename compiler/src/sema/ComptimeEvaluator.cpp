#include "sp/sema/ComptimeEvaluator.h"
#include <cmath>
#include <sstream>
#include <iostream>

namespace sp::sema {

std::string ComptimeValue::toString() const {
    switch (kind) {
        case ComptimeKind::Void: return "void";
        case ComptimeKind::Null: return "null";
        case ComptimeKind::Int: return std::to_string(intVal);
        case ComptimeKind::Float: return std::to_string(floatVal);
        case ComptimeKind::Bool: return boolVal ? "true" : "false";
        case ComptimeKind::String: return stringVal;
        case ComptimeKind::Char: return std::string(1, charVal);
        case ComptimeKind::Array: {
            std::string res = "[";
            for (size_t i = 0; i < arrayVal.size(); ++i) {
                if (i > 0) res += ", ";
                res += arrayVal[i].toString();
            }
            res += "]";
            return res;
        }
    }
    return "unknown";
}

bool ComptimeValue::isTruthy() const {
    switch (kind) {
        case ComptimeKind::Bool: return boolVal;
        case ComptimeKind::Int: return intVal != 0;
        case ComptimeKind::Float: return floatVal != 0.0;
        case ComptimeKind::String: return !stringVal.empty();
        case ComptimeKind::Array: return !arrayVal.empty();
        default: return false;
    }
}

std::unique_ptr<ast::ExpressionNode> ComptimeValue::toLiteralAST(diagnostics::SourceLocation loc) const {
    switch (kind) {
        case ComptimeKind::Int:
            return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, std::to_string(intVal), loc);
        case ComptimeKind::Float:
            return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Float, std::to_string(floatVal), loc);
        case ComptimeKind::Bool:
            return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Boolean, boolVal ? "true" : "false", loc);
        case ComptimeKind::String:
            return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, stringVal, loc);
        case ComptimeKind::Char:
            return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Char, std::string(1, charVal), loc);
        case ComptimeKind::Null:
            return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Null, "null", loc);
        case ComptimeKind::Array: {
            std::vector<ast::ExprPtr> elements;
            for (const auto& elem : arrayVal) {
                elements.push_back(elem.toLiteralAST(loc));
            }
            return std::make_unique<ast::ArrayExpr>(std::move(elements), loc);
        }
        case ComptimeKind::Void:
        default:
            return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Null, "null", loc);
    }
}

ComptimeEvaluator::ComptimeEvaluator(diagnostics::DiagnosticEngine* diagnostics)
    : m_diagnostics(diagnostics) {
    pushScope();
}

void ComptimeEvaluator::pushScope() {
    m_scopes.emplace_back();
}

void ComptimeEvaluator::popScope() {
    if (!m_scopes.empty()) {
        m_scopes.pop_back();
    }
}

void ComptimeEvaluator::setVar(const std::string& name, ComptimeValue val) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            found->second = std::move(val);
            return;
        }
    }
    if (!m_scopes.empty()) {
        m_scopes.back()[name] = std::move(val);
    }
}

// Phase 4 slice D: fresh binding in the innermost scope only.
//
// The old code used setVar() for parameter binding, which searches ALL
// enclosing scopes and OVERWRITES the first existing binding. In a
// recursive call that clobbered the CALLER's parameter: fibonacci(5)
// called fibonacci(4), which overwrote the outer `n` with 4, so after the
// first recursive call returned, `fibonacci(n - 2)` read the corrupted
// `n` -- comptime fibonacci(5) folded to -15 and fibonacci(10) to -80
// instead of 5 and 55 (runtime evaluation was correct). Single-recursion
// functions like factorial were immune: they never read `n` again after a
// recursive call returns. defineVar() gives every call frame its own
// parameter bindings; `let` declarations get the same treatment so an
// inner `let x` cannot clobber an outer `x`.
void ComptimeEvaluator::defineVar(const std::string& name, ComptimeValue val) {
    if (!m_scopes.empty()) {
        m_scopes.back()[name] = std::move(val);
    }
}

bool ComptimeEvaluator::getVar(const std::string& name, ComptimeValue& outVal) const {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            outVal = found->second;
            return true;
        }
    }
    return false;
}

void ComptimeEvaluator::reportError(std::string_view code, std::string message, const diagnostics::SourceLocation& loc) {
    m_hasError = true;
    m_lastError = message;
    if (m_diagnostics) {
        m_diagnostics->reportError(code, message, loc);
    }
}

void ComptimeEvaluator::registerFunction(const ast::FunctionDecl* fn) {
    if (fn) {
        m_functions[fn->getName()] = fn;
    }
}

ComptimeValue ComptimeEvaluator::evaluate(const ast::ExpressionNode& expr) {
    if (++m_stepCount > MAX_EVAL_STEPS) {
        reportError("E1800", "Compile-time execution step limit exceeded (possible infinite loop or recursion)", expr.getLocation());
        return ComptimeValue::voidVal();
    }

    switch (expr.getKind()) {
        case ast::ASTNodeKind::LiteralExpr: {
            const auto& lit = static_cast<const ast::LiteralExpr&>(expr);
            switch (lit.getLiteralKind()) {
                case ast::LiteralKind::Integer: {
                    try {
                        return ComptimeValue::fromInt(std::stoll(lit.getValue()));
                    } catch (...) {
                        return ComptimeValue::fromInt(0);
                    }
                }
                case ast::LiteralKind::Float: {
                    try {
                        return ComptimeValue::fromFloat(std::stod(lit.getValue()));
                    } catch (...) {
                        return ComptimeValue::fromFloat(0.0);
                    }
                }
                case ast::LiteralKind::Boolean:
                    return ComptimeValue::fromBool(lit.getValue() == "true");
                case ast::LiteralKind::String:
                    return ComptimeValue::fromString(lit.getValue());
                case ast::LiteralKind::Char:
                    return ComptimeValue::fromChar(lit.getValue().empty() ? '\0' : lit.getValue()[0]);
                case ast::LiteralKind::Null:
                    return ComptimeValue::nullVal();
            }
            break;
        }

        case ast::ASTNodeKind::IdentifierExpr: {
            const auto& id = static_cast<const ast::IdentifierExpr&>(expr);
            ComptimeValue val;
            if (getVar(id.getName(), val)) {
                return val;
            }
            reportError("E1803", "Undefined variable in compile-time context: '" + id.getName() + "'", id.getLocation());
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::BinaryExpr:
            return evaluateBinary(static_cast<const ast::BinaryExpr&>(expr));

        case ast::ASTNodeKind::UnaryExpr:
            return evaluateUnary(static_cast<const ast::UnaryExpr&>(expr));

        case ast::ASTNodeKind::CallExpr:
            return evaluateCall(static_cast<const ast::CallExpr&>(expr));

        case ast::ASTNodeKind::ArrayExpr: {
            const auto& arr = static_cast<const ast::ArrayExpr&>(expr);
            std::vector<ComptimeValue> elements;
            for (const auto& elem : arr.getElements()) {
                if (elem) elements.push_back(evaluate(*elem));
            }
            return ComptimeValue::fromArray(std::move(elements));
        }

        case ast::ASTNodeKind::IndexExpr: {
            const auto& idxExpr = static_cast<const ast::IndexExpr&>(expr);
            ComptimeValue arrVal = evaluate(idxExpr.getObject());
            ComptimeValue idxVal = evaluate(idxExpr.getIndex());
            if (arrVal.kind == ComptimeKind::Array && idxVal.kind == ComptimeKind::Int) {
                int64_t i = idxVal.intVal;
                if (i >= 0 && static_cast<size_t>(i) < arrVal.arrayVal.size()) {
                    return arrVal.arrayVal[static_cast<size_t>(i)];
                }
                reportError("E1804", "Array index out of bounds in comptime evaluation: " + std::to_string(i), expr.getLocation());
            } else if (arrVal.kind == ComptimeKind::String && idxVal.kind == ComptimeKind::Int) {
                int64_t i = idxVal.intVal;
                if (i >= 0 && static_cast<size_t>(i) < arrVal.stringVal.size()) {
                    return ComptimeValue::fromChar(arrVal.stringVal[static_cast<size_t>(i)]);
                }
                reportError("E1804", "String index out of bounds in comptime evaluation: " + std::to_string(i), expr.getLocation());
            } else if (!m_hasError) {
                // Phase 3 slice D: indexing a non-array/non-string (or with
                // a non-int index) is a loud error, never a silent void.
                reportError("E1807", "Indexing is only supported on arrays and strings with an int index in compile-time evaluation", expr.getLocation());
            }
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::MemberExpr: {
            const auto& mem = static_cast<const ast::MemberExpr&>(expr);
            ComptimeValue obj = evaluate(mem.getObject());
            if (mem.getMember() == "length") {
                if (obj.kind == ComptimeKind::Array) {
                    return ComptimeValue::fromInt(static_cast<int64_t>(obj.arrayVal.size()));
                } else if (obj.kind == ComptimeKind::String) {
                    return ComptimeValue::fromInt(static_cast<int64_t>(obj.stringVal.size()));
                }
            }
            // Phase 3 slice D: only `.length` is supported on values in
            // comptime; any other member access is a loud error, never a
            // silent void.
            if (!m_hasError) {
                reportError("E1807", "Only '.length' is supported on values in compile-time evaluation (got '." + mem.getMember() + "')", expr.getLocation());
            }
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::TernaryExpr: {
            const auto& tern = static_cast<const ast::TernaryExpr&>(expr);
            ComptimeValue cond = evaluate(tern.getCondition());
            return cond.isTruthy() ? evaluate(tern.getThenExpr()) : evaluate(tern.getElseExpr());
        }

        case ast::ASTNodeKind::AssignExpr: {
            const auto& assign = static_cast<const ast::AssignExpr&>(expr);
            if (assign.getTarget().getKind() == ast::ASTNodeKind::IdentifierExpr) {
                const auto& targetId = static_cast<const ast::IdentifierExpr&>(assign.getTarget());
                ComptimeValue rightVal = evaluate(assign.getValue());
                setVar(targetId.getName(), rightVal);
                return rightVal;
            }
            if (!m_hasError) {
                reportError("E1807", "Only assignment to a local variable is supported in compile-time evaluation", expr.getLocation());
            }
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::ComptimeExpr: {
            const auto& comp = static_cast<const ast::ComptimeExpr&>(expr);
            if (comp.hasExpr()) {
                return evaluate(*comp.getExpr());
            } else if (comp.isBlock() && comp.getBlock()) {
                bool returned = false;
                return evaluateBlock(*comp.getBlock(), returned);
            }
            return ComptimeValue::voidVal();
        }

        default:
            // Phase 3 slice D: an expression kind the evaluator cannot
            // interpret is a loud error, never a silent void that the
            // caller might mistake for a value (or fold to null).
            if (!m_hasError) {
                reportError("E1807", "This expression kind is not supported in compile-time evaluation", expr.getLocation());
            }
            break;
    }

    return ComptimeValue::voidVal();
}

ComptimeValue ComptimeEvaluator::evaluateBinary(const ast::BinaryExpr& expr) {
    ComptimeValue left = evaluate(expr.getLeft());
    ComptimeValue right = evaluate(expr.getRight());

    // Logical short-circuiting handled if both evaluated
    switch (expr.getOp()) {
        case lexer::TokenType::Plus:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal + right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromFloat(l + r);
            }
            if (left.kind == ComptimeKind::String || right.kind == ComptimeKind::String) {
                return ComptimeValue::fromString(left.toString() + right.toString());
            }
            break;

        case lexer::TokenType::Minus:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal - right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromFloat(l - r);
            }
            break;

        case lexer::TokenType::Star:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal * right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromFloat(l * r);
            }
            break;

        case lexer::TokenType::Slash:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                if (right.intVal == 0) {
                    reportError("E1805", "Division by zero in compile-time evaluation", expr.getLocation());
                    return ComptimeValue::fromInt(0);
                }
                return ComptimeValue::fromInt(left.intVal / right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                if (r == 0.0) {
                    reportError("E1805", "Division by zero in compile-time evaluation", expr.getLocation());
                    return ComptimeValue::fromFloat(0.0);
                }
                return ComptimeValue::fromFloat(l / r);
            }
            break;

        case lexer::TokenType::Percent:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                if (right.intVal == 0) {
                    reportError("E1805", "Modulo by zero in compile-time evaluation", expr.getLocation());
                    return ComptimeValue::fromInt(0);
                }
                return ComptimeValue::fromInt(left.intVal % right.intVal);
            }
            break;

        case lexer::TokenType::EqualEqual:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromBool(left.intVal == right.intVal);
            }
            if (left.kind == ComptimeKind::Float && right.kind == ComptimeKind::Float) {
                return ComptimeValue::fromBool(left.floatVal == right.floatVal);
            }
            if ((left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Float) ||
                (left.kind == ComptimeKind::Float && right.kind == ComptimeKind::Int)) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromBool(l == r);
            }
            if (left.kind == ComptimeKind::Bool && right.kind == ComptimeKind::Bool) {
                return ComptimeValue::fromBool(left.boolVal == right.boolVal);
            }
            if (left.kind == ComptimeKind::String && right.kind == ComptimeKind::String) {
                return ComptimeValue::fromBool(left.stringVal == right.stringVal);
            }
            if (left.kind == ComptimeKind::Char && right.kind == ComptimeKind::Char) {
                return ComptimeValue::fromBool(left.charVal == right.charVal);
            }
            return ComptimeValue::fromBool(left.kind == right.kind);

        case lexer::TokenType::BangEqual:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromBool(left.intVal != right.intVal);
            }
            if (left.kind == ComptimeKind::Float && right.kind == ComptimeKind::Float) {
                return ComptimeValue::fromBool(left.floatVal != right.floatVal);
            }
            if ((left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Float) ||
                (left.kind == ComptimeKind::Float && right.kind == ComptimeKind::Int)) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromBool(l != r);
            }
            if (left.kind == ComptimeKind::Bool && right.kind == ComptimeKind::Bool) {
                return ComptimeValue::fromBool(left.boolVal != right.boolVal);
            }
            if (left.kind == ComptimeKind::String && right.kind == ComptimeKind::String) {
                return ComptimeValue::fromBool(left.stringVal != right.stringVal);
            }
            if (left.kind == ComptimeKind::Char && right.kind == ComptimeKind::Char) {
                return ComptimeValue::fromBool(left.charVal != right.charVal);
            }
            return ComptimeValue::fromBool(left.kind != right.kind);

        case lexer::TokenType::Less:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromBool(left.intVal < right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromBool(l < r);
            }
            break;

        case lexer::TokenType::Greater:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromBool(left.intVal > right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromBool(l > r);
            }
            break;

        case lexer::TokenType::LessEqual:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromBool(left.intVal <= right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromBool(l <= r);
            }
            break;

        case lexer::TokenType::GreaterEqual:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromBool(left.intVal >= right.intVal);
            }
            if (left.kind == ComptimeKind::Float || right.kind == ComptimeKind::Float) {
                double l = (left.kind == ComptimeKind::Float) ? left.floatVal : static_cast<double>(left.intVal);
                double r = (right.kind == ComptimeKind::Float) ? right.floatVal : static_cast<double>(right.intVal);
                return ComptimeValue::fromBool(l >= r);
            }
            break;

        case lexer::TokenType::AmpAmp:
            return ComptimeValue::fromBool(left.isTruthy() && right.isTruthy());

        case lexer::TokenType::PipePipe:
            return ComptimeValue::fromBool(left.isTruthy() || right.isTruthy());

        case lexer::TokenType::Amp:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal & right.intVal);
            }
            break;

        case lexer::TokenType::Pipe:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal | right.intVal);
            }
            break;

        case lexer::TokenType::Caret:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal ^ right.intVal);
            }
            break;

        case lexer::TokenType::LessLess:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal << right.intVal);
            }
            break;

        case lexer::TokenType::GreaterGreater:
            if (left.kind == ComptimeKind::Int && right.kind == ComptimeKind::Int) {
                return ComptimeValue::fromInt(left.intVal >> right.intVal);
            }
            break;

        default:
            break;
    }

    // Phase 3 slice D: reaching here means the operator has no lowering
    // for these operand types (e.g. float `<=`, char arithmetic). Loud
    // error, never a silent void.
    if (!m_hasError) {
        reportError("E1807", "This operator cannot be applied to these operand types in compile-time evaluation", expr.getLocation());
    }
    return ComptimeValue::voidVal();
}

ComptimeValue ComptimeEvaluator::evaluateUnary(const ast::UnaryExpr& expr) {
    ComptimeValue op = evaluate(expr.getOperand());
    switch (expr.getOp()) {
        case lexer::TokenType::Minus:
            if (op.kind == ComptimeKind::Int) return ComptimeValue::fromInt(-op.intVal);
            if (op.kind == ComptimeKind::Float) return ComptimeValue::fromFloat(-op.floatVal);
            break;
        case lexer::TokenType::Bang:
            return ComptimeValue::fromBool(!op.isTruthy());
        case lexer::TokenType::Tilde:
            if (op.kind == ComptimeKind::Int) return ComptimeValue::fromInt(~op.intVal);
            break;
        default:
            break;
    }
    // Phase 3 slice D: no lowering for this operand type — loud error,
    // never a silent void.
    if (!m_hasError) {
        reportError("E1807", "This unary operator cannot be applied to this operand type in compile-time evaluation", expr.getLocation());
    }
    return ComptimeValue::voidVal();
}

ComptimeValue ComptimeEvaluator::evaluateCall(const ast::CallExpr& expr) {
    std::string fnName;
    if (expr.getCallee().getKind() == ast::ASTNodeKind::IdentifierExpr) {
        fnName = static_cast<const ast::IdentifierExpr&>(expr.getCallee()).getName();
    }

    std::vector<ComptimeValue> args;
    for (const auto& arg : expr.getArgs()) {
        if (arg) args.push_back(evaluate(*arg));
    }

    // Builtins
    if (fnName == "assert") {
        if (!args.empty() && !args[0].isTruthy()) {
            std::string msg = args.size() > 1 ? args[1].toString() : "Compile-time assertion failed";
            reportError("E1801", msg, expr.getLocation());
        }
        return ComptimeValue::voidVal();
    }

    if (fnName == "min" && args.size() == 2) {
        if (args[0].kind == ComptimeKind::Int && args[1].kind == ComptimeKind::Int) {
            return ComptimeValue::fromInt(std::min(args[0].intVal, args[1].intVal));
        }
    }

    if (fnName == "max" && args.size() == 2) {
        if (args[0].kind == ComptimeKind::Int && args[1].kind == ComptimeKind::Int) {
            return ComptimeValue::fromInt(std::max(args[0].intVal, args[1].intVal));
        }
    }

    if (fnName == "abs" && args.size() == 1) {
        if (args[0].kind == ComptimeKind::Int) {
            return ComptimeValue::fromInt(std::abs(args[0].intVal));
        }
    }

    // User-defined function
    auto it = m_functions.find(fnName);
    if (it != m_functions.end() && it->second) {
        const auto* fn = it->second;
        pushScope();

        for (size_t i = 0; i < fn->getParams().size() && i < args.size(); ++i) {
            // Phase 4 slice D: defineVar, not setVar -- each call frame
            // needs its own parameter bindings (see defineVar).
            defineVar(fn->getParams()[i].getName(), args[i]);
        }

        ComptimeValue res = ComptimeValue::voidVal();
        if (fn->hasBody()) {
            bool returned = false;
            res = evaluateBlock(*fn->getBody(), returned);
        } else if (fn->isShortForm()) {
            res = evaluate(*fn->getShortBody());
        }

        popScope();
        return res;
    }

    // Phase 3 slice D: calling anything that is not a registered
    // `comptime fn` (or a comptime builtin) is a loud error. The old code
    // silently returned void, so `comptime println("x")` or a call to a
    // plain runtime function would fold to nothing with no diagnostic.
    // (An arg that already errored still reports only that first error.)
    if (!m_hasError) {
        std::string what = fnName.empty()
            ? "a method or non-identifier call"
            : "function '" + fnName + "'";
        reportError("E1806", "Cannot call " + what + " in compile-time evaluation: only `comptime fn` functions and the builtins min/max/abs/assert are available", expr.getLocation());
    }
    return ComptimeValue::voidVal();
}

ComptimeValue ComptimeEvaluator::evaluateStmt(const ast::StatementNode& stmt, bool& returned) {
    switch (stmt.getKind()) {
        case ast::ASTNodeKind::VarDeclStmt: {
            const auto& varDecl = static_cast<const ast::VarDeclStmt&>(stmt);
            ComptimeValue initVal = ComptimeValue::voidVal();
            if (varDecl.hasInitializer()) {
                initVal = evaluate(*varDecl.getInitializer());
            }
            // Phase 4 slice D: defineVar, not setVar -- a `let` declares a
            // new variable; an inner `let x` must shadow, not clobber, an
            // outer `x` (same reason as parameter binding, see defineVar).
            defineVar(varDecl.getName(), std::move(initVal));
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::ExprStmt: {
            const auto& exprStmt = static_cast<const ast::ExprStmt&>(stmt);
            return evaluate(exprStmt.getExpr());
        }

        case ast::ASTNodeKind::ReturnStmt: {
            const auto& retStmt = static_cast<const ast::ReturnStmt&>(stmt);
            returned = true;
            if (retStmt.hasValue()) {
                return evaluate(*retStmt.getValue());
            }
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::IfStmt: {
            const auto& ifStmt = static_cast<const ast::IfStmt&>(stmt);
            ComptimeValue cond = evaluate(ifStmt.getCondition());
            if (cond.isTruthy()) {
                return evaluateStmt(ifStmt.getThenBranch(), returned);
            } else if (ifStmt.hasElseBranch()) {
                return evaluateStmt(*ifStmt.getElseBranch(), returned);
            }
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::WhileStmt: {
            const auto& whileStmt = static_cast<const ast::WhileStmt&>(stmt);
            while (evaluate(whileStmt.getCondition()).isTruthy()) {
                evaluateStmt(whileStmt.getBody(), returned);
                if (returned || m_hasError) break;
            }
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::ForStmt: {
            const auto& forStmt = static_cast<const ast::ForStmt&>(stmt);
            pushScope();
            if (forStmt.hasInit()) {
                evaluateStmt(*forStmt.getInit(), returned);
            }
            while (!forStmt.hasCondition() || evaluate(*forStmt.getCondition()).isTruthy()) {
                evaluateStmt(forStmt.getBody(), returned);
                if (returned || m_hasError) break;
                if (forStmt.hasIncrement()) {
                    evaluate(*forStmt.getIncrement());
                }
            }
            popScope();
            return ComptimeValue::voidVal();
        }

        case ast::ASTNodeKind::BlockStmt:
            return evaluateBlock(static_cast<const ast::BlockStmt&>(stmt), returned);

        default:
            // Phase 3 slice D: a statement kind the evaluator cannot
            // interpret (throw, try/catch, switch, ...) is a loud error,
            // never a silent skip.
            if (!m_hasError) {
                reportError("E1807", "This statement kind is not supported in compile-time evaluation", stmt.getLocation());
            }
            break;
    }
    return ComptimeValue::voidVal();
}

ComptimeValue ComptimeEvaluator::evaluateBlock(const ast::BlockStmt& block, bool& returned) {
    pushScope();
    ComptimeValue lastVal = ComptimeValue::voidVal();
    for (const auto& stmtPtr : block.getStatements()) {
        if (!stmtPtr) continue;
        lastVal = evaluateStmt(*stmtPtr, returned);
        if (returned || m_hasError) break;
    }
    popScope();
    return lastVal;
}

std::unique_ptr<ast::ExpressionNode> ComptimeEvaluator::foldToLiteral(const ast::ExpressionNode& expr) {
    ComptimeValue val = evaluate(expr);
    return val.toLiteralAST(expr.getLocation());
}

void ComptimeEvaluator::processProgram(const ast::Program& program) {
    // Phase 3 slice D: register ONLY functions marked `comptime`. The old
    // code registered every function, so a comptime call could silently
    // interpret a runtime function body (whose IO/side effects the
    // evaluator cannot reproduce). Only `comptime fn` bodies are pure
    // compile-time code; anything else called from comptime is a loud
    // error (see evaluateCall).
    for (const auto& decl : program.getDeclarations()) {
        if (decl && decl->getKind() == ast::ASTNodeKind::FunctionDecl) {
            const auto* fn = static_cast<const ast::FunctionDecl*>(decl.get());
            if (fn->isComptime()) {
                registerFunction(fn);
            }
        }
    }
}

} // namespace sp::sema
