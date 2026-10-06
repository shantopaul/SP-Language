#include "Formatter.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/lexer/Token.h"
#include "sp/lexer/TokenType.h"

#include <sstream>
#include <vector>
#include <algorithm>

namespace sp::fmt {

using namespace sp::lexer;

Formatter::Formatter(FormatOptions options)
    : m_options(options) {}

namespace {

bool isKeyword(TokenType type) {
    return static_cast<int>(type) >= static_cast<int>(TokenType::KeywordFn) &&
           static_cast<int>(type) <= static_cast<int>(TokenType::KeywordByte);
}

bool isBinaryOperator(TokenType type) {
    switch (type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
        case TokenType::Equal:
        case TokenType::PlusEqual:
        case TokenType::MinusEqual:
        case TokenType::StarEqual:
        case TokenType::SlashEqual:
        case TokenType::PercentEqual:
        case TokenType::EqualEqual:
        case TokenType::BangEqual:
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
        case TokenType::AmpAmp:
        case TokenType::PipePipe:
        case TokenType::Amp:
        case TokenType::Pipe:
        case TokenType::Caret:
        case TokenType::LessLess:
        case TokenType::GreaterGreater:
        case TokenType::QuestionQuestion:
        case TokenType::Arrow:
        case TokenType::FatArrow:
            return true;
        default:
            return false;
    }
}

bool isKeywordControlStmt(TokenType type) {
    return type == TokenType::KeywordIf ||
           type == TokenType::KeywordWhile ||
           type == TokenType::KeywordFor ||
           type == TokenType::KeywordSwitch ||
           type == TokenType::KeywordCatch ||
           type == TokenType::KeywordMatch;
}

} // namespace

std::string Formatter::format(const std::string& sourceCode) {
    if (sourceCode.empty()) return "";

    std::ostringstream diagStream;
    sp::diagnostics::DiagnosticEngine diagEngine(diagStream);

    sp::lexer::Lexer lexer(sourceCode, "<format>", &diagEngine);
    lexer.setPreserveComments(true);
    auto tokens = lexer.tokenizeAll();

    if (diagEngine.hasErrors() || tokens.empty()) {
        // Return untouched if lexical errors exist
        return sourceCode;
    }

    std::string result;
    result.reserve(sourceCode.size() + 256);

    int indentLevel = 0;
    bool atStartOfLine = true;
    int parenDepth = 0;
    int forParenDepth = -1;
    int bracketDepth = 0;

    auto emitIndent = [&](int level) {
        int target = std::max(0, level);
        if (m_options.useTabs) {
            result.append(target, '\t');
        } else {
            result.append(target * m_options.indentWidth, ' ');
        }
        atStartOfLine = false;
    };

    bool lastWasUnaryOp = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        if (tok.type == TokenType::EndOfFile) {
            break;
        }

        const Token* prev = (i > 0) ? &tokens[i - 1] : nullptr;
        const Token* next = (i + 1 < tokens.size() && tokens[i + 1].type != TokenType::EndOfFile) ? &tokens[i + 1] : nullptr;

        // Preserve empty lines from original source (up to maxEmptyLines)
        if (prev != nullptr && tok.location.line > prev->location.line + 1) {
            int emptyLines = std::min(static_cast<int>(tok.location.line - prev->location.line - 1), m_options.maxEmptyLines);
            for (int k = 0; k < emptyLines; ++k) {
                if (!result.empty() && result.back() != '\n') {
                    result += '\n';
                }
                result += '\n';
                atStartOfLine = true;
            }
        }

        // 1. Handling CloseBrace '}'
        if (tok.type == TokenType::CloseBrace) {
            if (!atStartOfLine) {
                result += '\n';
                atStartOfLine = true;
            }
            indentLevel = std::max(0, indentLevel - 1);
            emitIndent(indentLevel);
            result += "}";

            if (next != nullptr) {
                if (next->type == TokenType::KeywordElse ||
                    next->type == TokenType::KeywordCatch ||
                    next->type == TokenType::KeywordFinally) {
                    result += " ";
                } else if (next->type == TokenType::Semicolon ||
                           next->type == TokenType::Comma ||
                           next->type == TokenType::CloseParen) {
                    // No newline immediately
                } else {
                    result += '\n';
                    atStartOfLine = true;
                }
            } else {
                result += '\n';
                atStartOfLine = true;
            }
            lastWasUnaryOp = false;
            continue;
        }

        // 2. Handling OpenBrace '{'
        if (tok.type == TokenType::OpenBrace) {
            if (!atStartOfLine) {
                if (!result.empty() && result.back() != ' ' && result.back() != '\n') {
                    result += ' ';
                }
            } else {
                emitIndent(indentLevel);
            }
            result += "{";
            indentLevel++;
            result += '\n';
            atStartOfLine = true;
            lastWasUnaryOp = false;
            continue;
        }

        // 3. Handling Semicolon ';'
        if (tok.type == TokenType::Semicolon) {
            result += ";";
            if (parenDepth > 0 && forParenDepth >= 0) {
                result += " ";
            } else {
                result += '\n';
                atStartOfLine = true;
            }
            lastWasUnaryOp = false;
            continue;
        }

        // 4. Handling Comments
        if (tok.type == TokenType::Comment || tok.type == TokenType::DocComment) {
            if (!atStartOfLine) {
                if (!result.empty() && result.back() != ' ') {
                    result += " ";
                }
            } else {
                emitIndent(indentLevel);
            }
            result += tok.lexeme;
            result += '\n';
            atStartOfLine = true;
            lastWasUnaryOp = false;
            continue;
        }

        // 5. Parentheses & Brackets tracking
        if (tok.type == TokenType::OpenParen) {
            if (prev != nullptr && isKeywordControlStmt(prev->type)) {
                if (!result.empty() && result.back() != ' ') {
                    result += " ";
                }
            }
            if (prev != nullptr && prev->type == TokenType::KeywordFor) {
                forParenDepth = parenDepth;
            }
            parenDepth++;
        } else if (tok.type == TokenType::CloseParen) {
            parenDepth = std::max(0, parenDepth - 1);
            if (parenDepth < forParenDepth) {
                forParenDepth = -1;
            }
        } else if (tok.type == TokenType::OpenBracket) {
            bracketDepth++;
        } else if (tok.type == TokenType::CloseBracket) {
            bracketDepth = std::max(0, bracketDepth - 1);
        }

        // Check if current token is a unary prefix operator
        bool isUnaryOp = false;
        if (tok.type == TokenType::Minus || tok.type == TokenType::Plus ||
            tok.type == TokenType::Bang || tok.type == TokenType::Tilde ||
            tok.type == TokenType::PlusPlus || tok.type == TokenType::MinusMinus) {
            if (prev == nullptr || isBinaryOperator(prev->type) ||
                prev->type == TokenType::OpenParen || prev->type == TokenType::OpenBracket ||
                prev->type == TokenType::Comma || prev->type == TokenType::Colon ||
                prev->type == TokenType::KeywordReturn) {
                isUnaryOp = true;
            }
        }

        // 6. Regular tokens indentation & spacing
        if (atStartOfLine) {
            emitIndent(indentLevel);
        } else {
            bool needSpace = false;

            if (prev != nullptr) {
                if (tok.type == TokenType::CloseParen ||
                    tok.type == TokenType::CloseBracket ||
                    tok.type == TokenType::Comma ||
                    tok.type == TokenType::Semicolon) {
                    needSpace = false;
                } else if (tok.type == TokenType::Colon &&
                           (prev->type == TokenType::CloseParen || prev->type == TokenType::Identifier)) {
                    needSpace = false; // fn foo(): void or let x: int
                } else if (lastWasUnaryOp) {
                    needSpace = false; // No space between unary op and operand
                } else if (isUnaryOp) {
                    // Space before unary op if following binary op or keyword
                    if (isBinaryOperator(prev->type) || isKeyword(prev->type)) {
                        needSpace = true;
                    }
                } else if (prev->type == TokenType::Comma) {
                    needSpace = m_options.spaceAfterComma;
                } else if (prev->type == TokenType::Colon) {
                    needSpace = m_options.spaceAfterColon;
                } else if (isKeyword(prev->type)) {
                    needSpace = true;
                } else if (m_options.spaceAroundBinaryOps && isBinaryOperator(tok.type)) {
                    needSpace = true;
                } else if (m_options.spaceAroundBinaryOps && isBinaryOperator(prev->type)) {
                    needSpace = true;
                } else if (tok.type == TokenType::Question || prev->type == TokenType::Question) {
                    needSpace = true; // ternary
                } else if (prev->type == TokenType::CloseParen) {
                    if (tok.type != TokenType::Dot &&
                        tok.type != TokenType::QuestionDot &&
                        tok.type != TokenType::CloseBracket) {
                        needSpace = true;
                    }
                } else if (prev->type == TokenType::CloseBracket) {
                    if (tok.type != TokenType::Dot &&
                        tok.type != TokenType::QuestionDot &&
                        tok.type != TokenType::CloseBracket) {
                        needSpace = true;
                    }
                } else if (tok.type == TokenType::OpenBrace) {
                    needSpace = true;
                }
            }

            if (needSpace && !result.empty() && result.back() != ' ' && result.back() != '\n') {
                result += " ";
            }
        }

        lastWasUnaryOp = isUnaryOp;
        result += tok.lexeme;
    }

    // Ensure trailing newline at end of file if not empty
    if (!result.empty() && result.back() != '\n') {
        result += '\n';
    }

    return result;
}

bool Formatter::isFormatted(const std::string& sourceCode) {
    return format(sourceCode) == sourceCode;
}

} // namespace sp::fmt
