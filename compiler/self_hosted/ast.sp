// SP 2.0 Self-Hosted Compiler: Abstract Syntax Tree Nodes
module spc.self_hosted.ast;

import spc.self_hosted.tokens;

class ASTNode {
    int line;
    ASTNode(int line) {
        this.line = line;
    }
}

// Expressions
class Expr : ASTNode {
    Expr(int line) {
        this.line = line;
    }
}

class LiteralExpr : Expr {
    string value;
    string typeName;

    LiteralExpr(string value, string typeName, int line) {
        this.line = line;
        this.value = value;
        this.typeName = typeName;
    }
}

class IdentifierExpr : Expr {
    string name;

    IdentifierExpr(string name, int line) {
        this.line = line;
        this.name = name;
    }
}

class BinaryExpr : Expr {
    Expr left;
    string op;
    Expr right;

    BinaryExpr(Expr left, string op, Expr right, int line) {
        this.line = line;
        this.left = left;
        this.op = op;
        this.right = right;
    }
}

class CallExpr : Expr {
    Expr callee;
    Expr[] args;

    CallExpr(Expr callee, Expr[] args, int line) {
        this.line = line;
        this.callee = callee;
        this.args = args;
    }
}

class ComptimeExpr : Expr {
    Expr expr;

    ComptimeExpr(Expr expr, int line) {
        this.line = line;
        this.expr = expr;
    }
}

class AwaitExpr : Expr {
    Expr expr;

    AwaitExpr(Expr expr, int line) {
        this.line = line;
        this.expr = expr;
    }
}

// Statements
class Stmt : ASTNode {
    string kind;
    Stmt(int line) {
        this.line = line;
        this.kind = "stmt";
    }
}

class BlockStmt : Stmt {
    Stmt[] statements;

    BlockStmt(Stmt[] statements, int line) {
        this.line = line;
        this.kind = "block";
        this.statements = statements;
    }
}

class VarDeclStmt : Stmt {
    string name;
    string typeName;
    Expr initializer;
    bool isConst;

    VarDeclStmt(string name, string typeName, Expr initializer, bool isConst, int line) {
        this.line = line;
        this.kind = "var_decl";
        this.name = name;
        this.typeName = typeName;
        this.initializer = initializer;
        this.isConst = isConst;
    }
}

class IfStmt : Stmt {
    Expr condition;
    Stmt thenBranch;
    Stmt elseBranch;

    IfStmt(Expr condition, Stmt thenBranch, Stmt elseBranch, int line) {
        this.line = line;
        this.kind = "if";
        this.condition = condition;
        this.thenBranch = thenBranch;
        this.elseBranch = elseBranch;
    }
}

class ReturnStmt : Stmt {
    Expr value;

    ReturnStmt(Expr value, int line) {
        this.line = line;
        this.value = value;
    }
}

class ExprStmt : Stmt {
    Expr expr;

    ExprStmt(Expr expr, int line) {
        this.line = line;
        this.expr = expr;
    }
}

// Declarations
class Parameter {
    string name;
    string typeName;

    Parameter(string name, string typeName) {
        this.name = name;
        this.typeName = typeName;
    }
}

class FunctionDecl : ASTNode {
    string name;
    Parameter[] params;
    string returnType;
    BlockStmt body;
    bool isAsync;
    bool isComptime;

    FunctionDecl(string name, Parameter[] params, string returnType, BlockStmt body, bool isAsync, bool isComptime, int line) {
        this.line = line;
        this.name = name;
        this.params = params;
        this.returnType = returnType;
        this.body = body;
        this.isAsync = isAsync;
        this.isComptime = isComptime;
    }
}

class ProgramNode : ASTNode {
    FunctionDecl[] functions;

    ProgramNode(FunctionDecl[] functions) {
        this.line = 1;
        this.functions = functions;
    }
}
