#include "sp/parser/Parser.h"
#include <iostream>

namespace sp::parser {

Parser::Parser(const std::vector<lexer::Token>& tokens, std::string filename, diagnostics::DiagnosticEngine* diagnostics)
    : m_tokens(tokens), m_filename(std::move(filename)), m_diagnostics(diagnostics) {}

bool Parser::isAtEnd() const noexcept {
    return peek().is(lexer::TokenType::EndOfFile);
}

const lexer::Token& Parser::peek() const noexcept {
    if (m_current >= m_tokens.size()) {
        static const lexer::Token eofToken(lexer::TokenType::EndOfFile, "", diagnostics::SourceLocation("<eof>", 0, 0));
        return eofToken;
    }
    return m_tokens[m_current];
}

const lexer::Token& Parser::peekNext() const noexcept {
    if (m_current + 1 >= m_tokens.size()) {
        return peek();
    }
    return m_tokens[m_current + 1];
}

const lexer::Token& Parser::previous() const noexcept {
    if (m_current == 0) return m_tokens[0];
    return m_tokens[m_current - 1];
}

const lexer::Token& Parser::advance() noexcept {
    if (!isAtEnd()) {
        m_current++;
    }
    return previous();
}

bool Parser::check(lexer::TokenType type) const noexcept {
    if (isAtEnd()) return false;
    return peek().is(type);
}

bool Parser::checkNext(lexer::TokenType type) const noexcept {
    if (isAtEnd()) return false;
    return peekNext().is(type);
}

bool Parser::match(lexer::TokenType type) noexcept {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchAny(std::initializer_list<lexer::TokenType> types) noexcept {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

const lexer::Token& Parser::consume(lexer::TokenType type, std::string_view message, std::string_view hint) {
    if (check(type)) {
        return advance();
    }
    errorAt(peek(), message, hint);
    static const lexer::Token errorTok(lexer::TokenType::Invalid, "", diagnostics::SourceLocation("<err>", 0, 0));
    return errorTok;
}

void Parser::errorAt(const lexer::Token& token, std::string_view message, std::string_view hint) {
    if (m_diagnostics) {
        m_diagnostics->reportError("E1000", message, token.location, hint);
    }
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().is(lexer::TokenType::Semicolon)) return;

        switch (peek().type) {
            case lexer::TokenType::KeywordClass:
            case lexer::TokenType::KeywordFn:
            case lexer::TokenType::KeywordVar:
            case lexer::TokenType::KeywordLet:
            case lexer::TokenType::KeywordConst:
            case lexer::TokenType::KeywordFor:
            case lexer::TokenType::KeywordIf:
            case lexer::TokenType::KeywordWhile:
            case lexer::TokenType::KeywordReturn:
            case lexer::TokenType::KeywordInterface:
            case lexer::TokenType::KeywordStruct:
            case lexer::TokenType::KeywordEnum:
            case lexer::TokenType::KeywordModule:
            case lexer::TokenType::KeywordImport:
                return;
            default:
                break;
        }

        advance();
    }
}

bool Parser::isGenericCall(size_t startPos) const noexcept {
    if (startPos >= m_tokens.size() || !m_tokens[startPos].is(lexer::TokenType::Less)) {
        return false;
    }
    size_t pos = startPos + 1;
    int depth = 1;
    while (pos < m_tokens.size()) {
        const auto& tok = m_tokens[pos];
        if (tok.is(lexer::TokenType::Less)) {
            depth++;
        } else if (tok.is(lexer::TokenType::Greater)) {
            depth--;
            if (depth == 0) {
                return (pos + 1 < m_tokens.size() && m_tokens[pos + 1].is(lexer::TokenType::OpenParen));
            }
        } else if (tok.is(lexer::TokenType::Identifier) ||
                   tok.is(lexer::TokenType::Comma) ||
                   tok.is(lexer::TokenType::OpenBracket) ||
                   tok.is(lexer::TokenType::CloseBracket) ||
                   tok.is(lexer::TokenType::Question) ||
                   isTypeKeyword(tok.type)) {
            // Valid token within type argument list
        } else {
            return false;
        }
        pos++;
    }
    return false;
}

Precedence Parser::getPrecedence(lexer::TokenType type) const noexcept {
    switch (type) {
        case lexer::TokenType::Equal:
        case lexer::TokenType::PlusEqual:
        case lexer::TokenType::MinusEqual:
        case lexer::TokenType::StarEqual:
        case lexer::TokenType::SlashEqual:
        case lexer::TokenType::PercentEqual:
            return Precedence::Assignment;

        case lexer::TokenType::Question:
            return Precedence::Ternary;

        case lexer::TokenType::QuestionQuestion:
            return Precedence::NullCoalescing;

        case lexer::TokenType::PipePipe:
            return Precedence::LogicalOr;

        case lexer::TokenType::AmpAmp:
            return Precedence::LogicalAnd;

        case lexer::TokenType::Pipe:
            return Precedence::BitwiseOr;

        case lexer::TokenType::Caret:
            return Precedence::BitwiseXor;

        case lexer::TokenType::Amp:
            return Precedence::BitwiseAnd;

        case lexer::TokenType::EqualEqual:
        case lexer::TokenType::BangEqual:
            return Precedence::Equality;

        case lexer::TokenType::Less:
            if (isGenericCall(m_current)) {
                return Precedence::Call;
            }
            return Precedence::Relational;

        case lexer::TokenType::LessEqual:
        case lexer::TokenType::Greater:
        case lexer::TokenType::GreaterEqual:
            return Precedence::Relational;

        case lexer::TokenType::LessLess:
        case lexer::TokenType::GreaterGreater:
            return Precedence::Shift;

        case lexer::TokenType::DotDot:
            return Precedence::Range;

        case lexer::TokenType::Plus:
        case lexer::TokenType::Minus:
            return Precedence::Additive;

        case lexer::TokenType::Star:
        case lexer::TokenType::Slash:
        case lexer::TokenType::Percent:
            return Precedence::Multiplicative;

        case lexer::TokenType::OpenParen:
        case lexer::TokenType::OpenBracket:
        case lexer::TokenType::Dot:
        case lexer::TokenType::QuestionDot:
        case lexer::TokenType::PlusPlus:
        case lexer::TokenType::MinusMinus:
            return Precedence::Call;

        default:
            return Precedence::None;
    }
}

ast::ExprPtr Parser::parseExpression(Precedence precedence) {
    ast::ExprPtr left = parsePrefix();
    if (!left) return nullptr;

    while (precedence <= getPrecedence(peek().type)) {
        left = parseInfix(std::move(left));
        if (!left) break;
    }

    return left;
}

ast::ExprPtr Parser::parsePrefix() {
    const lexer::Token& token = peek();

    // Literals
    if (match(lexer::TokenType::IntegerLiteral)) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, previous().lexeme, previous().location);
    }
    if (match(lexer::TokenType::FloatLiteral)) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Float, previous().lexeme, previous().location);
    }
    if (match(lexer::TokenType::StringLiteral)) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, previous().lexeme, previous().location);
    }
    if (match(lexer::TokenType::CharLiteral)) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Char, previous().lexeme, previous().location);
    }
    if (match(lexer::TokenType::BooleanLiteral)) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Boolean, previous().lexeme, previous().location);
    }
    if (match(lexer::TokenType::KeywordNull)) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Null, "null", previous().location);
    }

    // Identifiers & Special Keywords
    if (match(lexer::TokenType::KeywordThis)) {
        return std::make_unique<ast::IdentifierExpr>("this", previous().location);
    }
    if (match(lexer::TokenType::KeywordSuper)) {
        return std::make_unique<ast::IdentifierExpr>("super", previous().location);
    }
    if (match(lexer::TokenType::KeywordAssert)) {
        return std::make_unique<ast::IdentifierExpr>("assert", previous().location);
    }
    if (check(lexer::TokenType::Identifier) && m_current + 1 < m_tokens.size() && m_tokens[m_current + 1].is(lexer::TokenType::FatArrow)) {
        diagnostics::SourceLocation loc = peek().location;
        std::string paramName = advance().lexeme;
        advance(); // consume '=>'
        ast::ASTNodePtr body;
        if (check(lexer::TokenType::OpenBrace)) {
            body = parseBlock();
        } else {
            body = parseExpression();
        }
        std::vector<ast::LambdaExpr::Param> params = {{paramName, ""}};
        return std::make_unique<ast::LambdaExpr>(std::move(params), "", std::move(body), loc);
    }
    if (match(lexer::TokenType::Identifier)) {
        return std::make_unique<ast::IdentifierExpr>(previous().lexeme, previous().location);
    }

    // Grouped expression or Lambda: (x, y) => expr
    if (check(lexer::TokenType::OpenParen)) {
        return parseGroupedOrLambda();
    }

    // Array Literal: [a, b, c]
    if (match(lexer::TokenType::OpenBracket)) {
        return parseArrayLiteral();
    }

    // Map Literal: { key: value, ... }
    if (check(lexer::TokenType::OpenBrace)) {
        return parseMapLiteral();
    }

    // New Object: new ClassName(...)
    if (match(lexer::TokenType::KeywordNew)) {
        return parseNewExpr();
    }

    // Prefix unary operators (!, -, +, ~, ++, --)
    if (matchAny({lexer::TokenType::Bang, lexer::TokenType::Minus, lexer::TokenType::Plus,
                  lexer::TokenType::Tilde, lexer::TokenType::PlusPlus, lexer::TokenType::MinusMinus})) {
        lexer::TokenType op = previous().type;
        diagnostics::SourceLocation loc = previous().location;
        ast::ExprPtr operand = parseExpression(Precedence::Unary);
        return std::make_unique<ast::UnaryExpr>(op, std::move(operand), true, loc);
    }

    errorAt(token, "Expected expression, found: " + token.lexeme);
    advance();
    return nullptr;
}

