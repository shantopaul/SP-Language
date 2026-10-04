#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/diagnostics/SourceLocation.h"
#include "sp/lexer/TokenType.h"
#include "sp/lexer/Token.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/ast/ASTPrinter.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/runtime/Runtime.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>
#include <iomanip>

namespace {

void printVersion() {
    std::cout << "spc (SP Programming Language Compiler) version 0.3.0-alpha\n"
              << "Copyright (c) 2026 Shanto Paul\n"
              << "Target: native machine code (LLVM backend)\n";
}

void printHelp() {
    std::cout << "Usage: spc [command] [options] <file.sp>\n\n"
              << "Commands:\n"
              << "  tokens <file>   Scan source file and dump all tokens with source positions\n"
              << "  ast <file>      Parse source file and print the formatted Abstract Syntax Tree\n"
              << "  sema <file>     Perform semantic analysis and dump resolved symbols and types\n"
              << "  check <file>    Perform lexical, syntactic, and semantic type analysis\n"
              << "  build <file>    Compile an SP source file to a native executable\n"
              << "  run <file>      Compile and immediately execute an SP program\n"
              << "  fmt <file>      Format SP source files according to style rules\n"
              << "  test [path]     Execute SP unit and integration tests\n"
              << "  init [name]     Initialize a new SP project with sp.toml\n\n"
              << "Options:\n"
              << "  -h, --help      Display this help message\n"
              << "  -v, --version   Display compiler version information\n"
              << "  --debug         Build with debug symbols enabled\n"
              << "  --release       Build with optimizations enabled\n"
              << "  --target <arch> Cross-compile for target architecture\n";
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::string_view command = argv[1];

    if (command == "-v" || command == "--version" || command == "version") {
        printVersion();
        return 0;
    }

    if (command == "-h" || command == "--help" || command == "help") {
        printHelp();
        return 0;
    }

    sp::diagnostics::DiagnosticEngine diagEngine(std::cerr);

    if (command == "tokens" || command == "ast" || command == "sema" || command == "check" || command == "build" || command == "run") {
        if (argc < 3) {
            diagEngine.reportError("E1000", "Missing input source file", sp::diagnostics::SourceLocation("cli", 1, 1), "Specify an input file: spc " + std::string(command) + " main.sp");
            return 1;
        }

        std::string filename = argv[2];
        std::string source = readFile(filename);
        if (source.empty()) {
            diagEngine.reportError("E1009", "Could not open or read source file: " + filename, sp::diagnostics::SourceLocation(filename, 1, 1), "Check that the file exists and is accessible");
            return 1;
        }

        sp::lexer::Lexer lexer(source, filename, &diagEngine);

        if (command == "tokens") {
            lexer.setPreserveComments(true);
            std::vector<sp::lexer::Token> tokens = lexer.tokenizeAll();

            std::cout << "Tokens for " << filename << " (" << tokens.size() << " tokens):\n";
            std::cout << std::string(70, '-') << "\n";
            std::cout << std::left << std::setw(8) << "Line:Col" 
                      << std::setw(24) << "TokenType" 
                      << "Lexeme\n";
            std::cout << std::string(70, '-') << "\n";

            for (const auto& tok : tokens) {
                std::string locStr = std::to_string(tok.location.line) + ":" + std::to_string(tok.location.column);
                std::cout << std::left << std::setw(8) << locStr
                          << std::setw(24) << sp::lexer::tokenTypeToString(tok.type)
                          << "\"" << tok.lexeme << "\"\n";
            }
            return diagEngine.hasErrors() ? 1 : 0;
        }

        if (command == "ast") {
            std::vector<sp::lexer::Token> tokens = lexer.tokenizeAll();
            if (diagEngine.hasErrors()) {
                std::cerr << "Lexing failed with " << diagEngine.errorCount() << " error(s).\n";
                return 1;
            }

            sp::parser::Parser parser(tokens, filename, &diagEngine);
            auto program = parser.parseProgram();

            if (diagEngine.hasErrors()) {
                std::cerr << "Parsing failed with " << diagEngine.errorCount() << " error(s).\n";
                return 1;
            }

            sp::ast::ASTPrinter printer;
            std::cout << printer.print(*program);
            return 0;
        }

        if (command == "sema") {
            std::vector<sp::lexer::Token> tokens = lexer.tokenizeAll();
            if (diagEngine.hasErrors()) {
                std::cerr << "Lexing failed with " << diagEngine.errorCount() << " error(s).\n";
                return 1;
            }

            sp::parser::Parser parser(tokens, filename, &diagEngine);
            auto program = parser.parseProgram();

            if (diagEngine.hasErrors() || !program) {
                std::cerr << "Parsing failed with " << diagEngine.errorCount() << " error(s).\n";
                return 1;
            }

            sp::sema::SemanticAnalyzer sema(diagEngine);
            bool success = sema.analyze(*program);

            if (!success || diagEngine.hasErrors()) {
                std::cerr << "Semantic analysis failed with " << diagEngine.errorCount() << " error(s).\n";
                return 1;
            }

            std::cout << "Semantic analysis passed for " << filename << ".\n";
            std::cout << "Global symbols:\n";
            std::cout << std::string(60, '-') << "\n";
            for (const auto& [name, sym] : sema.getSymbolTable().getGlobalScope()->getSymbols()) {
                std::cout << "  " << std::left << std::setw(20) << name
                          << ": " << (sym->getType() ? sym->getType()->toString() : "unknown") << "\n";
            }
            return 0;
        }

        if (command == "check") {
            std::vector<sp::lexer::Token> tokens = lexer.tokenizeAll();
            if (diagEngine.hasErrors()) {
                std::cerr << "Check failed: " << diagEngine.errorCount() << " lexer error(s).\n";
                return 1;
            }

            sp::parser::Parser parser(tokens, filename, &diagEngine);
            auto program = parser.parseProgram();

            if (diagEngine.hasErrors() || !program) {
                std::cerr << "Check failed: " << diagEngine.errorCount() << " syntax error(s).\n";
                return 1;
            }

            sp::sema::SemanticAnalyzer sema(diagEngine);
            bool semaSuccess = sema.analyze(*program);

            if (!semaSuccess || diagEngine.hasErrors()) {
                std::cerr << "Check failed: " << diagEngine.errorCount() << " semantic error(s).\n";
                return 1;
            }

            size_t declCount = program ? program->getDeclarations().size() : 0;
            std::cout << "Check passed: Syntax & Semantics valid (" << tokens.size() << " tokens, " 
                      << declCount << " declarations) in " << filename << ".\n";
            return 0;
        }

        std::cout << "SP Compiler: Processing '" << filename << "' with command '" << command << "'...\n";
        return 0;
    }

    diagEngine.reportError("E1000", "Unknown command: " + std::string(command), sp::diagnostics::SourceLocation("cli", 1, 1), "Run 'spc --help' to see valid compiler commands");
    return 1;
}
