#include "sp/parser/Parser.h"
#include "sp/lexer/Lexer.h"
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
    if (type == lexer::TokenType::Greater && m_pendingGreater > 0) return true;
    return peek().is(type);
}

bool Parser::checkNext(lexer::TokenType type) const noexcept {
    if (isAtEnd()) return false;
    return peekNext().is(type);
}

bool Parser::match(lexer::TokenType type) noexcept {
    if (type == lexer::TokenType::Greater) {
        if (m_pendingGreater > 0) {
            m_pendingGreater--;
            advance();
            return true;
        }
        if (check(lexer::TokenType::Greater)) {
            advance();
            return true;
        }
        if (check(lexer::TokenType::GreaterGreater)) {
            m_pendingGreater = 1;
            return true;
        }
        return false;
    }
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchAny(std::initializer_list<lexer::TokenType> types) noexcept {
    for (auto type : types) {
        if (match(type)) {
            return true;
        }
    }
    return false;
}

const lexer::Token& Parser::consume(lexer::TokenType type, std::string_view message, std::string_view hint) {
    if (match(type)) {
        return previous();
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

bool Parser::isTernaryQuestion() const noexcept {
    return isTernaryQuestionAt(m_current);
}

bool Parser::isTernaryQuestionAt(size_t pos) const noexcept {
    if (pos >= m_tokens.size() || !m_tokens[pos].is(lexer::TokenType::Question)) {
        return false;
    }

    size_t next = pos + 1;
    if (next >= m_tokens.size()) {
        return false;
    }

    const auto& nextTok = m_tokens[next];
    if (nextTok.is(lexer::TokenType::Colon) ||
        nextTok.is(lexer::TokenType::Semicolon) ||
        nextTok.is(lexer::TokenType::Comma) ||
        nextTok.is(lexer::TokenType::CloseParen) ||
        nextTok.is(lexer::TokenType::CloseBracket) ||
        nextTok.is(lexer::TokenType::CloseBrace) ||
        nextTok.is(lexer::TokenType::FatArrow) ||
        nextTok.is(lexer::TokenType::EndOfFile)) {
        return false;
    }

    int parenDepth = 0;
    int bracketDepth = 0;
    int braceDepth = 0;
    int ternaryDepth = 0;

    for (size_t i = next; i < m_tokens.size(); ++i) {
        const auto& tok = m_tokens[i];
        if (tok.is(lexer::TokenType::OpenParen)) {
            parenDepth++;
        } else if (tok.is(lexer::TokenType::CloseParen)) {
            if (parenDepth == 0) return false;
            parenDepth--;
        } else if (tok.is(lexer::TokenType::OpenBracket)) {
            bracketDepth++;
        } else if (tok.is(lexer::TokenType::CloseBracket)) {
            if (bracketDepth == 0) return false;
            bracketDepth--;
        } else if (tok.is(lexer::TokenType::OpenBrace)) {
            braceDepth++;
        } else if (tok.is(lexer::TokenType::CloseBrace)) {
            if (braceDepth == 0) return false;
            braceDepth--;
        } else if (parenDepth == 0 && bracketDepth == 0 && braceDepth == 0) {
            if (tok.is(lexer::TokenType::Semicolon) ||
                tok.is(lexer::TokenType::Comma) ||
                tok.is(lexer::TokenType::FatArrow) ||
                tok.is(lexer::TokenType::EndOfFile) ||
                tok.is(lexer::TokenType::KeywordReturn) ||
                tok.is(lexer::TokenType::KeywordLet) ||
                tok.is(lexer::TokenType::KeywordVar) ||
                tok.is(lexer::TokenType::KeywordConst) ||
                tok.is(lexer::TokenType::KeywordFn)) {
                return false;
            }
            if (tok.is(lexer::TokenType::Question)) {
                if (i + 1 < m_tokens.size()) {
                    const auto& afterQ = m_tokens[i + 1];
                    if (!afterQ.is(lexer::TokenType::Colon) &&
                        !afterQ.is(lexer::TokenType::Semicolon) &&
                        !afterQ.is(lexer::TokenType::Comma) &&
                        !afterQ.is(lexer::TokenType::CloseParen) &&
                        !afterQ.is(lexer::TokenType::CloseBracket) &&
                        !afterQ.is(lexer::TokenType::CloseBrace)) {
                        ternaryDepth++;
                    }
                }
            } else if (tok.is(lexer::TokenType::Colon)) {
                if (ternaryDepth == 0) {
                    return true;
                }
                ternaryDepth--;
            }
        }
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
            return isTernaryQuestion() ? Precedence::Ternary : Precedence::Call;

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
        return parseStringLiteral();
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
    if (match(lexer::TokenType::KeywordSpawn)) {
        return std::make_unique<ast::IdentifierExpr>("spawn", previous().location);
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
    if (isTypeKeyword(peek().type)) {
        const auto& tok = advance();
        return std::make_unique<ast::IdentifierExpr>(tok.lexeme, tok.location);
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

    // Await Expression: await expr
    if (match(lexer::TokenType::KeywordAwait)) {
        diagnostics::SourceLocation loc = previous().location;
        ast::ExprPtr operand = parseExpression(Precedence::Unary);
        return std::make_unique<ast::AwaitExpr>(std::move(operand), loc);
    }

    // Comptime Expression: comptime { ... } or comptime expr
    if (match(lexer::TokenType::KeywordComptime)) {
        diagnostics::SourceLocation loc = previous().location;
        if (check(lexer::TokenType::OpenBrace)) {
            auto block = parseBlock();
            return std::make_unique<ast::ComptimeExpr>(std::move(block), loc);
        }
        ast::ExprPtr operand = parseExpression(Precedence::Unary);
        return std::make_unique<ast::ComptimeExpr>(std::move(operand), loc);
    }

    // Prefix unary operators (!, -, +, ~, ++, --, *, &)
    if (matchAny({lexer::TokenType::Bang, lexer::TokenType::Minus, lexer::TokenType::Plus,
                  lexer::TokenType::Tilde, lexer::TokenType::PlusPlus, lexer::TokenType::MinusMinus,
                  lexer::TokenType::Star, lexer::TokenType::Amp})) {
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
        const lexer::Token& memberTok = (check(lexer::TokenType::Identifier) || isTypeKeyword(peek().type) || peek().type == lexer::TokenType::KeywordSpawn)
            ? advance()
            : consume(lexer::TokenType::Identifier, "Expected member identifier after '.'");
        return std::make_unique<ast::MemberExpr>(std::move(left), memberTok.lexeme, isSafeNav, loc);
    }

    // Ternary Operator: condition ? thenExpr : elseExpr, or Postfix Try: expr?
    if (opTok.is(lexer::TokenType::Question)) {
        if (!isTernaryQuestionAt(m_current - 1)) {
            return std::make_unique<ast::TryExpr>(std::move(left), loc);
        }
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
            } else if (check(lexer::TokenType::Identifier) && m_current + 1 < m_tokens.size() &&
                       m_tokens[m_current + 1].type == lexer::TokenType::Identifier) {
                type = advance().lexeme;
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

ast::ExprPtr Parser::parseStringLiteral() {
    const std::string& str = previous().lexeme;
    diagnostics::SourceLocation loc = previous().location;

    if (str.find("${") == std::string::npos) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, str, loc);
    }

    std::vector<ast::ExprPtr> parts;
    size_t lastPos = 0;

    while (lastPos < str.size()) {
        size_t start = str.find("${", lastPos);
        if (start == std::string::npos) {
            std::string text = str.substr(lastPos);
            if (!text.empty() || parts.empty()) {
                parts.push_back(std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, text, loc));
            }
            break;
        }

        if (start > lastPos) {
            std::string prefix = str.substr(lastPos, start - lastPos);
            parts.push_back(std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, prefix, loc));
        }

        size_t end = str.find('}', start + 2);
        if (end == std::string::npos) {
            parts.push_back(std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, str.substr(start), loc));
            break;
        }

        std::string exprCode = str.substr(start + 2, end - (start + 2));
        lexer::Lexer subLexer(exprCode);
        std::vector<lexer::Token> subTokens = subLexer.tokenizeAll();
        Parser subParser(subTokens, m_filename, m_diagnostics);
        ast::ExprPtr parsedExpr = subParser.parseExpression();
        if (parsedExpr) {
            parts.push_back(std::move(parsedExpr));
        }

        lastPos = end + 1;
    }

    if (parts.empty()) {
        return std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, "", loc);
    }

    size_t startIndex = 0;
    ast::ExprPtr result = nullptr;
    auto* firstLit = dynamic_cast<ast::LiteralExpr*>(parts[0].get());
    if (firstLit && firstLit->getLiteralKind() == ast::LiteralKind::String) {
        result = std::move(parts[0]);
        startIndex = 1;
    } else {
        result = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, "", loc);
        startIndex = 0;
    }

    for (size_t i = startIndex; i < parts.size(); ++i) {
        result = std::make_unique<ast::BinaryExpr>(std::move(result), lexer::TokenType::Plus, std::move(parts[i]), loc);
    }

    return result;
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

    if (check(lexer::TokenType::CloseBrace)) {
        advance();
        return std::make_unique<ast::MapExpr>(std::vector<ast::MapExpr::Entry>{}, loc);
    }

    ast::ExprPtr first = parseExpression();
    if (match(lexer::TokenType::Colon)) {
        std::vector<ast::MapExpr::Entry> entries;
        ast::ExprPtr value = parseExpression();
        entries.emplace_back(std::move(first), std::move(value));

        while (match(lexer::TokenType::Comma)) {
            if (check(lexer::TokenType::CloseBrace)) break;
            ast::ExprPtr key = parseExpression();
            consume(lexer::TokenType::Colon, "Expected ':' between key and value in map entry");
            ast::ExprPtr val = parseExpression();
            entries.emplace_back(std::move(key), std::move(val));
        }

        consume(lexer::TokenType::CloseBrace, "Expected '}' at end of map literal");
        return std::make_unique<ast::MapExpr>(std::move(entries), loc);
    } else {
        std::vector<ast::ExprPtr> elements;
        elements.push_back(std::move(first));

        while (match(lexer::TokenType::Comma)) {
            if (check(lexer::TokenType::CloseBrace)) break;
            elements.push_back(parseExpression());
        }

        consume(lexer::TokenType::CloseBrace, "Expected '}' at end of array literal");
        return std::make_unique<ast::ArrayExpr>(std::move(elements), loc);
    }
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
    if (check(lexer::TokenType::At)) {
        auto decl = parseDeclaration();
        if (decl) {
            return std::unique_ptr<ast::StatementNode>(static_cast<ast::StatementNode*>(decl.release()));
        }
        return nullptr;
    }
    if (check(lexer::TokenType::OpenBrace)) {
        return parseBlock();
    }
    if (match(lexer::TokenType::KeywordWeak)) {
        if (match(lexer::TokenType::KeywordVar) || match(lexer::TokenType::KeywordLet)) {
            return parseVarDecl(false, true, true);
        }
        if (match(lexer::TokenType::KeywordConst)) {
            return parseVarDecl(true, false, true);
        }
        return parseVarDecl(false, false, true);
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
    if (match(lexer::TokenType::KeywordEnum)) {
        auto ed = parseEnumDecl("");
        return std::unique_ptr<ast::StatementNode>(static_cast<ast::StatementNode*>(ed.release()));
    }
    if (match(lexer::TokenType::KeywordUnsafe)) {
        if (check(lexer::TokenType::OpenBrace)) {
            auto block = parseBlock();
            if (block) block->setUnsafe(true);
            return block;
        }
    }

    // Check for variable declaration with explicit or user-defined type: int x = 10; or Person p = ...;
    bool couldBeTypeDecl = isTypeKeyword(peek().type) ||
                           check(lexer::TokenType::Star) ||
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

std::vector<ast::Annotation> Parser::parseAnnotations() {
    std::vector<ast::Annotation> annotations;
    while (match(lexer::TokenType::At)) {
        diagnostics::SourceLocation loc = previous().location;
        const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected annotation name after '@'");
        std::vector<std::string> args;
        if (match(lexer::TokenType::OpenParen)) {
            if (!check(lexer::TokenType::CloseParen)) {
                do {
                    if (check(lexer::TokenType::StringLiteral)) {
                        const lexer::Token& strTok = advance();
                        std::string val = strTok.lexeme;
                        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                            val = val.substr(1, val.size() - 2);
                        }
                        args.push_back(val);
                    } else if (check(lexer::TokenType::Identifier) || check(lexer::TokenType::IntegerLiteral) || check(lexer::TokenType::FloatLiteral)) {
                        args.push_back(advance().lexeme);
                    } else {
                        errorAt(peek(), "Expected string literal or identifier in annotation argument");
                        break;
                    }
                } while (match(lexer::TokenType::Comma));
            }
            consume(lexer::TokenType::CloseParen, "Expected ')' after annotation arguments");
        }
        annotations.push_back(ast::Annotation{nameTok.lexeme, std::move(args), loc});
    }
    return annotations;
}

ast::StmtPtr Parser::parseVarDecl(bool isConst, bool isVar, bool isWeak, std::string visibility, bool isStatic) {
    diagnostics::SourceLocation loc = previous().location;
    std::string type;

    if (isTypeKeyword(peek().type)) {
        type = parseType();
    } else if (!isVar && !isConst) {
        type = parseType();
    }

    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected variable name in declaration");

    // Phase 5 slice C: no name, no declaration (`let ;` used to produce
    // a VarDeclStmt with an empty name). Do not synchronize here: the
    // trailing `;` (or whatever follows) is consumed by the enclosing
    // loop / next parse attempt, so returning null cannot loop.
    if (nameTok.lexeme.empty()) return nullptr;

    // Support type annotation after colon: let x: int = 10; or const PI: float = 3.14;
    if (match(lexer::TokenType::Colon)) {
        type = parseType();
    }

    ast::ExprPtr init = nullptr;

    if (match(lexer::TokenType::Equal)) {
        init = parseExpression();
    }

    consume(lexer::TokenType::Semicolon, "Expected ';' after variable declaration");
    return std::make_unique<ast::VarDeclStmt>(type, nameTok.lexeme, std::move(init), isConst, isVar, loc, visibility.empty() ? "public" : visibility, isStatic, isWeak);
}

ast::StmtPtr Parser::parseIfStatement() {
    diagnostics::SourceLocation loc = previous().location;
    consume(lexer::TokenType::OpenParen, "Expected '(' after 'if'");
    ast::ExprPtr condition = parseExpression();
    // Phase 5 slice C: no condition, no if. Return null instead of an
    // IfStmt with a null condition (which used to segfault analyze()),
    // and do NOT synchronize: the failed expression parse already
    // advanced past the offending token, so the next real statement
    // must not be swallowed as this if's then-branch.
    if (!condition) return nullptr;
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
    // Phase 5 slice C: no condition, no loop (same as parseIfStatement).
    if (!condition) return nullptr;
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
                // Phase 5 slice C: a failed statement parse returns null;
                // do not keep a null entry in the case body.
                if (auto stmt = parseStatement()) stmts.push_back(std::move(stmt));
            }
            cases.emplace_back(std::move(val), std::move(stmts));
        } else if (match(lexer::TokenType::KeywordDefault)) {
            consume(lexer::TokenType::Colon, "Expected ':' after default");
            while (!isAtEnd() && !check(lexer::TokenType::KeywordCase) &&
                   !check(lexer::TokenType::KeywordDefault) && !check(lexer::TokenType::CloseBrace)) {
                // Phase 5 slice C: same null-skip as the case bodies above.
                if (auto stmt = parseStatement()) defaultStmts.push_back(std::move(stmt));
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
        if (check(lexer::TokenType::Identifier) && peek().lexeme == "_") {
            advance();
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
        // Phase 5 slice C: a failed statement parse returns null; do not
        // keep null entries in the block (they used to surface as
        // phantom null ExprStmts to later passes).
        if (auto stmt = parseStatement()) statements.push_back(std::move(stmt));
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' to close block");
    return std::make_unique<ast::BlockStmt>(std::move(statements), loc);
}

ast::StmtPtr Parser::parseExprStatement() {
    diagnostics::SourceLocation loc = peek().location;
    ast::ExprPtr expr = parseExpression();
    // Phase 5 slice C: no expression, no statement. A lone `;` used to
    // produce an ExprStmt with a null expression (which segfaulted
    // analyze()); now it produces nothing and the caller skips it.
    if (!expr) return nullptr;
    consume(lexer::TokenType::Semicolon, "Expected ';' after expression");
    return std::make_unique<ast::ExprStmt>(std::move(expr), loc);
}

ast::ASTNodePtr Parser::parseDeclaration() {
    std::vector<ast::Annotation> annotations;
    while (check(lexer::TokenType::At)) {
        auto anns = parseAnnotations();
        annotations.insert(annotations.end(), std::make_move_iterator(anns.begin()), std::make_move_iterator(anns.end()));
    }

    auto attachAnnotations = [&](ast::ASTNodePtr node) -> ast::ASTNodePtr {
        if (!annotations.empty() && node) {
            if (auto* varDecl = dynamic_cast<ast::VarDeclStmt*>(node.get())) {
                varDecl->setAnnotations(annotations);
            } else if (auto* classDecl = dynamic_cast<ast::ClassDecl*>(node.get())) {
                classDecl->setAnnotations(annotations);
            } else if (auto* fnDecl = dynamic_cast<ast::FunctionDecl*>(node.get())) {
                fnDecl->setAnnotations(annotations);
            }
        }
        return node;
    };

    bool isReprC = false;
    if (match(lexer::TokenType::Hash)) {
        consume(lexer::TokenType::OpenBracket, "Expected '[' after '#'");
        const lexer::Token& attrName = consume(lexer::TokenType::Identifier, "Expected attribute name");
        if (attrName.lexeme == "repr") {
            consume(lexer::TokenType::OpenParen, "Expected '(' after 'repr'");
            const lexer::Token& reprArg = consume(lexer::TokenType::Identifier, "Expected repr argument (e.g. C)");
            if (reprArg.lexeme == "C") {
                isReprC = true;
            }
            consume(lexer::TokenType::CloseParen, "Expected ')' after repr argument");
        }
        consume(lexer::TokenType::CloseBracket, "Expected ']' after attribute");
    }

    std::string visibility;
    bool isStatic = false;
    bool isOverride = false;
    bool isAbstract = false;
    bool isWeak = false;
    bool isUnsafe = false;
    bool isAsync = false;
    bool isComptime = false;

    // Modifiers
    while (true) {
        if (check(lexer::TokenType::At)) {
            auto anns = parseAnnotations();
            annotations.insert(annotations.end(), std::make_move_iterator(anns.begin()), std::make_move_iterator(anns.end()));
            continue;
        }
        if (match(lexer::TokenType::KeywordPublic)) { visibility = "public"; continue; }
        if (match(lexer::TokenType::KeywordPrivate)) { visibility = "private"; continue; }
        if (match(lexer::TokenType::KeywordProtected)) { visibility = "protected"; continue; }
        if (match(lexer::TokenType::KeywordInternal)) { visibility = "internal"; continue; }
        if (match(lexer::TokenType::KeywordStatic)) { isStatic = true; continue; }
        if (match(lexer::TokenType::KeywordOverride)) { isOverride = true; continue; }
        if (match(lexer::TokenType::KeywordAbstract)) { isAbstract = true; continue; }
        if (match(lexer::TokenType::KeywordWeak)) { isWeak = true; continue; }
        if (match(lexer::TokenType::KeywordUnsafe)) { isUnsafe = true; continue; }
        if (match(lexer::TokenType::KeywordAsync)) { isAsync = true; continue; }
        if (match(lexer::TokenType::KeywordComptime)) { isComptime = true; continue; }
        break;
    }

    if (isUnsafe && check(lexer::TokenType::OpenBrace)) {
        auto block = parseBlock();
        if (block) block->setUnsafe(true);
        return attachAnnotations(std::move(block));
    }

    if (isComptime && check(lexer::TokenType::OpenBrace)) {
        diagnostics::SourceLocation loc = previous().location;
        auto block = parseBlock();
        return attachAnnotations(std::make_unique<ast::ComptimeExpr>(std::move(block), loc));
    }

    if (match(lexer::TokenType::KeywordModule)) return attachAnnotations(parseModuleDecl());
    if (match(lexer::TokenType::KeywordImport)) return attachAnnotations(parseImportDecl());
    if (match(lexer::TokenType::KeywordFn)) return attachAnnotations(parseFunctionDecl(visibility, isStatic, isOverride, isAbstract, isUnsafe, isAsync, isComptime));
    if (match(lexer::TokenType::KeywordClass)) return attachAnnotations(parseClassDecl(visibility, isAbstract));
    if (match(lexer::TokenType::KeywordInterface)) return attachAnnotations(parseInterfaceDecl(visibility));
    if (match(lexer::TokenType::KeywordStruct)) return attachAnnotations(parseStructDecl(visibility, isReprC));
    if (match(lexer::TokenType::KeywordEnum)) return attachAnnotations(parseEnumDecl(visibility));
    if (match(lexer::TokenType::KeywordTest)) return attachAnnotations(parseTestDecl());

    // Variable declarations with visibility or const/var
    if (match(lexer::TokenType::KeywordConst)) return attachAnnotations(parseVarDecl(true, false, isWeak, visibility, isStatic));
    if (match(lexer::TokenType::KeywordVar)) return attachAnnotations(parseVarDecl(false, true, isWeak, visibility, isStatic));
    if (match(lexer::TokenType::KeywordLet)) return attachAnnotations(parseVarDecl(false, true, isWeak, visibility, isStatic));

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
        return attachAnnotations(std::make_unique<ast::BlockStmt>(std::move(externDecls), loc));
    }

    // Short-form function: main() { ... } or func() -> int { ... } without fn keyword
    if (check(lexer::TokenType::Identifier) && checkNext(lexer::TokenType::OpenParen)) {
        size_t lookahead = m_current + 1;
        int parenDepth = 1;
        lookahead++;
        while (lookahead < m_tokens.size() && parenDepth > 0) {
            if (m_tokens[lookahead].is(lexer::TokenType::OpenParen)) parenDepth++;
            else if (m_tokens[lookahead].is(lexer::TokenType::CloseParen)) parenDepth--;
            lookahead++;
        }
        if (lookahead < m_tokens.size()) {
            if (m_tokens[lookahead].is(lexer::TokenType::OpenBrace) ||
                m_tokens[lookahead].is(lexer::TokenType::Arrow) ||
                m_tokens[lookahead].is(lexer::TokenType::FatArrow)) {
                const lexer::Token& nameTok = advance();
                diagnostics::SourceLocation loc = nameTok.location;
                std::vector<ast::Parameter> params = parseParameters();
                std::string returnType = "void";
                if (match(lexer::TokenType::Arrow)) {
                    returnType = parseType();
                }
                std::unique_ptr<ast::BlockStmt> body = nullptr;
                std::unique_ptr<ast::ExpressionNode> exprBody = nullptr;
                if (match(lexer::TokenType::FatArrow)) {
                    exprBody = parseExpression();
                    consume(lexer::TokenType::Semicolon, "Expected ';' after expression body");
                } else {
                    body = parseBlock();
                }
                return attachAnnotations(std::make_unique<ast::FunctionDecl>(
                    nameTok.lexeme, std::move(params), returnType,
                    std::move(body), std::move(exprBody), visibility, isStatic, isOverride, isAbstract, false, loc));
            }
        }
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

                return attachAnnotations(std::make_unique<ast::FunctionDecl>(
                    nameTok.lexeme, std::move(params), type,
                    std::move(body), nullptr, visibility, isStatic, isOverride, isAbstract, false, loc));
            } else if (checkNext(lexer::TokenType::Equal) || checkNext(lexer::TokenType::Semicolon) || checkNext(lexer::TokenType::Comma)) {
                // Field or global variable: [public/protected] int count = 0;
                const lexer::Token& nameTok = advance();
                diagnostics::SourceLocation loc = nameTok.location;
                ast::ExprPtr init = nullptr;
                if (match(lexer::TokenType::Equal)) {
                    init = parseExpression();
                }
                consume(lexer::TokenType::Semicolon, "Expected ';' after variable declaration");
                return attachAnnotations(std::make_unique<ast::VarDeclStmt>(type, nameTok.lexeme, std::move(init), false, false, loc, visibility, isStatic, isWeak));
            }
        }
        m_current = saved; // Not a declaration, rewind
    }

    return attachAnnotations(parseStatement());
}