ast::ExprPtr Parser::parseInfix(ast::ExprPtr left) {
    // Generic Function Call: callee<T1, T2>(arg1, arg2)
    if (check(lexer::TokenType::Less) && isGenericCall(m_current)) {
        const lexer::Token& opTok = advance();
        diagnostics::SourceLocation loc = opTok.location;
        std::vector<std::string> typeArgs;
        do {
            typeArgs.push_back(parseType());
        } while (match(lexer::TokenType::Comma));
        consume(lexer::TokenType::Greater, "Expected '>' after generic type arguments");
        consume(lexer::TokenType::OpenParen, "Expected '(' after generic type arguments in call");
        std::vector<ast::ExprPtr> args;
        if (!check(lexer::TokenType::CloseParen)) {
            do {
                args.push_back(parseExpression());
            } while (match(lexer::TokenType::Comma));
        }
        consume(lexer::TokenType::CloseParen, "Expected ')' after argument list");
        return std::make_unique<ast::CallExpr>(std::move(left), std::move(typeArgs), std::move(args), loc);
    }

    const lexer::Token& opTok = advance();
    diagnostics::SourceLocation loc = opTok.location;
    Precedence prec = getPrecedence(opTok.type);

    // Postfix ++ and --
    if (opTok.is(lexer::TokenType::PlusPlus) || opTok.is(lexer::TokenType::MinusMinus)) {
        return std::make_unique<ast::UnaryExpr>(opTok.type, std::move(left), false, loc);
    }

    // Function Call: callee(arg1, arg2)
    if (opTok.is(lexer::TokenType::OpenParen)) {
        std::vector<ast::ExprPtr> args;
        if (!check(lexer::TokenType::CloseParen)) {
            do {
                args.push_back(parseExpression());
            } while (match(lexer::TokenType::Comma));
        }
        consume(lexer::TokenType::CloseParen, "Expected ')' after argument list");
        return std::make_unique<ast::CallExpr>(std::move(left), std::move(args), loc);
    }

    // Index Expression: obj[index]
    if (opTok.is(lexer::TokenType::OpenBracket)) {
        ast::ExprPtr index = parseExpression();
        consume(lexer::TokenType::CloseBracket, "Expected ']' after index");
        return std::make_unique<ast::IndexExpr>(std::move(left), std::move(index), loc);
    }

    // Member Access: obj.member or safe navigation obj?.member
    if (opTok.is(lexer::TokenType::Dot) || opTok.is(lexer::TokenType::QuestionDot)) {
        bool isSafeNav = opTok.is(lexer::TokenType::QuestionDot);
        const lexer::Token& memberTok = consume(lexer::TokenType::Identifier, "Expected member identifier after '.'");
        return std::make_unique<ast::MemberExpr>(std::move(left), memberTok.lexeme, isSafeNav, loc);
    }

    // Ternary Operator: condition ? thenExpr : elseExpr
    if (opTok.is(lexer::TokenType::Question)) {
        ast::ExprPtr thenExpr = parseExpression();
        consume(lexer::TokenType::Colon, "Expected ':' in ternary operator");
        ast::ExprPtr elseExpr = parseExpression(Precedence::Ternary);
        return std::make_unique<ast::TernaryExpr>(std::move(left), std::move(thenExpr), std::move(elseExpr), loc);
    }

    // Assignment operators: target = value
    if (opTok.is(lexer::TokenType::Equal) ||
        opTok.is(lexer::TokenType::PlusEqual) ||
        opTok.is(lexer::TokenType::MinusEqual) ||
        opTok.is(lexer::TokenType::StarEqual) ||
        opTok.is(lexer::TokenType::SlashEqual) ||
        opTok.is(lexer::TokenType::PercentEqual)) {
        ast::ExprPtr right = parseExpression(Precedence::Assignment);
        return std::make_unique<ast::AssignExpr>(std::move(left), opTok.type, std::move(right), loc);
    }

    // Standard binary operators (left-associative)
    ast::ExprPtr right = parseExpression(static_cast<Precedence>(static_cast<int>(prec) + 1));
    return std::make_unique<ast::BinaryExpr>(std::move(left), opTok.type, std::move(right), loc);
}

