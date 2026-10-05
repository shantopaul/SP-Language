#pragma once

#include "sp/ast/ASTNode.h"
#include "sp/lexer/TokenType.h"
#include <string>
#include <vector>
#include <memory>
#include <utility>

namespace sp::ast {

class ExpressionNode : public ASTNode {
public:
    using ASTNode::ASTNode;
};

using ExprPtr = std::unique_ptr<ExpressionNode>;

enum class LiteralKind {
    Integer,
    Float,
    String,
    Char,
    Boolean,
    Null
};

class LiteralExpr : public ExpressionNode {
public:
    LiteralExpr(LiteralKind kind, std::string value, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::LiteralExpr, std::move(loc)),
          m_literalKind(kind), m_value(std::move(value)) {}

    [[nodiscard]] LiteralKind getLiteralKind() const noexcept { return m_literalKind; }
    [[nodiscard]] const std::string& getValue() const noexcept { return m_value; }

    void accept(ASTVisitor& visitor) const override;

private:
    LiteralKind m_literalKind;
    std::string m_value;
};

class IdentifierExpr : public ExpressionNode {
public:
    IdentifierExpr(std::string name, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::IdentifierExpr, std::move(loc)),
          m_name(std::move(name)) {}

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_name;
};

class BinaryExpr : public ExpressionNode {
public:
    BinaryExpr(ExprPtr left, lexer::TokenType op, ExprPtr right, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::BinaryExpr, std::move(loc)),
          m_left(std::move(left)), m_op(op), m_right(std::move(right)) {}

    [[nodiscard]] const ExpressionNode& getLeft() const noexcept { return *m_left; }
    [[nodiscard]] lexer::TokenType getOp() const noexcept { return m_op; }
    [[nodiscard]] const ExpressionNode& getRight() const noexcept { return *m_right; }

    [[nodiscard]] ExprPtr& getLeftPtr() noexcept { return m_left; }
    [[nodiscard]] ExprPtr& getRightPtr() noexcept { return m_right; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_left;
    lexer::TokenType m_op;
    ExprPtr m_right;
};

class UnaryExpr : public ExpressionNode {
public:
    UnaryExpr(lexer::TokenType op, ExprPtr operand, bool isPrefix, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::UnaryExpr, std::move(loc)),
          m_op(op), m_operand(std::move(operand)), m_isPrefix(isPrefix) {}

    [[nodiscard]] lexer::TokenType getOp() const noexcept { return m_op; }
    [[nodiscard]] const ExpressionNode& getOperand() const noexcept { return *m_operand; }
    [[nodiscard]] bool isPrefix() const noexcept { return m_isPrefix; }

    [[nodiscard]] ExprPtr& getOperandPtr() noexcept { return m_operand; }

    void accept(ASTVisitor& visitor) const override;

private:
    lexer::TokenType m_op;
    ExprPtr m_operand;
    bool m_isPrefix;
};

class CallExpr : public ExpressionNode {
public:
    CallExpr(ExprPtr callee, std::vector<std::string> typeArgs, std::vector<ExprPtr> args, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::CallExpr, std::move(loc)),
          m_callee(std::move(callee)), m_typeArgs(std::move(typeArgs)), m_args(std::move(args)) {}

    CallExpr(ExprPtr callee, std::vector<ExprPtr> args, diagnostics::SourceLocation loc)
        : CallExpr(std::move(callee), {}, std::move(args), std::move(loc)) {}

    [[nodiscard]] const ExpressionNode& getCallee() const noexcept { return *m_callee; }
    [[nodiscard]] const std::vector<std::string>& getTypeArgs() const noexcept { return m_typeArgs; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeArgs.empty(); }
    [[nodiscard]] const std::vector<ExprPtr>& getArgs() const noexcept { return m_args; }

    [[nodiscard]] ExprPtr& getCalleePtr() noexcept { return m_callee; }
    [[nodiscard]] std::vector<ExprPtr>& getArgs() noexcept { return m_args; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_callee;
    std::vector<std::string> m_typeArgs;
    std::vector<ExprPtr> m_args;
};

class MemberExpr : public ExpressionNode {
public:
    MemberExpr(ExprPtr object, std::string member, bool isSafeNav, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::MemberExpr, std::move(loc)),
          m_object(std::move(object)), m_member(std::move(member)), m_isSafeNav(isSafeNav) {}

    [[nodiscard]] const ExpressionNode& getObject() const noexcept { return *m_object; }
    [[nodiscard]] const std::string& getMember() const noexcept { return m_member; }
    [[nodiscard]] bool isSafeNav() const noexcept { return m_isSafeNav; }