ast::ASTNodePtr Parser::parseModuleDecl() {
    diagnostics::SourceLocation loc = previous().location;
    std::string moduleName;

    const lexer::Token& part = isModulePathSegment(peek().type)
        ? advance()
        : consume(lexer::TokenType::Identifier, "Expected module identifier");
    moduleName += part.lexeme;

    while (match(lexer::TokenType::Dot)) {
        moduleName += ".";
        const lexer::Token& subPart = isModulePathSegment(peek().type)
            ? advance()
            : consume(lexer::TokenType::Identifier, "Expected identifier after '.' in module name");
        moduleName += subPart.lexeme;
    }

    consume(lexer::TokenType::Semicolon, "Expected ';' after module declaration");
    return std::make_unique<ast::ModuleDecl>(moduleName, loc);
}

ast::ASTNodePtr Parser::parseImportDecl() {
    diagnostics::SourceLocation loc = previous().location;
    std::string importPath;
    bool isWildcard = false;

    const lexer::Token& part = isModulePathSegment(peek().type)
        ? advance()
        : consume(lexer::TokenType::Identifier, "Expected import path");
    importPath += part.lexeme;

    while (match(lexer::TokenType::Dot)) {
        if (match(lexer::TokenType::Star)) {
            isWildcard = true;
            break;
        }
        importPath += ".";
        const lexer::Token& subPart = isModulePathSegment(peek().type)
            ? advance()
            : consume(lexer::TokenType::Identifier, "Expected identifier after '.' in import path");
        importPath += subPart.lexeme;
    }

    consume(lexer::TokenType::Semicolon, "Expected ';' after import statement");
    return std::make_unique<ast::ImportDecl>(importPath, isWildcard, loc);
}