ast::ExprPtr Parser::parseGroupedOrLambda() {
    diagnostics::SourceLocation loc = peek().location;
    advance(); // Consume '('

    // Empty parameters lambda: () => expr
    if (match(lexer::TokenType::CloseParen)) {
        if (match(lexer::TokenType::FatArrow)) {
            ast::ASTNodePtr body;
            if (check(lexer::TokenType::OpenBrace)) {
                body = parseBlock();
            } else {
                body = parseExpression();
            }
            return std::make_unique<ast::LambdaExpr>(std::vector<ast::LambdaExpr::Param>{}, "", std::move(body), loc);
        }
        errorAt(previous(), "Empty parentheses '()' are only allowed in lambda definitions");
        return nullptr;
    }

    // Check if this looks like a parameter list with types for lambda: (int a, int b) =>
    size_t savedPos = m_current;
    bool isLambda = false;
    std::vector<ast::LambdaExpr::Param> params;

    if (check(lexer::TokenType::Identifier) || isTypeKeyword(peek().type)) {
        while (!isAtEnd() && !check(lexer::TokenType::CloseParen)) {
            std::string type;
            if (isTypeKeyword(peek().type)) {
                type = parseType();
            }
            if (match(lexer::TokenType::Identifier)) {
                std::string name = previous().lexeme;
                params.push_back({name, type});
            } else {
                break;
            }
            if (!match(lexer::TokenType::Comma)) break;
        }
        if (match(lexer::TokenType::CloseParen) && check(lexer::TokenType::FatArrow)) {
            isLambda = true;
        }
    }

    if (isLambda) {
        advance(); // consume '=>'
        ast::ASTNodePtr body;
        if (check(lexer::TokenType::OpenBrace)) {
            body = parseBlock();
        } else {
            body = parseExpression();
        }
        return std::make_unique<ast::LambdaExpr>(std::move(params), "", std::move(body), loc);
    }

    // Rewind and parse as grouped expression: ( expr )
    m_current = savedPos;
    ast::ExprPtr expr = parseExpression();
    consume(lexer::TokenType::CloseParen, "Expected ')' after expression");
    return expr;
}

ast::ExprPtr Parser::parseArrayLiteral() {
    diagnostics::SourceLocation loc = previous().location;
    std::vector<ast::ExprPtr> elements;

    if (!check(lexer::TokenType::CloseBracket)) {
        do {
            elements.push_back(parseExpression());
        } while (match(lexer::TokenType::Comma));
    }

    consume(lexer::TokenType::CloseBracket, "Expected ']' at end of array literal");
    return std::make_unique<ast::ArrayExpr>(std::move(elements), loc);
}

