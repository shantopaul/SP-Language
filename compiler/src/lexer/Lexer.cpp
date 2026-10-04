#include "sp/lexer/Lexer.h"
#include <cctype>
#include <unordered_map>

namespace sp::lexer {

Lexer::Lexer(std::string_view source, std::string filename, diagnostics::DiagnosticEngine* diagnostics)
    : m_source(source), m_filename(std::move(filename)), m_diagnostics(diagnostics) {}

bool Lexer::isAtEnd() const noexcept {
    return m_current >= m_source.size();
}

char Lexer::advance() noexcept {
    char c = m_source[m_current++];
    if (c == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

char Lexer::peek() const noexcept {
    if (isAtEnd()) return '\0';
    return m_source[m_current];
}

char Lexer::peekNext() const noexcept {
    if (m_current + 1 >= m_source.size()) return '\0';
    return m_source[m_current + 1];
}

bool Lexer::match(char expected) noexcept {
    if (isAtEnd()) return false;
    if (m_source[m_current] != expected) return false;
    m_current++;
    m_column++;
    return true;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                advance();
                break;
            default:
                return;
        }
    }
}

Token Lexer::makeToken(TokenType type) const {
    std::string lexeme(m_source.substr(m_start, m_current - m_start));
    return Token(type, lexeme, diagnostics::SourceLocation(m_filename, m_tokenStartLine, m_tokenStartColumn));
}

Token Lexer::makeToken(TokenType type, std::string lexeme) const {
    return Token(type, std::move(lexeme), diagnostics::SourceLocation(m_filename, m_tokenStartLine, m_tokenStartColumn));
}

Token Lexer::errorToken(std::string_view code, std::string_view message, std::string_view hint) {
    diagnostics::SourceLocation loc(m_filename, m_tokenStartLine, m_tokenStartColumn);
    if (m_diagnostics) {
        m_diagnostics->reportError(code, message, loc, hint);
    }
    std::string lexeme(m_source.substr(m_start, m_current - m_start));
    return Token(TokenType::Invalid, lexeme, loc);
}

Token Lexer::nextToken() {
    while (true) {
        skipWhitespace();

        m_start = m_current;
        m_tokenStartLine = m_line;
        m_tokenStartColumn = m_column;

        if (isAtEnd()) {
            return makeToken(TokenType::EndOfFile, "");
        }

        char c = advance();

        // Identifiers and keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            return scanIdentifierOrKeyword();
        }

        // Numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            return scanNumber();
        }

