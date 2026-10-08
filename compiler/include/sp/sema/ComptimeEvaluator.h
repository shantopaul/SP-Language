#pragma once

#include "sp/ast/ASTNode.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/diagnostics/DiagnosticEngine.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace sp::sema {

enum class ComptimeKind {
    Void,
    Int,
    Float,
    Bool,
    String,
    Char,
    Array,
    Null
};

struct ComptimeValue {
    ComptimeKind kind{ComptimeKind::Void};
    int64_t intVal{0};
    double floatVal{0.0};
    bool boolVal{false};
    std::string stringVal;
    char charVal{'\0'};
    std::vector<ComptimeValue> arrayVal;

    static ComptimeValue fromInt(int64_t v) {
        ComptimeValue val;
        val.kind = ComptimeKind::Int;
        val.intVal = v;
        return val;
    }

    static ComptimeValue fromFloat(double v) {
        ComptimeValue val;
        val.kind = ComptimeKind::Float;
        val.floatVal = v;
        return val;
    }

    static ComptimeValue fromBool(bool v) {
        ComptimeValue val;
        val.kind = ComptimeKind::Bool;
        val.boolVal = v;
        return val;
    }

    static ComptimeValue fromString(std::string v) {
        ComptimeValue val;
        val.kind = ComptimeKind::String;
        val.stringVal = std::move(v);
        return val;
    }

    static ComptimeValue fromChar(char v) {
        ComptimeValue val;
        val.kind = ComptimeKind::Char;
        val.charVal = v;
        return val;
    }

    static ComptimeValue fromArray(std::vector<ComptimeValue> v) {
        ComptimeValue val;
        val.kind = ComptimeKind::Array;
        val.arrayVal = std::move(v);
        return val;
    }

    static ComptimeValue nullVal() {
        ComptimeValue val;
        val.kind = ComptimeKind::Null;
        return val;
    }

    static ComptimeValue voidVal() {
        ComptimeValue val;
        val.kind = ComptimeKind::Void;
        return val;
    }

    [[nodiscard]] std::string toString() const;
    [[nodiscard]] bool isTruthy() const;
    [[nodiscard]] std::unique_ptr<ast::ExpressionNode> toLiteralAST(diagnostics::SourceLocation loc) const;
};

class ComptimeEvaluator {
public:
    explicit ComptimeEvaluator(diagnostics::DiagnosticEngine* diagnostics = nullptr);

    // Register pure compile-time function
    void registerFunction(const ast::FunctionDecl* fn);

    // Evaluate an expression into a compile-time value
    ComptimeValue evaluate(const ast::ExpressionNode& expr);

    // Evaluate and replace a comptime expression with a constant literal AST node
    std::unique_ptr<ast::ExpressionNode> foldToLiteral(const ast::ExpressionNode& expr);

    // Scan an entire Program, registering comptime functions and folding comptime blocks/expressions
    void processProgram(ast::Program& program);

    [[nodiscard]] bool hasError() const noexcept { return m_hasError; }
    [[nodiscard]] const std::string& getLastError() const noexcept { return m_lastError; }

private:
    ComptimeValue evaluateBinary(const ast::BinaryExpr& expr);
    ComptimeValue evaluateUnary(const ast::UnaryExpr& expr);
    ComptimeValue evaluateCall(const ast::CallExpr& expr);
    ComptimeValue evaluateStmt(const ast::StatementNode& stmt, bool& returned);
    ComptimeValue evaluateBlock(const ast::BlockStmt& block, bool& returned);

    void pushScope();
    void popScope();
    void setVar(const std::string& name, ComptimeValue val);
    bool getVar(const std::string& name, ComptimeValue& outVal) const;

    void reportError(std::string_view code, std::string message, const diagnostics::SourceLocation& loc);

    diagnostics::DiagnosticEngine* m_diagnostics{nullptr};
    std::vector<std::unordered_map<std::string, ComptimeValue>> m_scopes;
    std::unordered_map<std::string, const ast::FunctionDecl*> m_functions;

    bool m_hasError{false};
    std::string m_lastError;
    size_t m_stepCount{0};
    static constexpr size_t MAX_EVAL_STEPS = 200000;
};

} // namespace sp::sema