ast::ExprPtr Parser::parseMapLiteral() {
    diagnostics::SourceLocation loc = peek().location;
    advance(); // Consume '{'

    std::vector<ast::MapExpr::Entry> entries;

    if (!check(lexer::TokenType::CloseBrace)) {
        do {
            ast::ExprPtr key = parseExpression();
            consume(lexer::TokenType::Colon, "Expected ':' between key and value in map entry");
            ast::ExprPtr value = parseExpression();
            entries.emplace_back(std::move(key), std::move(value));
        } while (match(lexer::TokenType::Comma));
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of map literal");
    return std::make_unique<ast::MapExpr>(std::move(entries), loc);
}

ast::ExprPtr Parser::parseNewExpr() {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& typeTok = consume(lexer::TokenType::Identifier, "Expected class name after 'new'");
    std::string typeName = typeTok.lexeme;

    std::vector<std::string> typeArgs;
    if (match(lexer::TokenType::Less)) {
        do {
            typeArgs.push_back(parseType());
        } while (match(lexer::TokenType::Comma));
        consume(lexer::TokenType::Greater, "Expected '>' after generic type arguments");
    }

    consume(lexer::TokenType::OpenParen, "Expected '(' after class name in new expression");
    std::vector<ast::ExprPtr> args;
    if (!check(lexer::TokenType::CloseParen)) {
        do {
            args.push_back(parseExpression());
        } while (match(lexer::TokenType::Comma));
    }
    consume(lexer::TokenType::CloseParen, "Expected ')' after constructor arguments");

    return std::make_unique<ast::NewExpr>(std::move(typeName), std::move(typeArgs), std::move(args), loc);
}

ast::StmtPtr Parser::parseStatement() {
    if (check(lexer::TokenType::OpenBrace)) {
        return parseBlock();
    }
    if (match(lexer::TokenType::KeywordVar)) {
        return parseVarDecl(false, true);
    }
    if (match(lexer::TokenType::KeywordLet)) {
        return parseVarDecl(false, true);
    }
    if (match(lexer::TokenType::KeywordConst)) {
        return parseVarDecl(true, false);
    }
    if (match(lexer::TokenType::KeywordIf)) {
        return parseIfStatement();
    }
    if (match(lexer::TokenType::KeywordWhile)) {
        return parseWhileStatement();
    }
    if (match(lexer::TokenType::KeywordDo)) {
        return parseDoWhileStatement();
    }
    if (match(lexer::TokenType::KeywordFor)) {
        return parseForStatement();
    }
    if (match(lexer::TokenType::KeywordSwitch)) {
        return parseSwitchStatement();
    }
    if (match(lexer::TokenType::KeywordMatch)) {
        return parseMatchStatement();
    }
    if (match(lexer::TokenType::KeywordReturn)) {
        return parseReturnStatement();
    }
    if (match(lexer::TokenType::KeywordBreak)) {
        return parseBreakStatement();
    }
    if (match(lexer::TokenType::KeywordContinue)) {
        return parseContinueStatement();
    }
    if (match(lexer::TokenType::KeywordTry)) {
        return parseTryCatchStatement();
    }
    if (match(lexer::TokenType::KeywordThrow)) {
        return parseThrowStatement();
    }

    // Check for variable declaration with explicit or user-defined type: int x = 10; or Person p = ...;
    bool couldBeTypeDecl = isTypeKeyword(peek().type) ||
                           (check(lexer::TokenType::Identifier) &&
                            (checkNext(lexer::TokenType::Identifier) ||
                             checkNext(lexer::TokenType::Less) ||
                             checkNext(lexer::TokenType::Question) ||
                             checkNext(lexer::TokenType::OpenBracket)));
    if (couldBeTypeDecl) {
        size_t saved = m_current;
        std::string type = parseType();
        if (check(lexer::TokenType::Identifier) &&
            (checkNext(lexer::TokenType::Equal) || checkNext(lexer::TokenType::Semicolon) || checkNext(lexer::TokenType::Comma))) {
            const lexer::Token& nameTok = advance();
            diagnostics::SourceLocation loc = nameTok.location;
            ast::ExprPtr init = nullptr;
            if (match(lexer::TokenType::Equal)) {
                init = parseExpression();
            }
            consume(lexer::TokenType::Semicolon, "Expected ';' after variable declaration");
            return std::make_unique<ast::VarDeclStmt>(type, nameTok.lexeme, std::move(init), false, false, loc);
        }
        m_current = saved; // Rewind
    }

    return parseExprStatement();
}

ast::StmtPtr Parser::parseVarDecl(bool isConst, bool isVar) {
    diagnostics::SourceLocation loc = previous().location;
    std::string type;

    if (isTypeKeyword(peek().type)) {
        type = parseType();
    } else if (!isVar && !isConst) {
        type = parseType();
    }

    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected variable name in declaration");

    // Support type annotation after colon: let x: int = 10; or const PI: float = 3.14;
    if (match(lexer::TokenType::Colon)) {
        type = parseType();
    }

    ast::ExprPtr init = nullptr;

    if (match(lexer::TokenType::Equal)) {
        init = parseExpression();
    }

    consume(lexer::TokenType::Semicolon, "Expected ';' after variable declaration");
    return std::make_unique<ast::VarDeclStmt>(type, nameTok.lexeme, std::move(init), isConst, isVar, loc);
}

ast::StmtPtr Parser::parseIfStatement() {
    diagnostics::SourceLocation loc = previous().location;
    consume(lexer::TokenType::OpenParen, "Expected '(' after 'if'");
    ast::ExprPtr condition = parseExpression();
    consume(lexer::TokenType::CloseParen, "Expected ')' after if condition");

    ast::StmtPtr thenBranch = parseStatement();
    ast::StmtPtr elseBranch = nullptr;

    if (match(lexer::TokenType::KeywordElse)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<ast::IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), loc);
}

ast::StmtPtr Parser::parseWhileStatement() {
    diagnostics::SourceLocation loc = previous().location;
    consume(lexer::TokenType::OpenParen, "Expected '(' after 'while'");
    ast::ExprPtr condition = parseExpression();
    consume(lexer::TokenType::CloseParen, "Expected ')' after while condition");

    ast::StmtPtr body = parseStatement();
    return std::make_unique<ast::WhileStmt>(std::move(condition), std::move(body), loc);
}

ast::StmtPtr Parser::parseDoWhileStatement() {
    diagnostics::SourceLocation loc = previous().location;
    ast::StmtPtr body = parseStatement();

    consume(lexer::TokenType::KeywordWhile, "Expected 'while' after 'do' body");
    consume(lexer::TokenType::OpenParen, "Expected '(' after 'while'");
    ast::ExprPtr condition = parseExpression();
    consume(lexer::TokenType::CloseParen, "Expected ')' after do-while condition");
    consume(lexer::TokenType::Semicolon, "Expected ';' after do-while statement");

    return std::make_unique<ast::DoWhileStmt>(std::move(body), std::move(condition), loc);
}

