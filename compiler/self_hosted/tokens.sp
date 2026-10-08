// SP 2.0 Self-Hosted Compiler: Token Definitions
module spc.self_hosted.tokens;

enum TokenType {
    Eof,
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    
    // Keywords
    KwFn,
    KwLet,
    KwVar,
    KwConst,
    KwIf,
    KwElse,
    KwWhile,
    KwFor,
    KwReturn,
    KwClass,
    KwStruct,
    KwEnum,
    KwImport,
    KwAsync,
    KwAwait,
    KwComptime,
    
    // Operators & Delimiters
    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    AmpAmp,
    PipePipe,
    Colon,
    Semi,
    Comma,
    Dot,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Arrow
}

class Token {
    TokenType kind;
    string lexeme;
    int line;
    int col;

    Token(TokenType kind, string lexeme, int line, int col) {
        this.kind = kind;
        this.lexeme = lexeme;
        this.line = line;
        this.col = col;
    }

    fn toString(): string {
        return "Token(" + this.lexeme + ", line: " + this.line.toString() + ")";
    }
}