ast::ASTNodePtr Parser::parseFunctionDecl(std::string visibility, bool isStatic, bool isOverride, bool isAbstract, bool isUnsafe, bool isAsync, bool isComptime) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = (check(lexer::TokenType::Identifier) || isTypeKeyword(peek().type) || peek().type == lexer::TokenType::KeywordSpawn)
        ? advance()
        : consume(lexer::TokenType::Identifier, "Expected function name");
    std::string name = nameTok.lexeme;

    // Phase 5 slice C: a failed name consume leaves the token stream
    // untouched, so without this the parser would return a nameless
    // FunctionDecl AND re-parse the leftover keyword as something else
    // (`fn test()` produced a nameless fn plus a phantom
    // `__sp_test_anonymous_test`). Drop the declaration and skip to the
    // next declaration boundary instead.
    if (name.empty()) {
        synchronize();
        return nullptr;
    }

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
    if (match(lexer::TokenType::Arrow) || match(lexer::TokenType::Colon)) {
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

    auto fnDecl = std::make_unique<ast::FunctionDecl>(
        name, std::move(typeParams), std::move(params), returnType,
        std::move(body), std::move(shortBody),
        visibility, isStatic, isOverride, isAbstract, isAsync, loc);
    fnDecl->setUnsafe(isUnsafe);
    fnDecl->setComptime(isComptime);
    return fnDecl;
}