ast::StmtPtr Parser::parseForStatement() {
    diagnostics::SourceLocation loc = previous().location;
    consume(lexer::TokenType::OpenParen, "Expected '(' after 'for'");

    // For-In loop:
    // Case 1: for (var x in iterable) or for (let x in iterable)
    if ((check(lexer::TokenType::KeywordVar) || check(lexer::TokenType::KeywordLet)) &&
        checkNext(lexer::TokenType::Identifier) &&
        m_current + 2 < m_tokens.size() && m_tokens[m_current + 2].lexeme == "in") {
        advance(); // var / let
        std::string varName = advance().lexeme;
        advance(); // in
        ast::ExprPtr iterable = parseExpression();
        consume(lexer::TokenType::CloseParen, "Expected ')' after for-in expression");
        ast::StmtPtr body = parseStatement();
        return std::make_unique<ast::ForInStmt>(varName, "auto", std::move(iterable), std::move(body), loc);
    }

    // Case 2: for (x in iterable)
    if (check(lexer::TokenType::Identifier) && checkNext(lexer::TokenType::Identifier) &&
        m_tokens[m_current + 1].lexeme == "in") {
        std::string varName = advance().lexeme;
        advance(); // in
        ast::ExprPtr iterable = parseExpression();
        consume(lexer::TokenType::CloseParen, "Expected ')' after for-in expression");
        ast::StmtPtr body = parseStatement();
        return std::make_unique<ast::ForInStmt>(varName, "auto", std::move(iterable), std::move(body), loc);
    }

    // Case 3: for (Type x in iterable) e.g. for (int x in nums) or for (string s in list)
    if (checkNext(lexer::TokenType::Identifier) &&
        m_current + 2 < m_tokens.size() && m_tokens[m_current + 2].lexeme == "in") {
        std::string type = parseType();
        std::string varName = advance().lexeme;
        advance(); // in
        ast::ExprPtr iterable = parseExpression();
        consume(lexer::TokenType::CloseParen, "Expected ')' after for-in expression");
        ast::StmtPtr body = parseStatement();
        return std::make_unique<ast::ForInStmt>(varName, type, std::move(iterable), std::move(body), loc);
    }

    // Classic for loop: for (init; condition; increment)
    ast::StmtPtr init = nullptr;
    if (!match(lexer::TokenType::Semicolon)) {
        init = parseStatement();
        // If init was a VarDeclStmt, it already consumed the ';'
    }

    ast::ExprPtr condition = nullptr;
    if (!check(lexer::TokenType::Semicolon)) {
        condition = parseExpression();
    }
    consume(lexer::TokenType::Semicolon, "Expected ';' after for loop condition");

    ast::ExprPtr increment = nullptr;
    if (!check(lexer::TokenType::CloseParen)) {
        increment = parseExpression();
    }
    consume(lexer::TokenType::CloseParen, "Expected ')' after for clauses");

    ast::StmtPtr body = parseStatement();
    return std::make_unique<ast::ForStmt>(std::move(init), std::move(condition), std::move(increment), std::move(body), loc);
}

ast::StmtPtr Parser::parseSwitchStatement() {
    diagnostics::SourceLocation loc = previous().location;
    consume(lexer::TokenType::OpenParen, "Expected '(' after 'switch'");
    ast::ExprPtr subject = parseExpression();
    consume(lexer::TokenType::CloseParen, "Expected ')' after switch subject");
    consume(lexer::TokenType::OpenBrace, "Expected '{' before switch cases");

    std::vector<ast::SwitchCase> cases;
    std::vector<ast::StmtPtr> defaultStmts;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        if (match(lexer::TokenType::KeywordCase)) {
            ast::ExprPtr val = parseExpression();
            consume(lexer::TokenType::Colon, "Expected ':' after case value");
            std::vector<ast::StmtPtr> stmts;
            while (!isAtEnd() && !check(lexer::TokenType::KeywordCase) &&
                   !check(lexer::TokenType::KeywordDefault) && !check(lexer::TokenType::CloseBrace)) {
                stmts.push_back(parseStatement());
            }
            cases.emplace_back(std::move(val), std::move(stmts));
        } else if (match(lexer::TokenType::KeywordDefault)) {
            consume(lexer::TokenType::Colon, "Expected ':' after default");
            while (!isAtEnd() && !check(lexer::TokenType::KeywordCase) &&
                   !check(lexer::TokenType::KeywordDefault) && !check(lexer::TokenType::CloseBrace)) {
                defaultStmts.push_back(parseStatement());
            }
        } else {
            errorAt(peek(), "Expected 'case' or 'default' in switch block");
            advance();
        }
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' after switch cases");
    return std::make_unique<ast::SwitchStmt>(std::move(subject), std::move(cases), std::move(defaultStmts), loc);
}

ast::StmtPtr Parser::parseMatchStatement() {
    diagnostics::SourceLocation loc = previous().location;
    ast::ExprPtr subject = parseExpression();
    consume(lexer::TokenType::OpenBrace, "Expected '{' before match arms");

    std::vector<ast::MatchArm> arms;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        ast::ExprPtr pattern = nullptr;

        // Wildcard '_' pattern
        if (match(lexer::TokenType::Identifier) && previous().lexeme == "_") {
            pattern = nullptr;
        } else {
            pattern = parseExpression();
        }

        consume(lexer::TokenType::FatArrow, "Expected '=>' after match pattern");

        ast::ASTNodePtr body;
        if (check(lexer::TokenType::OpenBrace)) {
            body = parseBlock();
        } else {
            body = parseExpression();
        }

        arms.emplace_back(std::move(pattern), std::move(body));

        match(lexer::TokenType::Comma);
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' after match block");
    return std::make_unique<ast::MatchStmt>(std::move(subject), std::move(arms), loc);
}

ast::StmtPtr Parser::parseReturnStatement() {
    diagnostics::SourceLocation loc = previous().location;
    ast::ExprPtr value = nullptr;

    if (!check(lexer::TokenType::Semicolon)) {
        value = parseExpression();
    }

    consume(lexer::TokenType::Semicolon, "Expected ';' after return statement");
    return std::make_unique<ast::ReturnStmt>(std::move(value), loc);
}

ast::StmtPtr Parser::parseBreakStatement() {
    diagnostics::SourceLocation loc = previous().location;
    consume(lexer::TokenType::Semicolon, "Expected ';' after break");
    return std::make_unique<ast::BreakStmt>(loc);
}

ast::StmtPtr Parser::parseContinueStatement() {
    diagnostics::SourceLocation loc = previous().location;
    consume(lexer::TokenType::Semicolon, "Expected ';' after continue");
    return std::make_unique<ast::ContinueStmt>(loc);
}

ast::StmtPtr Parser::parseTryCatchStatement() {
    diagnostics::SourceLocation loc = previous().location;
    std::unique_ptr<ast::BlockStmt> tryBlock = parseBlock();
    std::vector<ast::CatchClause> catches;

    while (match(lexer::TokenType::KeywordCatch)) {
        consume(lexer::TokenType::OpenParen, "Expected '(' after 'catch'");
        std::string exType = parseType();
        const lexer::Token& exName = consume(lexer::TokenType::Identifier, "Expected exception variable name");
        consume(lexer::TokenType::CloseParen, "Expected ')' after catch clause parameters");

        std::unique_ptr<ast::BlockStmt> catchBody = parseBlock();
        catches.emplace_back(std::move(exType), exName.lexeme, std::move(catchBody));
    }

    std::unique_ptr<ast::BlockStmt> finallyBlock = nullptr;
    if (match(lexer::TokenType::KeywordFinally)) {
        finallyBlock = parseBlock();
    }

    return std::make_unique<ast::TryCatchStmt>(std::move(tryBlock), std::move(catches), std::move(finallyBlock), loc);
}