        switch (c) {
            // Delimiters
            case '(': return makeToken(TokenType::OpenParen);
            case ')': return makeToken(TokenType::CloseParen);
            case '{': return makeToken(TokenType::OpenBrace);
            case '}': return makeToken(TokenType::CloseBrace);
            case '[': return makeToken(TokenType::OpenBracket);
            case ']': return makeToken(TokenType::CloseBracket);
            case ';': return makeToken(TokenType::Semicolon);
            case ':': return makeToken(TokenType::Colon);
            case ',': return makeToken(TokenType::Comma);
            case '@': return makeToken(TokenType::At);
            case '~': return makeToken(TokenType::Tilde);
            case '^': return makeToken(TokenType::Caret);

            // Dot and Range
            case '.':
                if (match('.')) {
                    return makeToken(TokenType::DotDot);
                }
                return makeToken(TokenType::Dot);

            // Question mark and null operators
            case '?':
                if (match('.')) {
                    return makeToken(TokenType::QuestionDot);
                }
                if (match('?')) {
                    return makeToken(TokenType::QuestionQuestion);
                }
                return makeToken(TokenType::Question);

            // Arithmetic & Compound Assignment
            case '+':
                if (match('+')) return makeToken(TokenType::PlusPlus);
                if (match('=')) return makeToken(TokenType::PlusEqual);
                return makeToken(TokenType::Plus);

            case '-':
                if (match('-')) return makeToken(TokenType::MinusMinus);
                if (match('>')) return makeToken(TokenType::Arrow);
                if (match('=')) return makeToken(TokenType::MinusEqual);
                return makeToken(TokenType::Minus);

            case '*':
                if (match('=')) return makeToken(TokenType::StarEqual);
                return makeToken(TokenType::Star);

            case '/':
                if (match('/')) {
                    Token commentTok = scanSingleLineComment();
                    if (m_preserveComments) return commentTok;
                    continue; // Skip comment and look for next token
                }
                if (match('*')) {
                    Token commentTok = scanMultiLineComment();
                    if (commentTok.is(TokenType::Invalid)) return commentTok;
                    if (m_preserveComments) return commentTok;
                    continue; // Skip comment and look for next token
                }
                if (match('=')) return makeToken(TokenType::SlashEqual);
                return makeToken(TokenType::Slash);

            case '%':
                if (match('=')) return makeToken(TokenType::PercentEqual);
                return makeToken(TokenType::Percent);

            // Logical & Bitwise
            case '&':
                if (match('&')) return makeToken(TokenType::AmpAmp);
                return makeToken(TokenType::Amp);

            case '|':
                if (match('|')) return makeToken(TokenType::PipePipe);
                return makeToken(TokenType::Pipe);

            // Equality & Assignment
            case '!':
                if (match('=')) return makeToken(TokenType::BangEqual);
                return makeToken(TokenType::Bang);

            case '=':
                if (match('=')) return makeToken(TokenType::EqualEqual);
                if (match('>')) return makeToken(TokenType::FatArrow);
                return makeToken(TokenType::Equal);

            // Comparison & Bitshift
            case '<':
                if (match('=')) return makeToken(TokenType::LessEqual);
                if (match('<')) return makeToken(TokenType::LessLess);
                return makeToken(TokenType::Less);

            case '>':
                if (match('=')) return makeToken(TokenType::GreaterEqual);
                if (match('>')) return makeToken(TokenType::GreaterGreater);
                return makeToken(TokenType::Greater);

            // Literals
            case '"':
                return scanString();

            case '\'':
                return scanChar();

            default: {
                std::string msg = "Unexpected character: '";
                msg += c;
                msg += "'";
                return errorToken("E1000", msg, "Remove or replace the invalid character");
            }
        }
    }
}

Token Lexer::scanIdentifierOrKeyword() {
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }

    std::string_view text = m_source.substr(m_start, m_current - m_start);
    TokenType type = lookupKeyword(text);
    return makeToken(type);
}

Token Lexer::scanNumber() {
    // Check for hex (0x...), binary (0b...), octal (0o...)
    if (m_source[m_start] == '0') {
        char next = peek();
        if (next == 'x' || next == 'X') {
            advance(); // consume 'x'
            if (isAtEnd() || !std::isxdigit(static_cast<unsigned char>(peek()))) {
                return errorToken("E1000", "Hexadecimal literal requires at least one hex digit", "Example: 0xFF");
            }
            while (!isAtEnd() && (std::isxdigit(static_cast<unsigned char>(peek())) || peek() == '_')) {
                advance();
            }
            return makeToken(TokenType::IntegerLiteral);
        }
        if (next == 'b' || next == 'B') {
            advance(); // consume 'b'
            if (isAtEnd() || (peek() != '0' && peek() != '1')) {
                return errorToken("E1000", "Binary literal requires at least one binary digit (0 or 1)", "Example: 0b1010");
            }
            while (!isAtEnd() && (peek() == '0' || peek() == '1' || peek() == '_')) {
                advance();
            }
            return makeToken(TokenType::IntegerLiteral);
        }
        if (next == 'o' || next == 'O') {
            advance(); // consume 'o'
            if (isAtEnd() || (peek() < '0' || peek() > '7')) {
                return errorToken("E1000", "Octal literal requires at least one octal digit (0-7)", "Example: 0o755");
            }
            while (!isAtEnd() && ((peek() >= '0' && peek() <= '7') || peek() == '_')) {
                advance();
            }
            return makeToken(TokenType::IntegerLiteral);
        }
    }

    // Standard decimal integer
    while (!isAtEnd() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }

    // Look for fractional part (ensure not range operator '..')
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance(); // consume '.'
        while (!isAtEnd() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_')) {
            advance();
        }

        // Scientific exponent (e.g. 1.25e+10 or 1.25e-3)
        if (peek() == 'e' || peek() == 'E') {
            advance();
            if (peek() == '+' || peek() == '-') {
                advance();
            }
            if (isAtEnd() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                return errorToken("E1000", "Floating-point exponent requires digits", "Example: 1.5e-3");
            }
            while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }
        return makeToken(TokenType::FloatLiteral);
    }

    // Scientific exponent on integer (e.g. 1e6)
    if (peek() == 'e' || peek() == 'E') {
        advance();
        if (peek() == '+' || peek() == '-') {
            advance();
        }
        if (isAtEnd() || !std::isdigit(static_cast<unsigned char>(peek()))) {
            return errorToken("E1000", "Floating-point exponent requires digits", "Example: 1e6");
        }
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        return makeToken(TokenType::FloatLiteral);
    }

    return makeToken(TokenType::IntegerLiteral);
}

