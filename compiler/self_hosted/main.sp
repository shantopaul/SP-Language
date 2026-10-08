// SP 2.0 Self-Hosted Compiler: Driver Entry Point
module spc.self_hosted.main;

import spc.self_hosted.tokens;
import spc.self_hosted.lexer;
import spc.self_hosted.ast;
import spc.self_hosted.parser;
import spc.self_hosted.sema;

fn compileSource(string source): int {
    println(">>> [SP 2.0 Self-Hosted Frontend] Starting compilation...");

    // 1. Lexing
    let lexer = new Lexer(source);
    let tokens = lexer.tokenize();
    println(">>> Tokenized " + tokens.length().toString() + " tokens successfully.");

    // 2. Parsing
    let parser = new Parser(tokens);
    let program = parser.parseProgram();
    println(">>> Parsed " + program.functions.length().toString() + " top-level functions.");

    // 3. Semantic Analysis
    let sema = new SemanticAnalyzer();
    let success = sema.analyze(program);

    if (success) {
        println(">>> SP 2.0 Self-Hosted Frontend: Compilation finished with 0 errors!");
        return 0;
    } else {
        println(">>> SP 2.0 Self-Hosted Frontend: Compilation failed with errors.");
        return 1;
    }
}

fn main(): int {
    let sampleSource = "fn square(int x): int { return x * x; }";
    return compileSource(sampleSource);
}
