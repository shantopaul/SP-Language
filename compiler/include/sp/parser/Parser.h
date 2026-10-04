#pragma once

#include "sp/ast/ASTNode.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/ast/Declaration.h"
#include "sp/lexer/Token.h"
#include "sp/parser/Precedence.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <initializer_list>

namespace sp::parser {

class Parser {
public:
    Parser(const std::vector<lexer::Token>& tokens, std::string filename = "<anonymous>", diagnostics::DiagnosticEngine* diagnostics = nullptr);

    /// Parses the entire token stream into a Program AST node.
    std::unique_ptr<ast::Program> parseProgram();

    /// Parses a single expression (useful for REPL or tests).
    ast::ExprPtr parseExpression(Precedence precedence = Precedence::Assignment);

    /// Parses a single statement.
    ast::StmtPtr parseStatement();

    /// Parses a top-level declaration.
    ast::ASTNodePtr parseDeclaration();

    [[nodiscard]] bool hasErrors() const noexcept {
        return m_diagnostics && m_diagnostics->hasErrors();
    }

private:
    const std::vector<lexer::Token>& m_tokens;
    size_t m_current{0};
    std::string m_filename;
    diagnostics::DiagnosticEngine* m_diagnostics{nullptr};

    // Navigation & Inspection
    [[nodiscard]] bool isAtEnd() const noexcept;
    const lexer::Token& peek() const noexcept;
    const lexer::Token& peekNext() const noexcept;
    const lexer::Token& previous() const noexcept;
    const lexer::Token& advance() noexcept;
    [[nodiscard]] bool check(lexer::TokenType type) const noexcept;
    [[nodiscard]] bool checkNext(lexer::TokenType type) const noexcept;
    bool match(lexer::TokenType type) noexcept;
    bool matchAny(std::initializer_list<lexer::TokenType> types) noexcept;
    const lexer::Token& consume(lexer::TokenType type, std::string_view message, std::string_view hint = "");

    void errorAt(const lexer::Token& token, std::string_view message, std::string_view hint = "");
    void synchronize();

    // Pratt Expression Parsing
    ast::ExprPtr parsePrefix();
    ast::ExprPtr parseInfix(ast::ExprPtr left);
    static Precedence getPrecedence(lexer::TokenType type) noexcept;

    ast::ExprPtr parseGroupedOrLambda();
    ast::ExprPtr parseArrayLiteral();
    ast::ExprPtr parseMapLiteral();
    ast::ExprPtr parseNewExpr();

    // Statement Parsing
    ast::StmtPtr parseVarDecl(bool isConst = false, bool isVar = false);
    ast::StmtPtr parseIfStatement();
    ast::StmtPtr parseWhileStatement();
    ast::StmtPtr parseDoWhileStatement();
    ast::StmtPtr parseForStatement();
    ast::StmtPtr parseSwitchStatement();
    ast::StmtPtr parseMatchStatement();
    ast::StmtPtr parseReturnStatement();
    ast::StmtPtr parseBreakStatement();
    ast::StmtPtr parseContinueStatement();
    ast::StmtPtr parseTryCatchStatement();
    ast::StmtPtr parseThrowStatement();
    std::unique_ptr<ast::BlockStmt> parseBlock();
    ast::StmtPtr parseExprStatement();

    // Declarations & Compatibility
    ast::ASTNodePtr parseModuleDecl();
    ast::ASTNodePtr parseImportDecl();
    ast::ASTNodePtr parseFunctionDecl(std::string visibility = "", bool isStatic = false, bool isOverride = false, bool isAbstract = false);
    ast::ASTNodePtr parseClassDecl(std::string visibility = "", bool isAbstract = false);
    ast::ASTNodePtr parseInterfaceDecl(std::string visibility = "");
    ast::ASTNodePtr parseStructDecl(std::string visibility = "");
    ast::ASTNodePtr parseEnumDecl(std::string visibility = "");

    std::string parseType();
    std::vector<ast::Parameter> parseParameters();

    static bool isTypeKeyword(lexer::TokenType type) noexcept;
};

} // namespace sp::parser