    [[nodiscard]] ExprPtr& getObjectPtr() noexcept { return m_object; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_object;
    std::string m_member;
    bool m_isSafeNav;
};

class IndexExpr : public ExpressionNode {
public:
    IndexExpr(ExprPtr object, ExprPtr index, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::IndexExpr, std::move(loc)),
          m_object(std::move(object)), m_index(std::move(index)) {}

    [[nodiscard]] const ExpressionNode& getObject() const noexcept { return *m_object; }
    [[nodiscard]] const ExpressionNode& getIndex() const noexcept { return *m_index; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_object;
    ExprPtr m_index;
};

class ArrayExpr : public ExpressionNode {
public:
    ArrayExpr(std::vector<ExprPtr> elements, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::ArrayExpr, std::move(loc)),
          m_elements(std::move(elements)) {}

    [[nodiscard]] const std::vector<ExprPtr>& getElements() const noexcept { return m_elements; }
    [[nodiscard]] std::vector<ExprPtr>& getElements() noexcept { return m_elements; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::vector<ExprPtr> m_elements;
};

class MapExpr : public ExpressionNode {
public:
    using Entry = std::pair<ExprPtr, ExprPtr>;

    MapExpr(std::vector<Entry> entries, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::MapExpr, std::move(loc)),
          m_entries(std::move(entries)) {}

    [[nodiscard]] const std::vector<Entry>& getEntries() const noexcept { return m_entries; }
    [[nodiscard]] std::vector<Entry>& getEntries() noexcept { return m_entries; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::vector<Entry> m_entries;
};

class LambdaExpr : public ExpressionNode {
public:
    struct Param {
        std::string name;
        std::string type;
    };

    LambdaExpr(std::vector<Param> params, std::string returnType, ASTNodePtr body, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::LambdaExpr, std::move(loc)),
          m_params(std::move(params)), m_returnType(std::move(returnType)), m_body(std::move(body)) {}

    [[nodiscard]] const std::vector<Param>& getParams() const noexcept { return m_params; }
    [[nodiscard]] const std::string& getReturnType() const noexcept { return m_returnType; }
    [[nodiscard]] const ASTNode& getBody() const noexcept { return *m_body; }
    [[nodiscard]] ASTNodePtr& getBodyPtr() noexcept { return m_body; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::vector<Param> m_params;
    std::string m_returnType;
    ASTNodePtr m_body;
};

class TernaryExpr : public ExpressionNode {
public:
    TernaryExpr(ExprPtr condition, ExprPtr thenExpr, ExprPtr elseExpr, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::TernaryExpr, std::move(loc)),
          m_condition(std::move(condition)), m_thenExpr(std::move(thenExpr)), m_elseExpr(std::move(elseExpr)) {}

    [[nodiscard]] const ExpressionNode& getCondition() const noexcept { return *m_condition; }
    [[nodiscard]] const ExpressionNode& getThenExpr() const noexcept { return *m_thenExpr; }
    [[nodiscard]] const ExpressionNode& getElseExpr() const noexcept { return *m_elseExpr; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_condition;
    ExprPtr m_thenExpr;
    ExprPtr m_elseExpr;
};

class AssignExpr : public ExpressionNode {
public:
    AssignExpr(ExprPtr target, lexer::TokenType op, ExprPtr value, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::AssignExpr, std::move(loc)),
          m_target(std::move(target)), m_op(op), m_value(std::move(value)) {}

    [[nodiscard]] const ExpressionNode& getTarget() const noexcept { return *m_target; }
    [[nodiscard]] lexer::TokenType getOp() const noexcept { return m_op; }
    [[nodiscard]] const ExpressionNode& getValue() const noexcept { return *m_value; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_target;
    lexer::TokenType m_op;
    ExprPtr m_value;
};

class NewExpr : public ExpressionNode {
public:
    NewExpr(std::string typeName, std::vector<std::string> typeArgs, std::vector<ExprPtr> args, diagnostics::SourceLocation loc)
        : ExpressionNode(ASTNodeKind::NewExpr, std::move(loc)),
          m_typeName(std::move(typeName)), m_typeArgs(std::move(typeArgs)), m_args(std::move(args)) {}

    [[nodiscard]] const std::string& getTypeName() const noexcept { return m_typeName; }
    [[nodiscard]] const std::vector<std::string>& getTypeArgs() const noexcept { return m_typeArgs; }
    [[nodiscard]] bool isGeneric() const noexcept { return !m_typeArgs.empty(); }
    [[nodiscard]] const std::vector<ExprPtr>& getArgs() const noexcept { return m_args; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_typeName;
    std::vector<std::string> m_typeArgs;
    std::vector<ExprPtr> m_args;
};

} // namespace sp::ast
