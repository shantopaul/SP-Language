// SP 2.0 Self-Hosted Compiler: Lexer
module spc.self_hosted.lexer;

import spc.self_hosted.tokens;

class Lexer {
    string source;
    int cursor;
    int line;
    int col;
    Token[] tokens;

    Lexer(string source) {
        this.source = source;
        this.cursor = 0;
        this.line = 1;
        this.col = 1;
        this.tokens = [];
    }

    fn isAtEnd(): bool {
        return this.cursor >= this.source.length();
    }

    fn peek(): string {
        if (this.isAtEnd()) {
            return "\0";
        }
        return this.source.substring(this.cursor, this.cursor + 1);
    }

    fn advance(): string {
        let ch = this.peek();
        this.cursor = this.cursor + 1;
        this.col = this.col + 1;
        return ch;
    }

    fn isAlpha(string ch): bool {
        return (ch >= "a" && ch <= "z") || (ch >= "A" && ch <= "Z") || ch == "_";
    }

    fn isDigit(string ch): bool {
        return ch >= "0" && ch <= "9";
    }

    fn isAlphaNumeric(string ch): bool {
        return this.isAlpha(ch) || this.isDigit(ch);
    }

    fn scanToken() {
        let startCol = this.col;
        let ch = this.advance();

        if (ch == " " || ch == "\t" || ch == "\r") {
            return;
        }

        if (ch == "\n") {
            this.line = this.line + 1;
            this.col = 1;
            return;
        }

        // Single-line comment
        if (ch == "/" && this.peek() == "/") {
            while (!this.isAtEnd() && this.peek() != "\n") {
                this.advance();
            }
            return;
        }

        // Two-character operators
        if (ch == "=" && this.peek() == "=") {
            this.advance();
            this.tokens.push(new Token(TokenType.EqualEqual, "==", this.line, startCol));
            return;
        }
        if (ch == "!" && this.peek() == "=") {
            this.advance();
            this.tokens.push(new Token(TokenType.BangEqual, "!=", this.line, startCol));
            return;
        }
        if (ch == "<" && this.peek() == "=") {
            this.advance();
            this.tokens.push(new Token(TokenType.LessEqual, "<=", this.line, startCol));
            return;
        }
        if (ch == ">" && this.peek() == "=") {
            this.advance();
            this.tokens.push(new Token(TokenType.GreaterEqual, ">=", this.line, startCol));
            return;
        }
        if (ch == "-" && this.peek() == ">") {
            this.advance();
            this.tokens.push(new Token(TokenType.Arrow, "->", this.line, startCol));
            return;
        }
        if (ch == "&" && this.peek() == "&") {
            this.advance();
            this.tokens.push(new Token(TokenType.AmpAmp, "&&", this.line, startCol));
            return;
        }
        if (ch == "|" && this.peek() == "|") {
            this.advance();
            this.tokens.push(new Token(TokenType.PipePipe, "||", this.line, startCol));
            return;
        }

        // Single character punctuation
        if (ch == "+") { this.tokens.push(new Token(TokenType.Plus, "+", this.line, startCol)); return; }
        if (ch == "-") { this.tokens.push(new Token(TokenType.Minus, "-", this.line, startCol)); return; }
        if (ch == "*") { this.tokens.push(new Token(TokenType.Star, "*", this.line, startCol)); return; }
        if (ch == "/") { this.tokens.push(new Token(TokenType.Slash, "/", this.line, startCol)); return; }
        if (ch == "=") { this.tokens.push(new Token(TokenType.Equal, "=", this.line, startCol)); return; }
        if (ch == "<") { this.tokens.push(new Token(TokenType.Less, "<", this.line, startCol)); return; }
        if (ch == ">") { this.tokens.push(new Token(TokenType.Greater, ">", this.line, startCol)); return; }
        if (ch == ":") { this.tokens.push(new Token(TokenType.Colon, ":", this.line, startCol)); return; }
        if (ch == ";") { this.tokens.push(new Token(TokenType.Semi, ";", this.line, startCol)); return; }
        if (ch == ",") { this.tokens.push(new Token(TokenType.Comma, ",", this.line, startCol)); return; }
        if (ch == ".") { this.tokens.push(new Token(TokenType.Dot, ".", this.line, startCol)); return; }
        if (ch == "(") { this.tokens.push(new Token(TokenType.LParen, "(", this.line, startCol)); return; }
        if (ch == ")") { this.tokens.push(new Token(TokenType.RParen, ")", this.line, startCol)); return; }
        if (ch == "{") { this.tokens.push(new Token(TokenType.LBrace, "{", this.line, startCol)); return; }
        if (ch == "}") { this.tokens.push(new Token(TokenType.RBrace, "}", this.line, startCol)); return; }
        if (ch == "[") { this.tokens.push(new Token(TokenType.LBracket, "[", this.line, startCol)); return; }
        if (ch == "]") { this.tokens.push(new Token(TokenType.RBracket, "]", this.line, startCol)); return; }

        // Strings
        if (ch == "\"") {
            let start = this.cursor;
            while (!this.isAtEnd() && this.peek() != "\"") {
                if (this.peek() == "\n") {
                    this.line = this.line + 1;
                }
                this.advance();
            }
            let strVal = this.source.substring(start, this.cursor);
            if (!this.isAtEnd()) {
                this.advance(); // closing quote
            }
            this.tokens.push(new Token(TokenType.StringLiteral, strVal, this.line, startCol));
            return;
        }

        // Numbers
        if (this.isDigit(ch)) {
            let start = this.cursor - 1;
            while (!this.isAtEnd() && this.isDigit(this.peek())) {
                this.advance();
            }
            let numVal = this.source.substring(start, this.cursor);
            this.tokens.push(new Token(TokenType.IntegerLiteral, numVal, this.line, startCol));
            return;
        }

        // Identifiers and Keywords
        if (this.isAlpha(ch)) {
            let start = this.cursor - 1;
            while (!this.isAtEnd() && this.isAlphaNumeric(this.peek())) {
                this.advance();
            }
            let ident = this.source.substring(start, this.cursor);

            let kind = TokenType.Identifier;
            if (ident == "fn") kind = TokenType.KwFn;
            else if (ident == "let") kind = TokenType.KwLet;
            else if (ident == "var") kind = TokenType.KwVar;
            else if (ident == "const") kind = TokenType.KwConst;
            else if (ident == "if") kind = TokenType.KwIf;
            else if (ident == "else") kind = TokenType.KwElse;
            else if (ident == "while") kind = TokenType.KwWhile;
            else if (ident == "for") kind = TokenType.KwFor;
            else if (ident == "return") kind = TokenType.KwReturn;
            else if (ident == "class") kind = TokenType.KwClass;
            else if (ident == "struct") kind = TokenType.KwStruct;
            else if (ident == "enum") kind = TokenType.KwEnum;
            else if (ident == "import") kind = TokenType.KwImport;
            else if (ident == "async") kind = TokenType.KwAsync;
            else if (ident == "await") kind = TokenType.KwAwait;
            else if (ident == "comptime") kind = TokenType.KwComptime;

            this.tokens.push(new Token(kind, ident, this.line, startCol));
            return;
        }
    }

    fn tokenize(): Token[] {
        while (!this.isAtEnd()) {
            this.scanToken();
        }
        this.tokens.push(new Token(TokenType.Eof, "<EOF>", this.line, this.col));
        return this.tokens;
    }
}