Token Lexer::scanString() {
    std::string value;

    while (!isAtEnd() && peek() != '"') {
        char c = advance();

        if (c == '\n') {
            return errorToken("E1000", "Unclosed string literal before newline", "Close the string with a double quote '\"'");
        }

        if (c == '\\') {
            if (isAtEnd()) {
                return errorToken("E1000", "Unfinished escape sequence at end of file");
            }
            char escapeChar = advance();
            switch (escapeChar) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case 'r':  value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"':  value += '"'; break;
                case '\'': value += '\''; break;
                case '0':  value += '\0'; break;
                case 'u': {
                    // Unicode 4-digit hex sequence \uXXXX
                    std::string hex;
                    for (int i = 0; i < 4; i++) {
                        if (isAtEnd() || !std::isxdigit(static_cast<unsigned char>(peek()))) {
                            return errorToken("E1000", "Invalid unicode escape sequence, expected 4 hex digits", "Example: \\u0041");
                        }
                        hex += advance();
                    }
                    value += "\\u" + hex;
                    break;
                }
                default:
                    value += '\\';
                    value += escapeChar;
                    break;
            }
        } else {
            value += c;
        }
    }

    if (isAtEnd()) {
        return errorToken("E1000", "Unclosed string literal at end of file", "Add closing '\"' at end of string");
    }

    advance(); // Consume closing '"'
    return makeToken(TokenType::StringLiteral, value);
}

Token Lexer::scanChar() {
    if (isAtEnd() || peek() == '\'') {
        return errorToken("E1000", "Empty character literal", "Character literals must contain exactly one character");
    }

    char c = advance();
    std::string val;

    if (c == '\\') {
        if (isAtEnd()) {
            return errorToken("E1000", "Unfinished escape sequence in character literal");
        }
        char esc = advance();
        switch (esc) {
            case 'n':  val = "\n"; break;
            case 't':  val = "\t"; break;
            case 'r':  val = "\r"; break;
            case '\\': val = "\\"; break;
            case '\'': val = "'"; break;
            case '"':  val = "\""; break;
            case '0':  val = std::string(1, '\0'); break;
            default:   val = std::string(1, esc); break;
        }
    } else {
        val = std::string(1, c);
    }

    if (isAtEnd() || peek() != '\'') {
        return errorToken("E1000", "Unclosed character literal", "Close the character with a single quote '\'");
    }

    advance(); // Consume closing '\''
    return makeToken(TokenType::CharLiteral, val);
}

Token Lexer::scanSingleLineComment() {
    bool isDoc = (peek() == '/');
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
    TokenType type = isDoc ? TokenType::DocComment : TokenType::Comment;
    return makeToken(type);
}

Token Lexer::scanMultiLineComment() {
    int depth = 1;

    while (!isAtEnd() && depth > 0) {
        if (peek() == '/' && peekNext() == '*') {
            advance();
            advance();
            depth++;
        } else if (peek() == '*' && peekNext() == '/') {
            advance();
            advance();
            depth--;
        } else {
            advance();
        }
    }

    if (depth > 0) {
        return errorToken("E1000", "Unclosed multi-line comment", "Add '*/' to close the multi-line comment");
    }

    return makeToken(TokenType::Comment);
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.is(TokenType::EndOfFile)) {
            break;
        }
    }
    return tokens;
}