ast::StmtPtr Parser::parseThrowStatement() {
    diagnostics::SourceLocation loc = previous().location;
    ast::ExprPtr expr = parseExpression();
    consume(lexer::TokenType::Semicolon, "Expected ';' after throw statement");
    return std::make_unique<ast::ThrowStmt>(std::move(expr), loc);
}

std::unique_ptr<ast::BlockStmt> Parser::parseBlock() {
    diagnostics::SourceLocation loc = peek().location;
    consume(lexer::TokenType::OpenBrace, "Expected '{' to start block");

    std::vector<ast::StmtPtr> statements;
    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        statements.push_back(parseStatement());
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' to close block");
    return std::make_unique<ast::BlockStmt>(std::move(statements), loc);
}

ast::StmtPtr Parser::parseExprStatement() {
    diagnostics::SourceLocation loc = peek().location;
    ast::ExprPtr expr = parseExpression();
    consume(lexer::TokenType::Semicolon, "Expected ';' after expression");
    return std::make_unique<ast::ExprStmt>(std::move(expr), loc);
}

ast::ASTNodePtr Parser::parseDeclaration() {
    std::string visibility;
    bool isStatic = false;
    bool isOverride = false;
    bool isAbstract = false;

    // Modifiers
    while (true) {
        if (match(lexer::TokenType::KeywordPublic)) { visibility = "public"; continue; }
        if (match(lexer::TokenType::KeywordPrivate)) { visibility = "private"; continue; }
        if (match(lexer::TokenType::KeywordProtected)) { visibility = "protected"; continue; }
        if (match(lexer::TokenType::KeywordInternal)) { visibility = "internal"; continue; }
        if (match(lexer::TokenType::KeywordStatic)) { isStatic = true; continue; }
        if (match(lexer::TokenType::KeywordOverride)) { isOverride = true; continue; }
        if (match(lexer::TokenType::KeywordAbstract)) { isAbstract = true; continue; }
        break;
    }

    if (match(lexer::TokenType::KeywordModule)) return parseModuleDecl();
    if (match(lexer::TokenType::KeywordImport)) return parseImportDecl();
    if (match(lexer::TokenType::KeywordFn)) return parseFunctionDecl(visibility, isStatic, isOverride, isAbstract);
    if (match(lexer::TokenType::KeywordClass)) return parseClassDecl(visibility, isAbstract);
    if (match(lexer::TokenType::KeywordInterface)) return parseInterfaceDecl(visibility);
    if (match(lexer::TokenType::KeywordStruct)) return parseStructDecl(visibility);
    if (match(lexer::TokenType::KeywordEnum)) return parseEnumDecl(visibility);

    // Variable declarations with visibility or const/var
    if (match(lexer::TokenType::KeywordConst)) return parseVarDecl(true, false);
    if (match(lexer::TokenType::KeywordVar)) return parseVarDecl(false, true);
    if (match(lexer::TokenType::KeywordLet)) return parseVarDecl(false, true);

    // Extern block: extern "C" { ... }
    if (match(lexer::TokenType::KeywordExtern)) {
        diagnostics::SourceLocation loc = previous().location;
        if (check(lexer::TokenType::StringLiteral)) {
            advance(); // Consume "C"
        }
        consume(lexer::TokenType::OpenBrace, "Expected '{' to start extern block");
        std::vector<ast::StmtPtr> externDecls;
        while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
            ast::ASTNodePtr decl = parseDeclaration();
            if (decl) {
                // Cast ASTNodePtr to StmtPtr since all declarations inherit from StatementNode
                externDecls.push_back(std::unique_ptr<ast::StatementNode>(static_cast<ast::StatementNode*>(decl.release())));
            }
        }
        consume(lexer::TokenType::CloseBrace, "Expected '}' after extern block");
        return std::make_unique<ast::BlockStmt>(std::move(externDecls), loc);
    }

    // Syntax Compatibility: Java/C++ style declarations (type identifier (...) or type identifier = ...;)
    bool couldBeDecl = isTypeKeyword(peek().type) ||
                       (check(lexer::TokenType::Identifier) &&
                        (checkNext(lexer::TokenType::Identifier) ||
                         checkNext(lexer::TokenType::Less) ||
                         checkNext(lexer::TokenType::Question) ||
                         (checkNext(lexer::TokenType::OpenBracket) && m_current + 2 < m_tokens.size() && m_tokens[m_current + 2].type == lexer::TokenType::CloseBracket)));
    if (couldBeDecl) {
        size_t saved = m_current;
        std::string type = parseType();
        if (check(lexer::TokenType::Identifier)) {
            if (checkNext(lexer::TokenType::OpenParen)) {
                // Java/C++ style function: int add(int a, int b) { ... }
                const lexer::Token& nameTok = advance();
                diagnostics::SourceLocation loc = nameTok.location;
                std::vector<ast::Parameter> params = parseParameters();
                std::unique_ptr<ast::BlockStmt> body = nullptr;
                if (check(lexer::TokenType::OpenBrace)) {
                    body = parseBlock();
                } else {
                    consume(lexer::TokenType::Semicolon, "Expected ';' after function signature");
                }

                return std::make_unique<ast::FunctionDecl>(
                    nameTok.lexeme, std::move(params), type,
                    std::move(body), nullptr, visibility, isStatic, isOverride, isAbstract, false, loc);
            } else if (checkNext(lexer::TokenType::Equal) || checkNext(lexer::TokenType::Semicolon) || checkNext(lexer::TokenType::Comma)) {
                // Field or global variable: [public/protected] int count = 0;
                const lexer::Token& nameTok = advance();
                diagnostics::SourceLocation loc = nameTok.location;
                ast::ExprPtr init = nullptr;
                if (match(lexer::TokenType::Equal)) {
                    init = parseExpression();
                }
                consume(lexer::TokenType::Semicolon, "Expected ';' after variable declaration");
                return std::make_unique<ast::VarDeclStmt>(type, nameTok.lexeme, std::move(init), false, false, loc, visibility, isStatic);
            }
        }
        m_current = saved; // Not a declaration, rewind
    }

    return parseStatement();
}

