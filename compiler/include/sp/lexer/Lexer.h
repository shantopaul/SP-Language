#pragma once

#include "sp/lexer/Token.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <string>
#include <string_view>
#include <vector>

namespace sp::lexer {

class Lexer {
public:
    Lexer(std::string_view source, std::string filename = "<anonymous>", diagnostics::DiagnosticEngine* diagnostics = nullptr);

    /// Scans and returns the next token in the stream.
    Token nextToken();

    /// Scans the entire source and returns all tokens up to and including EndOfFile.
    std::vector<Token> tokenizeAll();

    /// Set whether the lexer emits Comment and DocComment tokens or skips them.
    void setPreserveComments(bool preserve) noexcept { m_preserveComments = preserve; }
    [[nodiscard]] bool preservesComments() const noexcept { return m_preserveComments; }

    [[nodiscard]] const std::string& getFilename() const noexcept { return m_filename; }

private:
    std::string_view m_source;
    std::string m_filename;
    diagnostics::DiagnosticEngine* m_diagnostics{nullptr};

    size_t m_start{0};
    size_t m_current{0};
    uint32_t m_line{1};
    uint32_t m_column{1};
    uint32_t m_tokenStartLine{1};
    uint32_t m_tokenStartColumn{1};
    bool m_preserveComments{false};

    // Character inspection and traversal
    [[nodiscard]] bool isAtEnd() const noexcept;
    char advance() noexcept;
    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peekNext() const noexcept;
    bool match(char expected) noexcept;

    void skipWhitespace();

    // Specific token scanners
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    Token scanChar();
    Token scanSingleLineComment();
    Token scanMultiLineComment();

    // Token creation helpers
    Token makeToken(TokenType type) const;
    Token makeToken(TokenType type, std::string lexeme) const;
    Token errorToken(std::string_view code, std::string_view message, std::string_view hint = "");

    static TokenType lookupKeyword(std::string_view text) noexcept;
};

} // namespace sp::lexer
