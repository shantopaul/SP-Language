// SP 2.0 Self-Hosted Compiler: Semantic Analyzer
module spc.self_hosted.sema;

import spc.self_hosted.ast;

class Symbol {
    string name;
    string typeName;

    Symbol(string name, string typeName) {
        this.name = name;
        this.typeName = typeName;
    }
}

class Scope {
    Symbol[] symbols;
    Scope parent;

    Scope(Scope parent) {
        this.symbols = [];
        this.parent = parent;
    }

    fn define(string name, string typeName) {
        this.symbols.push(new Symbol(name, typeName));
    }

    fn resolve(string name): Symbol {
        for (let i = 0; i < this.symbols.length(); i = i + 1) {
            if (this.symbols[i].name == name) {
                return this.symbols[i];
            }
        }
        if (this.parent != null) {
            return this.parent.resolve(name);
        }
        return null;
    }
}

class SemanticAnalyzer {
    Scope currentScope;
    int errorCount;

    SemanticAnalyzer() {
        this.currentScope = new Scope(null);
        this.errorCount = 0;
    }

    fn pushScope() {
        this.currentScope = new Scope(this.currentScope);
    }

    fn popScope() {
        if (this.currentScope.parent != null) {
            this.currentScope = this.currentScope.parent;
        }
    }

    fn reportError(string msg, int line) {
        println("Semantic Error [line " + line.toString() + "]: " + msg);
        this.errorCount = this.errorCount + 1;
    }

    fn analyzeStmt(Stmt stmt) {
        if (stmt == null) return;

        if (stmt.kind == "var_decl") {
            let varDecl: VarDeclStmt = stmt;
            this.currentScope.define(varDecl.name, varDecl.typeName);
            return;
        }

        if (stmt.kind == "block") {
            let block: BlockStmt = stmt;
            this.pushScope();
            for (let i = 0; i < block.statements.length(); i = i + 1) {
                this.analyzeStmt(block.statements[i]);
            }
            this.popScope();
            return;
        }

        if (stmt.kind == "if") {
            let ifStmt: IfStmt = stmt;
            this.analyzeStmt(ifStmt.thenBranch);
            if (ifStmt.elseBranch != null) {
                this.analyzeStmt(ifStmt.elseBranch);
            }
            return;
        }
    }

    fn analyzeFunction(FunctionDecl fnDecl) {
        this.pushScope();
        for (let i = 0; i < fnDecl.params.length(); i = i + 1) {
            this.currentScope.define(fnDecl.params[i].name, fnDecl.params[i].typeName);
        }
        this.analyzeStmt(fnDecl.body);
        this.popScope();
    }

    fn analyze(ProgramNode prog): bool {
        // Register top-level functions
        for (let i = 0; i < prog.functions.length(); i = i + 1) {
            this.currentScope.define(prog.functions[i].name, prog.functions[i].returnType);
        }

        // Analyze function bodies
        for (let i = 0; i < prog.functions.length(); i = i + 1) {
            this.analyzeFunction(prog.functions[i]);
        }

        return this.errorCount == 0;
    }
}
