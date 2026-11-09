#pragma once

#include "sp/ast/ASTNode.h"
#include "sp/ast/Expression.h"
#include <string>
#include <vector>
#include <memory>

namespace sp::ast {

class StatementNode : public ASTNode {
public:
    using ASTNode::ASTNode;
};

using StmtPtr = std::unique_ptr<StatementNode>;

class BlockStmt : public StatementNode {
public:
    BlockStmt(std::vector<StmtPtr> statements, diagnostics::SourceLocation loc, bool isUnsafe = false)
        : StatementNode(ASTNodeKind::BlockStmt, std::move(loc)),
          m_statements(std::move(statements)),
          m_isUnsafe(isUnsafe) {}

    [[nodiscard]] const std::vector<StmtPtr>& getStatements() const noexcept { return m_statements; }
    [[nodiscard]] std::vector<StmtPtr>& getStatements() noexcept { return m_statements; }
    [[nodiscard]] bool isUnsafe() const noexcept { return m_isUnsafe; }
    void setUnsafe(bool unsafe) noexcept { m_isUnsafe = unsafe; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::vector<StmtPtr> m_statements;
    bool m_isUnsafe{false};
};

class ExprStmt : public StatementNode {
public:
    ExprStmt(ExprPtr expr, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ExprStmt, std::move(loc)),
          m_expr(std::move(expr)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasExpr() const noexcept { return m_expr != nullptr; }
    [[nodiscard]] const ExpressionNode& getExpr() const noexcept { return *m_expr; }
    [[nodiscard]] ExprPtr& getExprPtr() noexcept { return m_expr; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_expr;
};

class VarDeclStmt : public StatementNode {
public:
    VarDeclStmt(std::string type, std::string name, ExprPtr init, bool isConst, bool isVar, diagnostics::SourceLocation loc, std::string visibility = "public", bool isStatic = false, bool isWeak = false)
        : StatementNode(ASTNodeKind::VarDeclStmt, std::move(loc)),
          m_type(std::move(type)), m_name(std::move(name)), m_init(std::move(init)),
          m_isConst(isConst), m_isVar(isVar), m_visibility(std::move(visibility)), m_isStatic(isStatic), m_isWeak(isWeak) {}

    [[nodiscard]] const std::string& getType() const noexcept { return m_type; }
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] bool hasInitializer() const noexcept { return m_init != nullptr; }
    [[nodiscard]] const ExpressionNode* getInitializer() const noexcept { return m_init.get(); }
    [[nodiscard]] ExprPtr& getInitializerPtr() noexcept { return m_init; }
    [[nodiscard]] bool isConst() const noexcept { return m_isConst; }
    [[nodiscard]] bool isVar() const noexcept { return m_isVar; }
    [[nodiscard]] const std::string& getVisibility() const noexcept { return m_visibility; }
    [[nodiscard]] bool isStatic() const noexcept { return m_isStatic; }
    [[nodiscard]] bool isWeak() const noexcept { return m_isWeak; }
    void setWeak(bool weak) noexcept { m_isWeak = weak; }

    [[nodiscard]] const std::vector<Annotation>& getAnnotations() const noexcept { return m_annotations; }
    void addAnnotation(Annotation annot) { m_annotations.push_back(std::move(annot)); }
    void setAnnotations(std::vector<Annotation> annots) { m_annotations = std::move(annots); }
    [[nodiscard]] bool hasAnnotation(const std::string& name) const noexcept {
        for (const auto& ann : m_annotations) {
            if (ann.name == name) return true;
        }
        return false;
    }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_type;
    std::string m_name;
    ExprPtr m_init;
    bool m_isConst;
    bool m_isVar;
    std::string m_visibility;
    bool m_isStatic{false};
    bool m_isWeak{false};
    std::vector<Annotation> m_annotations;
};

class IfStmt : public StatementNode {
public:
    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::IfStmt, std::move(loc)),
          m_condition(std::move(condition)), m_thenBranch(std::move(thenBranch)), m_elseBranch(std::move(elseBranch)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasCondition() const noexcept { return m_condition != nullptr; }
    [[nodiscard]] bool hasThenBranch() const noexcept { return m_thenBranch != nullptr; }
    [[nodiscard]] const ExpressionNode& getCondition() const noexcept { return *m_condition; }
    [[nodiscard]] const StatementNode& getThenBranch() const noexcept { return *m_thenBranch; }
    [[nodiscard]] bool hasElseBranch() const noexcept { return m_elseBranch != nullptr; }
    [[nodiscard]] const StatementNode* getElseBranch() const noexcept { return m_elseBranch.get(); }

    [[nodiscard]] ExprPtr& getConditionPtr() noexcept { return m_condition; }
    [[nodiscard]] StmtPtr& getThenBranchPtr() noexcept { return m_thenBranch; }
    [[nodiscard]] StmtPtr& getElseBranchPtr() noexcept { return m_elseBranch; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_condition;
    StmtPtr m_thenBranch;
    StmtPtr m_elseBranch;
};

class WhileStmt : public StatementNode {
public:
    WhileStmt(ExprPtr condition, StmtPtr body, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::WhileStmt, std::move(loc)),
          m_condition(std::move(condition)), m_body(std::move(body)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasCondition() const noexcept { return m_condition != nullptr; }
    [[nodiscard]] bool hasBody() const noexcept { return m_body != nullptr; }
    [[nodiscard]] const ExpressionNode& getCondition() const noexcept { return *m_condition; }
    [[nodiscard]] const StatementNode& getBody() const noexcept { return *m_body; }

    [[nodiscard]] ExprPtr& getConditionPtr() noexcept { return m_condition; }
    [[nodiscard]] StmtPtr& getBodyPtr() noexcept { return m_body; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_condition;
    StmtPtr m_body;
};

class DoWhileStmt : public StatementNode {
public:
    DoWhileStmt(StmtPtr body, ExprPtr condition, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::DoWhileStmt, std::move(loc)),
          m_body(std::move(body)), m_condition(std::move(condition)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasBody() const noexcept { return m_body != nullptr; }
    [[nodiscard]] bool hasCondition() const noexcept { return m_condition != nullptr; }
    [[nodiscard]] const StatementNode& getBody() const noexcept { return *m_body; }
    [[nodiscard]] const ExpressionNode& getCondition() const noexcept { return *m_condition; }

    [[nodiscard]] StmtPtr& getBodyPtr() noexcept { return m_body; }
    [[nodiscard]] ExprPtr& getConditionPtr() noexcept { return m_condition; }

    void accept(ASTVisitor& visitor) const override;

private:
    StmtPtr m_body;
    ExprPtr m_condition;
};

class ForStmt : public StatementNode {
public:
    ForStmt(StmtPtr init, ExprPtr condition, ExprPtr increment, StmtPtr body, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ForStmt, std::move(loc)),
          m_init(std::move(init)), m_condition(std::move(condition)), m_increment(std::move(increment)), m_body(std::move(body)) {}

    [[nodiscard]] bool hasInit() const noexcept { return m_init != nullptr; }
    [[nodiscard]] const StatementNode* getInit() const noexcept { return m_init.get(); }
    [[nodiscard]] bool hasCondition() const noexcept { return m_condition != nullptr; }
    [[nodiscard]] const ExpressionNode* getCondition() const noexcept { return m_condition.get(); }
    [[nodiscard]] bool hasIncrement() const noexcept { return m_increment != nullptr; }
    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasBody() const noexcept { return m_body != nullptr; }
    [[nodiscard]] const ExpressionNode* getIncrement() const noexcept { return m_increment.get(); }
    [[nodiscard]] const StatementNode& getBody() const noexcept { return *m_body; }

    [[nodiscard]] StmtPtr& getInitPtr() noexcept { return m_init; }
    [[nodiscard]] ExprPtr& getConditionPtr() noexcept { return m_condition; }
    [[nodiscard]] ExprPtr& getIncrementPtr() noexcept { return m_increment; }
    [[nodiscard]] StmtPtr& getBodyPtr() noexcept { return m_body; }

    void accept(ASTVisitor& visitor) const override;

private:
    StmtPtr m_init;
    ExprPtr m_condition;
    ExprPtr m_increment;
    StmtPtr m_body;
};

class ForInStmt : public StatementNode {
public:
    ForInStmt(std::string varName, std::string type, ExprPtr iterable, StmtPtr body, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ForInStmt, std::move(loc)),
          m_varName(std::move(varName)), m_type(std::move(type)), m_iterable(std::move(iterable)), m_body(std::move(body)) {}

    [[nodiscard]] const std::string& getVarName() const noexcept { return m_varName; }
    [[nodiscard]] const std::string& getType() const noexcept { return m_type; }
    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasIterable() const noexcept { return m_iterable != nullptr; }
    [[nodiscard]] bool hasBody() const noexcept { return m_body != nullptr; }
    [[nodiscard]] const ExpressionNode& getIterable() const noexcept { return *m_iterable; }
    [[nodiscard]] const StatementNode& getBody() const noexcept { return *m_body; }

    [[nodiscard]] ExprPtr& getIterablePtr() noexcept { return m_iterable; }
    [[nodiscard]] StmtPtr& getBodyPtr() noexcept { return m_body; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::string m_varName;
    std::string m_type;
    ExprPtr m_iterable;
    StmtPtr m_body;
};

class SwitchCase {
public:
    SwitchCase(ExprPtr value, std::vector<StmtPtr> stmts)
        : m_value(std::move(value)), m_stmts(std::move(stmts)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasValue() const noexcept { return m_value != nullptr; }
    [[nodiscard]] const ExpressionNode& getValue() const noexcept { return *m_value; }
    [[nodiscard]] const std::vector<StmtPtr>& getStatements() const noexcept { return m_stmts; }
    [[nodiscard]] std::vector<StmtPtr>& getStatements() noexcept { return m_stmts; }

private:
    ExprPtr m_value;
    std::vector<StmtPtr> m_stmts;
};

class SwitchStmt : public StatementNode {
public:
    SwitchStmt(ExprPtr subject, std::vector<SwitchCase> cases, std::vector<StmtPtr> defaultStmts, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::SwitchStmt, std::move(loc)),
          m_subject(std::move(subject)), m_cases(std::move(cases)), m_defaultStmts(std::move(defaultStmts)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasSubject() const noexcept { return m_subject != nullptr; }
    [[nodiscard]] const ExpressionNode& getSubject() const noexcept { return *m_subject; }
    [[nodiscard]] const std::vector<SwitchCase>& getCases() const noexcept { return m_cases; }
    [[nodiscard]] const std::vector<StmtPtr>& getDefaultStatements() const noexcept { return m_defaultStmts; }

    [[nodiscard]] ExprPtr& getSubjectPtr() noexcept { return m_subject; }
    [[nodiscard]] std::vector<SwitchCase>& getCases() noexcept { return m_cases; }
    [[nodiscard]] std::vector<StmtPtr>& getDefaultStatements() noexcept { return m_defaultStmts; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_subject;
    std::vector<SwitchCase> m_cases;
    std::vector<StmtPtr> m_defaultStmts;
};

class MatchArm {
public:
    MatchArm(ExprPtr pattern, ASTNodePtr body)
        : m_pattern(std::move(pattern)), m_body(std::move(body)) {}

    [[nodiscard]] bool isWildcard() const noexcept { return m_pattern == nullptr; }
    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasBody() const noexcept { return m_body != nullptr; }
    [[nodiscard]] const ExpressionNode* getPattern() const noexcept { return m_pattern.get(); }
    [[nodiscard]] const ASTNode& getBody() const noexcept { return *m_body; }

    [[nodiscard]] ExprPtr& getPatternPtr() noexcept { return m_pattern; }
    [[nodiscard]] ASTNodePtr& getBodyPtr() noexcept { return m_body; }

private:
    ExprPtr m_pattern;
    ASTNodePtr m_body;
};

class MatchStmt : public StatementNode {
public:
    MatchStmt(ExprPtr subject, std::vector<MatchArm> arms, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::MatchStmt, std::move(loc)),
          m_subject(std::move(subject)), m_arms(std::move(arms)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasSubject() const noexcept { return m_subject != nullptr; }
    [[nodiscard]] const ExpressionNode& getSubject() const noexcept { return *m_subject; }
    [[nodiscard]] const std::vector<MatchArm>& getArms() const noexcept { return m_arms; }

    [[nodiscard]] ExprPtr& getSubjectPtr() noexcept { return m_subject; }
    [[nodiscard]] std::vector<MatchArm>& getArms() noexcept { return m_arms; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_subject;
    std::vector<MatchArm> m_arms;
};

class ReturnStmt : public StatementNode {
public:
    ReturnStmt(ExprPtr value, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ReturnStmt, std::move(loc)),
          m_value(std::move(value)) {}

    [[nodiscard]] bool hasValue() const noexcept { return m_value != nullptr; }
    [[nodiscard]] const ExpressionNode* getValue() const noexcept { return m_value.get(); }
    [[nodiscard]] ExprPtr& getValuePtr() noexcept { return m_value; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_value;
};

class BreakStmt : public StatementNode {
public:
    explicit BreakStmt(diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::BreakStmt, std::move(loc)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ContinueStmt : public StatementNode {
public:
    explicit ContinueStmt(diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ContinueStmt, std::move(loc)) {}

    void accept(ASTVisitor& visitor) const override;
};

class CatchClause {
public:
    CatchClause(std::string type, std::string name, std::unique_ptr<BlockStmt> body)
        : m_type(std::move(type)), m_name(std::move(name)), m_body(std::move(body)) {}

    [[nodiscard]] const std::string& getType() const noexcept { return m_type; }
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const BlockStmt& getBody() const noexcept { return *m_body; }
    [[nodiscard]] std::unique_ptr<BlockStmt>& getBodyPtr() noexcept { return m_body; }

private:
    std::string m_type;
    std::string m_name;
    std::unique_ptr<BlockStmt> m_body;
};

class TryCatchStmt : public StatementNode {
public:
    TryCatchStmt(std::unique_ptr<BlockStmt> tryBlock, std::vector<CatchClause> catches, std::unique_ptr<BlockStmt> finallyBlock, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::TryCatchStmt, std::move(loc)),
          m_tryBlock(std::move(tryBlock)), m_catches(std::move(catches)), m_finallyBlock(std::move(finallyBlock)) {}

    [[nodiscard]] const BlockStmt& getTryBlock() const noexcept { return *m_tryBlock; }
    [[nodiscard]] const std::vector<CatchClause>& getCatches() const noexcept { return m_catches; }
    [[nodiscard]] bool hasFinally() const noexcept { return m_finallyBlock != nullptr; }
    [[nodiscard]] const BlockStmt* getFinallyBlock() const noexcept { return m_finallyBlock.get(); }

    [[nodiscard]] std::unique_ptr<BlockStmt>& getTryBlockPtr() noexcept { return m_tryBlock; }
    [[nodiscard]] std::vector<CatchClause>& getCatches() noexcept { return m_catches; }
    [[nodiscard]] std::unique_ptr<BlockStmt>& getFinallyBlockPtr() noexcept { return m_finallyBlock; }

    void accept(ASTVisitor& visitor) const override;

private:
    std::unique_ptr<BlockStmt> m_tryBlock;
    std::vector<CatchClause> m_catches;
    std::unique_ptr<BlockStmt> m_finallyBlock;
};

class ThrowStmt : public StatementNode {
public:
    ThrowStmt(ExprPtr expr, diagnostics::SourceLocation loc)
        : StatementNode(ASTNodeKind::ThrowStmt, std::move(loc)),
          m_expr(std::move(expr)) {}

    // Phase 4 slice B: the parser's error recovery can build this node
    // without a child (e.g. `if ;`); hasX() lets a walker skip the null.
    [[nodiscard]] bool hasExpr() const noexcept { return m_expr != nullptr; }
    [[nodiscard]] const ExpressionNode& getExpr() const noexcept { return *m_expr; }
    [[nodiscard]] ExprPtr& getExprPtr() noexcept { return m_expr; }

    void accept(ASTVisitor& visitor) const override;

private:
    ExprPtr m_expr;
};

} // namespace sp::ast
