#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/diagnostics/SourceLocation.h"
#include "sp/lexer/TokenType.h"
#include "sp/lexer/Token.h"
#include "sp/runtime/Runtime.h"
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void printVersion() {
    std::cout << "spc (SP Programming Language Compiler) version 0.1.0-alpha\n"
              << "Copyright (c) 2026 Shanto Paul\n"
              << "Target: native machine code (LLVM backend)\n";
}

void printHelp() {
    std::cout << "Usage: spc [command] [options] <file.sp>\n\n"
              << "Commands:\n"
              << "  build <file>    Compile an SP source file to a native executable\n"
              << "  run <file>      Compile and immediately execute an SP program\n"
              << "  check <file>    Type-check and validate syntax without code generation\n"
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

    if (command == "check" || command == "build" || command == "run") {
        if (argc < 3) {
            diagEngine.reportError("E1000", "Missing input source file", sp::diagnostics::SourceLocation("cli", 1, 1), "Specify an input file: spc " + std::string(command) + " main.sp");
            return 1;
        }

        std::string filename = argv[2];
        std::cout << "SP Compiler: Processing '" << filename << "' with command '" << command << "'...\n";
        return 0;
    }

    diagEngine.reportError("E1000", "Unknown command: " + std::string(command), sp::diagnostics::SourceLocation("cli", 1, 1), "Run 'spc --help' to see valid compiler commands");
    return 1;
}
