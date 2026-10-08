// SP 2.0 Self-Hosted Compiler: Recursive Descent Parser
module spc.self_hosted.parser;

import spc.self_hosted.tokens;
import spc.self_hosted.ast;

class Parser {
    Token[] tokens;
    int cursor;

    Parser(Token[] tokens) {
        this.tokens = tokens;
        this.cursor = 0;
    }

    fn isAtEnd(): bool {
        return this.peek().kind == TokenType.Eof;
    }

    fn peek(): Token {
        if (this.cursor >= this.tokens.length()) {
            return this.tokens[this.tokens.length() - 1];
        }
        return this.tokens[this.cursor];
    }

    fn advance(): Token {
        let t = this.peek();
        if (!this.isAtEnd()) {
            this.cursor = this.cursor + 1;
        }
        return t;
    }

    fn check(TokenType kind): bool {
        return this.peek().kind == kind;
    }

    fn matchToken(TokenType kind): bool {
        if (this.check(kind)) {
            this.advance();
            return true;
        }
        return false;
    }

    fn parsePrimary(): Expr {
        let t = this.peek();

        if (this.matchToken(TokenType.IntegerLiteral)) {
            return new LiteralExpr(t.lexeme, "int", t.line);
        }
        if (this.matchToken(TokenType.StringLiteral)) {
            return new LiteralExpr(t.lexeme, "string", t.line);
        }
        if (this.matchToken(TokenType.Identifier)) {
            let expr = new IdentifierExpr(t.lexeme, t.line);
            if (this.matchToken(TokenType.LParen)) {
                let args: Expr[] = [];
                if (!this.check(TokenType.RParen)) {
                    args.push(this.parseExpression());
                    while (this.matchToken(TokenType.Comma)) {
                        args.push(this.parseExpression());
                    }
                }
                this.matchToken(TokenType.RParen);
                return new CallExpr(expr, args, t.line);
            }
            return expr;
        }

        if (this.matchToken(TokenType.KwComptime)) {
            let inner = this.parseExpression();
            return new ComptimeExpr(inner, t.line);
        }

        if (this.matchToken(TokenType.KwAwait)) {
            let inner = this.parseExpression();
            return new AwaitExpr(inner, t.line);
        }

        if (this.matchToken(TokenType.LParen)) {
            let inner = this.parseExpression();
            this.matchToken(TokenType.RParen);
            return inner;
        }

        this.advance();
        return new LiteralExpr("0", "int", t.line);
    }

    fn parseBinary(int minPrec): Expr {
        let left = this.parsePrimary();

        while (true) {
            let t = this.peek();
            let prec = 0;
            if (t.kind == TokenType.Star || t.kind == TokenType.Slash) prec = 3;
            else if (t.kind == TokenType.Plus || t.kind == TokenType.Minus) prec = 2;
            else if (t.kind == TokenType.EqualEqual || t.kind == TokenType.BangEqual ||
                     t.kind == TokenType.Less || t.kind == TokenType.Greater) prec = 1;

            if (prec <= minPrec) break;

            this.advance();
            let right = this.parseBinary(prec);
            left = new BinaryExpr(left, t.lexeme, right, t.line);
        }

        return left;
    }

    fn parseExpression(): Expr {
        return this.parseBinary(0);
    }

    fn parseBlock(): BlockStmt {
        let line = this.peek().line;
        this.matchToken(TokenType.LBrace);
        let stmts: Stmt[] = [];

        while (!this.check(TokenType.RBrace) && !this.isAtEnd()) {
            stmts.push(this.parseStatement());
        }

        this.matchToken(TokenType.RBrace);
        return new BlockStmt(stmts, line);
    }

    fn parseStatement(): Stmt {
        let t = this.peek();

        if (this.matchToken(TokenType.KwLet) || this.matchToken(TokenType.KwConst) || this.matchToken(TokenType.KwVar)) {
            let isConst = t.kind == TokenType.KwConst;
            let nameTok = this.advance();
            let typeName = "auto";
            if (this.matchToken(TokenType.Colon)) {
                typeName = this.advance().lexeme;
            }
            let init: Expr = null;
            if (this.matchToken(TokenType.Equal)) {
                init = this.parseExpression();
            }
            this.matchToken(TokenType.Semi);
            return new VarDeclStmt(nameTok.lexeme, typeName, init, isConst, t.line);
        }

        if (this.matchToken(TokenType.KwReturn)) {
            let val: Expr = null;
            if (!this.check(TokenType.Semi)) {
                val = this.parseExpression();
            }
            this.matchToken(TokenType.Semi);
            return new ReturnStmt(val, t.line);
        }

        if (this.matchToken(TokenType.KwIf)) {
            this.matchToken(TokenType.LParen);
            let cond = this.parseExpression();
            this.matchToken(TokenType.RParen);
            let thenBranch = this.parseStatement();
            let elseBranch: Stmt = null;
            if (this.matchToken(TokenType.KwElse)) {
                elseBranch = this.parseStatement();
            }
            return new IfStmt(cond, thenBranch, elseBranch, t.line);
        }

        if (this.check(TokenType.LBrace)) {
            return this.parseBlock();
        }

        let expr = this.parseExpression();
        this.matchToken(TokenType.Semi);
        return new ExprStmt(expr, t.line);
    }

    fn parseFunction(): FunctionDecl {
        let isAsync = this.matchToken(TokenType.KwAsync);
        let isComptime = this.matchToken(TokenType.KwComptime);
        this.advance(); // 'fn'
        let nameTok = this.advance();

        this.matchToken(TokenType.LParen);
        let params: Parameter[] = [];
        if (!this.check(TokenType.RParen)) {
            let pType = this.advance().lexeme;
            let pName = this.advance().lexeme;
            params.push(new Parameter(pName, pType));
            while (this.matchToken(TokenType.Comma)) {
                let pt = this.advance().lexeme;
                let pn = this.advance().lexeme;
                params.push(new Parameter(pn, pt));
            }
        }
        this.matchToken(TokenType.RParen);

        let returnType = "void";
        if (this.matchToken(TokenType.Colon)) {
            returnType = this.advance().lexeme;
        }

        let body = this.parseBlock();
        return new FunctionDecl(nameTok.lexeme, params, returnType, body, isAsync, isComptime, nameTok.line);
    }

    fn parseProgram(): ProgramNode {
        let fns: FunctionDecl[] = [];
        while (!this.isAtEnd()) {
            if (this.check(TokenType.KwFn) || this.check(TokenType.KwAsync) || this.check(TokenType.KwComptime)) {
                fns.push(this.parseFunction());
            } else {
                this.advance();
            }
        }
        return new ProgramNode(fns);
    }
}