ast::ASTNodePtr Parser::parseModuleDecl() {
    diagnostics::SourceLocation loc = previous().location;
    std::string moduleName;

    const lexer::Token& part = consume(lexer::TokenType::Identifier, "Expected module identifier");
    moduleName += part.lexeme;

    while (match(lexer::TokenType::Dot)) {
        moduleName += ".";
        const lexer::Token& subPart = consume(lexer::TokenType::Identifier, "Expected identifier after '.' in module name");
        moduleName += subPart.lexeme;
    }

    consume(lexer::TokenType::Semicolon, "Expected ';' after module declaration");
    return std::make_unique<ast::ModuleDecl>(moduleName, loc);
}

ast::ASTNodePtr Parser::parseImportDecl() {
    diagnostics::SourceLocation loc = previous().location;
    std::string importPath;
    bool isWildcard = false;

    const lexer::Token& part = consume(lexer::TokenType::Identifier, "Expected import path");
    importPath += part.lexeme;

    while (match(lexer::TokenType::Dot)) {
        if (match(lexer::TokenType::Star)) {
            isWildcard = true;
            break;
        }
        importPath += ".";
        const lexer::Token& subPart = consume(lexer::TokenType::Identifier, "Expected identifier after '.' in import path");
        importPath += subPart.lexeme;
    }

    consume(lexer::TokenType::Semicolon, "Expected ';' after import statement");
    return std::make_unique<ast::ImportDecl>(importPath, isWildcard, loc);
}

ast::ASTNodePtr Parser::parseFunctionDecl(std::string visibility, bool isStatic, bool isOverride, bool isAbstract) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected function name");
    std::string name = nameTok.lexeme;

    std::vector<std::string> typeParams;
    if (match(lexer::TokenType::Less)) {
        do {
            const lexer::Token& paramTok = consume(lexer::TokenType::Identifier, "Expected type parameter name");
            typeParams.push_back(paramTok.lexeme);
        } while (match(lexer::TokenType::Comma));
        consume(lexer::TokenType::Greater, "Expected '>' after generic type parameters");
    }

    std::vector<ast::Parameter> params = parseParameters();

    std::string returnType = "void";
    if (match(lexer::TokenType::Arrow)) {
        returnType = parseType();
    }

    std::unique_ptr<ast::BlockStmt> body = nullptr;
    ast::ExprPtr shortBody = nullptr;

    // Short form: fn add(a, b) => a + b;
    if (match(lexer::TokenType::FatArrow)) {
        shortBody = parseExpression();
        consume(lexer::TokenType::Semicolon, "Expected ';' after short-form function expression");
    } else if (check(lexer::TokenType::OpenBrace)) {
        body = parseBlock();
    } else if (match(lexer::TokenType::Semicolon)) {
        // Function signature without body (extern or abstract)
    } else {
        errorAt(peek(), "Expected '{' or '=>' for function body");
    }

    return std::make_unique<ast::FunctionDecl>(
        name, std::move(typeParams), std::move(params), returnType,
        std::move(body), std::move(shortBody),
        visibility, isStatic, isOverride, isAbstract, false, loc);
}

ast::ASTNodePtr Parser::parseClassDecl(std::string visibility, bool isAbstract) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected class name");
    std::string name = nameTok.lexeme;

    std::vector<std::string> typeParams;
    if (match(lexer::TokenType::Less)) {
        do {
            const lexer::Token& paramTok = consume(lexer::TokenType::Identifier, "Expected type parameter name");
            typeParams.push_back(paramTok.lexeme);
        } while (match(lexer::TokenType::Comma));
        consume(lexer::TokenType::Greater, "Expected '>' after generic type parameters");
    }

    std::string baseClass;
    std::vector<std::string> interfaces;

    // Syntax Compatibility:
    // Java style: class Dog extends Animal implements Printable
    // C# style:   class Dog : Animal, Printable
    if (match(lexer::TokenType::KeywordExtends)) {
        const lexer::Token& baseTok = consume(lexer::TokenType::Identifier, "Expected base class name after 'extends'");
        baseClass = baseTok.lexeme;
    }

    if (match(lexer::TokenType::Colon)) {
        const lexer::Token& baseOrInterface = consume(lexer::TokenType::Identifier, "Expected base class or interface name after ':'");
        baseClass = baseOrInterface.lexeme;

        while (match(lexer::TokenType::Comma)) {
            const lexer::Token& ifaceTok = consume(lexer::TokenType::Identifier, "Expected interface name after ','");
            interfaces.push_back(ifaceTok.lexeme);
        }
    }

    if (match(lexer::TokenType::KeywordImplements)) {
        do {
            const lexer::Token& ifaceTok = consume(lexer::TokenType::Identifier, "Expected interface name after 'implements'");
            interfaces.push_back(ifaceTok.lexeme);
        } while (match(lexer::TokenType::Comma));
    }

    consume(lexer::TokenType::OpenBrace, "Expected '{' to start class body");

    std::vector<ast::ASTNodePtr> members;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        // Constructor compatibility: ClassName(...)
        if (check(lexer::TokenType::Identifier) && peek().lexeme == name && checkNext(lexer::TokenType::OpenParen)) {
            const lexer::Token& ctorName = advance();
            diagnostics::SourceLocation ctorLoc = ctorName.location;
            std::vector<ast::Parameter> params = parseParameters();
            std::unique_ptr<ast::BlockStmt> body = parseBlock();
            members.push_back(std::make_unique<ast::FunctionDecl>(
                ctorName.lexeme, std::move(params), "void", std::move(body), nullptr,
                "public", false, false, false, false, ctorLoc));
            continue;
        }

        members.push_back(parseDeclaration());
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of class body");

    return std::make_unique<ast::ClassDecl>(name, std::move(typeParams), baseClass, std::move(interfaces), std::move(members), visibility, isAbstract, loc);
}

ast::ASTNodePtr Parser::parseInterfaceDecl(std::string visibility) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected interface name");
    std::string name = nameTok.lexeme;

    consume(lexer::TokenType::OpenBrace, "Expected '{' to start interface body");

    std::vector<std::unique_ptr<ast::FunctionDecl>> methods;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        if (match(lexer::TokenType::KeywordFn)) {
            const lexer::Token& fnName = consume(lexer::TokenType::Identifier, "Expected method name in interface");
            diagnostics::SourceLocation fnLoc = fnName.location;
            std::vector<ast::Parameter> params = parseParameters();
            std::string returnType = "void";
            if (match(lexer::TokenType::Arrow)) {
                returnType = parseType();
            }
            consume(lexer::TokenType::Semicolon, "Expected ';' after interface method signature");
            methods.push_back(std::make_unique<ast::FunctionDecl>(
                fnName.lexeme, std::move(params), returnType, nullptr, nullptr,
                "public", false, false, true, false, fnLoc));
        } else {
            errorAt(peek(), "Only method signatures are permitted in interfaces");
            advance();
        }
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of interface body");
    return std::make_unique<ast::InterfaceDecl>(name, std::move(methods), visibility, loc);
}

