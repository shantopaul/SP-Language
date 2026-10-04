#pragma once

#include "sp/lexer/TokenType.h"
#include "sp/diagnostics/SourceLocation.h"
#include <string>
#include <string_view>

namespace sp::lexer {

struct Token {
    TokenType type{TokenType::Invalid};
    std::string lexeme;
    diagnostics::SourceLocation location;

    Token() = default;
    Token(TokenType t, std::string lex, diagnostics::SourceLocation loc)
        : type(t), lexeme(std::move(lex)), location(std::move(loc)) {}

    [[nodiscard]] bool is(TokenType expected) const noexcept {
        return type == expected;
    }

    [[nodiscard]] bool isNot(TokenType expected) const noexcept {
        return type != expected;
    }

    [[nodiscard]] std::string toString() const;
};

} // namespace sp::lexer