ast::ASTNodePtr Parser::parseClassDecl(std::string visibility, bool isAbstract) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected class name");
    std::string name = nameTok.lexeme;

    // Phase 5 slice C: no name, no declaration (same as parseFunctionDecl).
    if (name.empty()) {
        synchronize();
        return nullptr;
    }

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

        // Phase 5 slice C: a failed member parse returns null; do not
        // keep a null member (member parsers can now return null).
        if (auto member = parseDeclaration()) members.push_back(std::move(member));
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of class body");

    return std::make_unique<ast::ClassDecl>(name, std::move(typeParams), baseClass, std::move(interfaces), std::move(members), visibility, isAbstract, loc);
}

ast::ASTNodePtr Parser::parseInterfaceDecl(std::string visibility) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected interface name");
    std::string name = nameTok.lexeme;

    // Phase 5 slice C: no name, no declaration (same as parseFunctionDecl).
    if (name.empty()) {
        synchronize();
        return nullptr;
    }

    consume(lexer::TokenType::OpenBrace, "Expected '{' to start interface body");

    std::vector<std::unique_ptr<ast::FunctionDecl>> methods;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        if (match(lexer::TokenType::KeywordFn)) {
            const lexer::Token& fnName = consume(lexer::TokenType::Identifier, "Expected method name in interface");
            // Phase 5 slice C: no method name, no method. Synchronize
            // (the failed consume did not advance) and try the next one.
            if (fnName.lexeme.empty()) {
                synchronize();
                continue;
            }
            diagnostics::SourceLocation fnLoc = fnName.location;
            std::vector<ast::Parameter> params = parseParameters();
            std::string returnType = "void";
            if (match(lexer::TokenType::Arrow) || match(lexer::TokenType::Colon)) {
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

ast::ASTNodePtr Parser::parseStructDecl(std::string visibility, bool isReprC) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected struct name");
    std::string name = nameTok.lexeme;

    // Phase 5 slice C: no name, no declaration (same as parseFunctionDecl).
    if (name.empty()) {
        synchronize();
        return nullptr;
    }

    consume(lexer::TokenType::OpenBrace, "Expected '{' to start struct body");

    std::vector<std::unique_ptr<ast::VarDeclStmt>> fields;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        std::string type;
        std::string fieldName;
        diagnostics::SourceLocation fieldLoc = peek().location;

        if (check(lexer::TokenType::Identifier) && checkNext(lexer::TokenType::Colon)) {
            fieldName = advance().lexeme;
            consume(lexer::TokenType::Colon, "Expected ':' after field name");
            type = parseType();
        } else {
            type = parseType();
            const lexer::Token& fTok = consume(lexer::TokenType::Identifier, "Expected field name in struct");
            fieldName = fTok.lexeme;
            fieldLoc = fTok.location;
        }

        if (!match(lexer::TokenType::Semicolon) && !match(lexer::TokenType::Comma)) {
            if (!check(lexer::TokenType::CloseBrace) && !isAtEnd()) {
                advance();
            }
        }
        // Phase 5 slice C: a failed field-name consume yields an empty
        // name; do not keep a phantom field for it.
        if (!fieldName.empty()) {
            fields.push_back(std::make_unique<ast::VarDeclStmt>(type, fieldName, nullptr, false, false, fieldLoc));
        }
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of struct body");
    return std::make_unique<ast::StructDecl>(name, std::move(fields), visibility, isReprC, loc);
}

ast::ASTNodePtr Parser::parseEnumDecl(std::string visibility) {
    diagnostics::SourceLocation loc = previous().location;
    const lexer::Token& nameTok = consume(lexer::TokenType::Identifier, "Expected enum name");
    std::string name = nameTok.lexeme;

    // Phase 5 slice C: no name, no declaration (same as parseFunctionDecl).
    if (name.empty()) {
        synchronize();
        return nullptr;
    }

    std::vector<std::string> typeParams;
    if (match(lexer::TokenType::Less)) {
        do {
            const lexer::Token& paramTok = consume(lexer::TokenType::Identifier, "Expected type parameter name");
            typeParams.push_back(paramTok.lexeme);
        } while (match(lexer::TokenType::Comma));
        consume(lexer::TokenType::Greater, "Expected '>' after generic type parameters");
    }

    consume(lexer::TokenType::OpenBrace, "Expected '{' to start enum body");

    std::vector<ast::EnumVariant> variants;

    while (!isAtEnd() && !check(lexer::TokenType::CloseBrace)) {
        const lexer::Token& varTok = consume(lexer::TokenType::Identifier, "Expected enum variant identifier");
        std::vector<std::string> payloadTypes;

        if (match(lexer::TokenType::OpenParen)) {
            if (!check(lexer::TokenType::CloseParen)) {
                do {
                    payloadTypes.push_back(parseType());
                } while (match(lexer::TokenType::Comma));
            }
            consume(lexer::TokenType::CloseParen, "Expected ')' after enum variant payload types");
        }

        ast::ExprPtr val = nullptr;
        if (match(lexer::TokenType::Equal)) {
            val = parseExpression();
        }

        // Phase 5 slice C: a failed variant-name consume yields an empty
        // name; do not keep a phantom variant for it.
        if (!varTok.lexeme.empty()) {
            variants.emplace_back(varTok.lexeme, std::move(val), std::move(payloadTypes));
        }

        if (!match(lexer::TokenType::Comma)) {
            break;
        }
    }

    consume(lexer::TokenType::CloseBrace, "Expected '}' at end of enum body");
    return std::make_unique<ast::EnumDecl>(name, std::move(typeParams), std::move(variants), visibility, loc);
}

ast::ASTNodePtr Parser::parseTestDecl() {
    diagnostics::SourceLocation loc = previous().location;
    std::string testName;
    bool nameOk = true;

    if (match(lexer::TokenType::StringLiteral)) {
        testName = previous().lexeme;
        if (testName.size() >= 2 && testName.front() == '"' && testName.back() == '"') {
            testName = testName.substr(1, testName.size() - 2);
        }
    } else if (match(lexer::TokenType::Identifier)) {
        testName = previous().lexeme;
    } else {
        errorAt(peek(), "Expected test name string literal or identifier after 'test'");
        testName = "anonymous_test";
        nameOk = false;
    }

    if (match(lexer::TokenType::OpenParen)) {
        consume(lexer::TokenType::CloseParen, "Expected ')' after test declaration parameters");
    }

    std::unique_ptr<ast::BlockStmt> body;
    bool bodyOk = false;
    if (check(lexer::TokenType::OpenBrace)) {
        body = parseBlock();
        bodyOk = true;
    } else {
        errorAt(peek(), "Expected '{' to start test body");
        body = std::make_unique<ast::BlockStmt>(std::vector<ast::StmtPtr>{}, loc);
    }

    // Phase 5 slice C: a test declaration with neither a name nor a
    // body is not a declaration at all — it is leftover garbage from a
    // failed parse elsewhere (e.g. the `test` keyword left behind by a
    // failed `fn test()`). Drop it instead of emitting a phantom
    // `__sp_test_anonymous_test`. The anonymous-test fallback stays
    // when a body IS present: `test { ... }` is a real test.
    if (!nameOk && !bodyOk) {
        synchronize();
        return nullptr;
    }

    std::string funcName = "__sp_test_";
    for (char c : testName) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            funcName += c;
        } else {
            funcName += '_';
        }
    }

    return std::make_unique<ast::FunctionDecl>(
        funcName,
        std::vector<std::string>{},
        std::vector<ast::Parameter>{},
        "void",
        std::move(body),
        nullptr,
        "public",
        false, // static
        false, // override
        false, // abstract
        false, // async
        loc
    );
}