ast::ASTNodePtr Parser::parseStructDecl(std::string visibility) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected struct name");
    std::string name = nameTok.lexeme;

    consume(lexer::TokenType::OpenBrace, "Expected '{' to start struct body");

    std::vector<std::unique_ptr<ast::VarDeclStmt>> fields;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        std::string type = parseType();
        const lexer::Token& fieldName = consume(lexer::TokenType::Identifier, "Expected field name in struct");
        diagnostics::SourceLocation fieldLoc = fieldName.location;
        consume(lexer::TokenType::Semicolon, "Expected ';' after struct field");
        fields.push_back(std::make_unique<ast::VarDeclStmt>(type, fieldName.lexeme, nullptr, false, false, fieldLoc));
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of struct body");
    return std::make_unique<ast::StructDecl>(name, std::move(fields), visibility, loc);
}

ast::ASTNodePtr Parser::parseEnumDecl(std::string visibility) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected enum name");
    std::string name = nameTok.lexeme;

    consume(lexer::TokenType::OpenBrace, "Expected '{' to start enum body");

    std::vector<ast::EnumVariant> variants;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        const lexer::Token& varTok = consume(lexer::TokenType::Identifier, "Expected enum variant identifier");
        ast::ExprPtr val = nullptr;

        if (match(lexer::TokenType::Equal)) {
            val = parseExpression();
        }

        variants.emplace_back(varTok.lexeme, std::move(val));

        if (!match(lexer::TokenType::Comma)) {
            break;
        }
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of enum body");
    return std::make_unique<ast::EnumDecl>(name, std::move(variants), visibility, loc);
}

std::string Parser::parseType() {
    std::string type;
    if (isTypeKeyword(peek().type)) {
        type = advance().lexeme;
    } else if (match(lexer::TokenType::Identifier)) {
        type = previous().lexeme;
    } else {
        errorAt(peek(), "Expected type name");
        return "unknown";
    }

    // Generic type parameters: List<int>, Map<string, User>
    if (match(lexer::TokenType::Less)) {
        type += "<";
        do {
            type += parseType();
            if (check(lexer::TokenType::Comma)) {
                type += ", ";
                advance();
            }
        } while (!isAtEnd() && !check(lexer::TokenType::Greater));
        consume(lexer::TokenType::Greater, "Expected '>' after generic type arguments");
        type += ">";
    }

    // Array type: int[]
    while (check(lexer::TokenType::OpenBracket) && checkNext(lexer::TokenType::CloseBracket)) {
        advance();
        advance();
        type += "[]";
    }

    // Nullable type: User?
    if (match(lexer::TokenType::Question)) {
        type += "?";
    }

    return type;
}

std::vector<ast::Parameter> Parser::parseParameters() {
    consume(lexer::TokenType::OpenParen, "Expected '(' to begin parameter list");
    std::vector<ast::Parameter> params;

    if (!check(lexer::TokenType::CloseParen)) {
        do {
            std::string type;
            std::string paramName;

            if (check(lexer::TokenType::Identifier) && checkNext(lexer::TokenType::Colon)) {
                paramName = advance().lexeme;
                advance(); // consume ':'
                type = parseType();
            } else {
                bool couldBeType = isTypeKeyword(peek().type) ||
                                   (check(lexer::TokenType::Identifier) &&
                                    (checkNext(lexer::TokenType::Identifier) ||
                                     checkNext(lexer::TokenType::Question) ||
                                     (checkNext(lexer::TokenType::OpenBracket) && m_current + 2 < m_tokens.size() && m_tokens[m_current + 2].type == lexer::TokenType::CloseBracket) ||
                                     checkNext(lexer::TokenType::Less)));
                if (couldBeType) {
                    type = parseType();
                }
                const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected parameter name");
                paramName = nameTok.lexeme;
            }

            ast::ExprPtr defaultVal = nullptr;

            if (match(lexer::TokenType::Equal)) {
                defaultVal = parseExpression();
            }

            params.emplace_back(std::move(type), std::move(paramName), std::move(defaultVal));
        } while (match(lexer::TokenType::Comma));
    }

    consume(lexer::TokenType::CloseParen, "Expected ')' to end parameter list");
    return params;
}

bool Parser::isTypeKeyword(lexer::TokenType type) noexcept {
    switch (type) {
        case lexer::TokenType::KeywordVoid:
        case lexer::TokenType::KeywordInt:
        case lexer::TokenType::KeywordUint:
        case lexer::TokenType::KeywordInt8:
        case lexer::TokenType::KeywordInt16:
        case lexer::TokenType::KeywordInt32:
        case lexer::TokenType::KeywordInt64:
        case lexer::TokenType::KeywordUint8:
        case lexer::TokenType::KeywordUint16:
        case lexer::TokenType::KeywordUint32:
        case lexer::TokenType::KeywordUint64:
        case lexer::TokenType::KeywordFloat:
        case lexer::TokenType::KeywordDouble:
        case lexer::TokenType::KeywordBool:
        case lexer::TokenType::KeywordChar:
        case lexer::TokenType::KeywordString:
        case lexer::TokenType::KeywordByte:
            return true;
        default:
            return false;
    }
}

std::unique_ptr<ast::Program> Parser::parseProgram() {
    diagnostics::SourceLocation loc = peek().location;
    std::vector<ast::ASTNodePtr> declarations;

    while (!isAtEnd()) {
        if (match(lexer::TokenType::Semicolon)) continue;
        try {
            ast::ASTNodePtr decl = parseDeclaration();
            if (decl) {
                declarations.push_back(std::move(decl));
            }
        } catch (...) {
            synchronize();
        }
    }

    return std::make_unique<ast::Program>(std::move(declarations), loc);
}

} // namespace sp::parser