TokenType Lexer::lookupKeyword(std::string_view text) noexcept {
    static const std::unordered_map<std::string_view, TokenType> keywords = {
        // Declarations
        {"fn", TokenType::KeywordFn},
        {"class", TokenType::KeywordClass},
        {"interface", TokenType::KeywordInterface},
        {"struct", TokenType::KeywordStruct},
        {"enum", TokenType::KeywordEnum},
        {"trait", TokenType::KeywordTrait},

        // Control flow
        {"if", TokenType::KeywordIf},
        {"else", TokenType::KeywordElse},
        {"for", TokenType::KeywordFor},
        {"while", TokenType::KeywordWhile},
        {"do", TokenType::KeywordDo},
        {"switch", TokenType::KeywordSwitch},
        {"match", TokenType::KeywordMatch},
        {"case", TokenType::KeywordCase},
        {"default", TokenType::KeywordDefault},
        {"break", TokenType::KeywordBreak},
        {"continue", TokenType::KeywordContinue},
        {"return", TokenType::KeywordReturn},

        // Modules & Objects
        {"import", TokenType::KeywordImport},
        {"module", TokenType::KeywordModule},
        {"new", TokenType::KeywordNew},
        {"this", TokenType::KeywordThis},
        {"super", TokenType::KeywordSuper},
        {"null", TokenType::KeywordNull},

        // Variables
        {"var", TokenType::KeywordVar},
        {"let", TokenType::KeywordLet},
        {"const", TokenType::KeywordConst},

        // Modifiers
        {"public", TokenType::KeywordPublic},
        {"private", TokenType::KeywordPrivate},
        {"protected", TokenType::KeywordProtected},
        {"internal", TokenType::KeywordInternal},
        {"static", TokenType::KeywordStatic},
        {"abstract", TokenType::KeywordAbstract},
        {"override", TokenType::KeywordOverride},
        {"extends", TokenType::KeywordExtends},
        {"implements", TokenType::KeywordImplements},
        {"with", TokenType::KeywordWith},

        // Concurrency & Systems
        {"async", TokenType::KeywordAsync},
        {"await", TokenType::KeywordAwait},
        {"thread", TokenType::KeywordThread},
        {"spawn", TokenType::KeywordSpawn},
        {"channel", TokenType::KeywordChannel},
        {"mutex", TokenType::KeywordMutex},
        {"try", TokenType::KeywordTry},
        {"catch", TokenType::KeywordCatch},
        {"finally", TokenType::KeywordFinally},
        {"throw", TokenType::KeywordThrow},
        {"unsafe", TokenType::KeywordUnsafe},
        {"extern", TokenType::KeywordExtern},

        // Testing
        {"test", TokenType::KeywordTest},
        {"assert", TokenType::KeywordAssert},

        // Booleans
        {"true", TokenType::BooleanLiteral},
        {"false", TokenType::BooleanLiteral},

        // Primitive types
        {"void", TokenType::KeywordVoid},
        {"int", TokenType::KeywordInt},
        {"uint", TokenType::KeywordUint},
        {"int8", TokenType::KeywordInt8},
        {"int16", TokenType::KeywordInt16},
        {"int32", TokenType::KeywordInt32},
        {"int64", TokenType::KeywordInt64},
        {"uint8", TokenType::KeywordUint8},
        {"uint16", TokenType::KeywordUint16},
        {"uint32", TokenType::KeywordUint32},
        {"uint64", TokenType::KeywordUint64},
        {"float", TokenType::KeywordFloat},
        {"double", TokenType::KeywordDouble},
        {"bool", TokenType::KeywordBool},
        {"char", TokenType::KeywordChar},
        {"string", TokenType::KeywordString},
        {"byte", TokenType::KeywordByte}
    };

    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::Identifier;
}

} // namespace sp::lexer