std::string Parser::parseType() {
    if (match(lexer::TokenType::Star)) {
        return "ptr<" + parseType() + ">";
    }

    std::string type;
    if (isTypeKeyword(peek().type)) {
        type = advance().lexeme;
    } else if (match(lexer::TokenType::Identifier)) {
        type = previous().lexeme;
    } else {
        errorAt(peek(), "Expected type name");
        if (!isAtEnd()) advance();
        return "unknown";
    }

    // Generic type parameters: List<int>, Map<string, User>, ptr<ptr<T>>
    if (match(lexer::TokenType::Less)) {
        type += "<";
        do {
            type += parseType();
            if (check(lexer::TokenType::Comma)) {
                type += ", ";
                advance();
            }
        } while (!isAtEnd() && !check(lexer::TokenType::Greater) && !check(lexer::TokenType::GreaterGreater));

        if (match(lexer::TokenType::Greater)) {
            type += ">";
        } else {
            errorAt(peek(), "Expected '>' after generic type arguments");
            if (!isAtEnd()) advance();
            type += ">";
        }
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

bool Parser::isModulePathSegment(lexer::TokenType type) noexcept {
    if (type == lexer::TokenType::Identifier) return true;
    if (isTypeKeyword(type)) return true;
    if (type >= lexer::TokenType::KeywordFn && type <= lexer::TokenType::KeywordAssert) return true;
    return false;
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
