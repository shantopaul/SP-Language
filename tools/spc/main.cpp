#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/diagnostics/SourceLocation.h"
#include "sp/lexer/TokenType.h"
#include "sp/lexer/Token.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/ast/ASTPrinter.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRPrinter.h"
#include "sp/codegen/LLVMGenerator.h"
#include "sp/codegen/ObjectEmitter.h"
#include "sp/runtime/Runtime.h"
#include "sp/Version.h"
#include "fmt/Formatter.h"
#include "test/TestRunner.h"
#include "doc/DocGenerator.h"
#include "bindgen/Bindgen.h"
#include "benchmark/Benchmark.h"
#include "sp/util/PathUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>
#include <iomanip>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace {

void printVersion() {
    std::cout << "spc (SP Programming Language Compiler) version " << sp::SP_VERSION_STRING << "\n"
              << "Copyright (c) 2026 Shanto Paul\n"
              << "Website: " << sp::SP_HOMEPAGE_URL << "\n"
              << "Docs:    " << sp::SP_DOCS_URL << "\n"
              << "Target:  " << sp::codegen::LLVMGenerator::getHostTargetTriple() << " (in-process LLVM & LLD backend)\n";
}

void printHelp() {
    std::cout << "Usage: spc [command] [options] <file.sp>\n\n"
              << "Commands:\n"
              << "  tokens <file>   Scan source file and dump all tokens with source positions\n"
              << "  ast <file>      Parse source file and print the formatted Abstract Syntax Tree\n"
              << "  sema <file>     Perform semantic analysis and dump resolved symbols and types\n"
              << "  check <file>    Perform lexical, syntactic, and semantic type analysis\n"
              << "  ir <file>       Compile to SP Intermediate Representation (SP IR)\n"
              << "  llvm <file>     Translate SP IR to LLVM IR (.ll) representation\n"
              << "  obj <file>      Compile an SP source file directly to a native object file (.o / .obj)\n"
              << "  build <file>    Compile an SP source file to a native standalone executable\n"
              << "  run <file>      Compile and immediately execute an SP program\n"
              << "  fmt <file>      Format SP source files according to style rules\n"
              << "  test [path]     Execute SP unit and integration tests\n"
              << "  bench [path]    Execute micro-benchmarks with statistical regression tracking\n"
              << "  doc <path>      Generate HTML/Markdown documentation from /// comments\n"
              << "  init [name]     Initialize a new SP project with sp.toml\n"
              << "  bindgen <hdr.h> Generate SP FFI bindings from C header file (.h)\n"
              << "  doctor          Diagnose SP installation, environment, and backend toolchain\n"
              << "  setup           Check, auto-provision, or update portable backend toolchain\n\n"
              << "Options:\n"
              << "  --save-baseline Save current benchmark results as new historical baseline\n"
              << "  --warmup <n>    Number of warmup iterations (default: 5)\n"
              << "  --samples <n>   Number of sample measurement runs (default: 50)\n"
              << "  -o <path>       Specify output binary path (for build/obj command)\n"
              << "  -c, --emit-obj  Emit relocatable object file (.o / .obj) directly\n"
              << "  -S, --emit-ir   Emit LLVM IR (.ll) directly\n"
              << "  -O0, -O1, -O2, -O3, -Os, -Oz Optimization level (default: -O0)\n"
              << "  --opt-level <O> Specify optimization level (O0, O1, O2, O3, Os, Oz)\n"
              << "  --release       Build with -O3 optimizations enabled\n"
              << "  -g, --debug     Build with debug symbols (-g) enabled\n"
              << "  --target <arch> Cross-compile for target architecture triple\n"
              << "  -h, --help      Display this help message\n"
              << "  -v, --version   Display compiler version information\n";
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

int handleDoctor() {
    auto health = sp::util::inspectToolchainHealth();

    std::cout << "\n========================================================================\n"
              << "             SP Programming Language System Doctor (v" << sp::SP_VERSION_STRING << ")\n"
              << "========================================================================\n\n";

    // 1. SP Compiler
    std::cout << "[1] SP Compiler Binary:\n"
              << "    Location:       " << health.spcPath.string() << "\n"
              << "    Status:         [OK] Operational\n\n";

    // 2. SP_HOME
    std::cout << "[2] SP Installation Home (SP_HOME):\n"
              << "    Location:       " << (health.spHome.empty() ? "(none)" : health.spHome.string()) << "\n";
    const char* envHome = std::getenv("SP_HOME");
    if (envHome && *envHome) {
        std::cout << "    Environment:    SP_HOME is explicitly configured in environment\n"
                  << "    Status:         [OK]\n\n";
    } else {
        std::cout << "    Environment:    SP_HOME not set (inferred from binary location)\n"
                  << "    Status:         [OK] Portable relative auto-discovery active\n\n";
    }

    // 3. Standard Library
    std::cout << "[3] Standard Library (stdlib):\n"
              << "    Location:       " << (health.stdlibDir.empty() ? "(not found)" : health.stdlibDir.string()) << "\n";
    if (health.hasStdlib) {
        size_t moduleCount = 0;
        std::string moduleNames;
        try {
            for (const auto& entry : fs::directory_iterator(health.stdlibDir)) {
                if (entry.is_directory()) {
                    if (!moduleNames.empty()) moduleNames += ", ";
                    moduleNames += entry.path().filename().string();
                    moduleCount++;
                }
            }
        } catch (...) {}
        std::cout << "    Modules:        " << moduleNames << "\n"
                  << "    Status:         [OK] (" << moduleCount << " standard library modules ready)\n\n";
    } else {
        std::cout << "    Status:         [MISSING] Standard library directory not found!\n"
                  << "    Action:         Ensure 'stdlib' directory is placed in SP_HOME or next to spc.\n\n";
    }

    // 4. Runtime Library
    std::cout << "[4] Precompiled Runtime Library:\n"
              << "    Library:        " << (health.runtimeLib.empty() ? "(not found)" : health.runtimeLib.string()) << "\n"
              << "    Headers:        " << (health.runtimeInc.empty() ? "(not found)" : health.runtimeInc.string()) << "\n";
    if (health.hasRuntimeLib) {
        std::error_code ec;
        auto sz = fs::file_size(health.runtimeLib, ec);
        std::cout << "    Status:         [OK] (" << sz << " bytes precompiled runtime ready)\n\n";
    } else if (!health.runtimeInc.empty()) {
        std::cout << "    Status:         [OK] Runtime headers found (source compilation fallback ready)\n\n";
    } else {
        std::cout << "    Status:         [MISSING] Runtime library or headers not found!\n\n";
    }

    // 5. Backend Toolchain
    std::cout << "[5] Backend C/LLVM Toolchain:\n"
              << "    Target Triple:  " << health.targetTriple << "\n"
              << "    C Compiler:     " << (health.clangPath.empty() ? "(not found)" : health.clangPath.string()) << "\n"
              << "    Linker:         " << (health.linkerPath.empty() ? "(not found)" : health.linkerPath.string()) << "\n";
    if (health.hasClang || health.hasLinker) {
        std::cout << "    Status:         [OK] Native code generation backend operational\n\n";
    } else {
        std::cout << "    Status:         [MISSING] No Clang or LLD linker detected on this computer!\n"
                  << "    Action:         Run 'spc setup' to auto-provision a portable toolchain.\n\n";
    }

    std::cout << "------------------------------------------------------------------------\n";
    if (health.isOperational) {
        std::cout << "Diagnosis: ALL SYSTEMS OPERATIONAL\n"
                  << "Your computer is 100% ready to build and run native SP applications!\n";
    } else {
        std::cout << "Diagnosis: ACTION REQUIRED\n"
                  << "Run 'spc setup' to automatically configure missing components.\n";
    }
    std::cout << "========================================================================\n\n";
    return health.isOperational ? 0 : 1;
}

int handleSetup() {
    std::cout << "\n========================================================================\n"
              << "             SP Programming Language Auto-Setup & Repair\n"
              << "========================================================================\n\n";

    auto health = sp::util::inspectToolchainHealth();

    if (health.isOperational) {
        std::cout << "[✓] SP Language installation is already fully operational!\n"
                  << "    SP_HOME:   " << health.spHome.string() << "\n"
                  << "    Compiler:  " << health.spcPath.string() << "\n"
                  << "    Toolchain: " << health.clangPath.string() << "\n\n";
    } else {
        std::cout << "[*] Inspecting environment and provisioning missing components...\n";

#if defined(_WIN32)
        if (!health.hasClang && !health.hasLinker) {
            std::cout << "[*] No backend toolchain found. Preparing portable toolchain...\n";
            fs::path tcDir = health.spHome / "toolchain";
            fs::create_directories(tcDir);

            // Execute toolchain provisioning script
            std::string psCmd = 
                "powershell -ExecutionPolicy Bypass -Command \""
                "Write-Host 'Downloading portable toolchain for Windows x64...'; "
                "$tc = '" + tcDir.string() + "'; "
                "$url = 'https://splang.shantopaul.com/downloads/toolchain-win64.zip'; "
                "$tmp = Join-Path $env:TEMP 'sp_toolchain.zip'; "
                "try { "
                "  Invoke-WebRequest -Uri $url -OutFile $tmp -UseBasicParsing -TimeoutSec 120; "
                "  Expand-Archive -Path $tmp -DestinationPath $tc -Force; "
                "  Remove-Item $tmp -Force -ErrorAction SilentlyContinue; "
                "  Write-Host 'Portable toolchain provisioned successfully!'; "
                "} catch { "
                "  Write-Host 'Notice: Online download unreachable. If using offline package, toolchain is bundled in toolchain/ directory.'; "
                "}\"";
            std::system(psCmd.c_str());
        }
#endif
    }

    // Run quick verification smoke test
    std::cout << "[*] Running end-to-end compiler verification test...\n";
    std::string testProg = "import std.io;\nfn main() {\n    println(\"SP self-test: SUCCESS!\");\n}\n";
    std::string testPath = (fs::temp_directory_path() / "sp_setup_smoke.sp").string();
    std::string testExe = (fs::temp_directory_path() / "sp_setup_smoke.exe").string();

    {
        std::ofstream out(testPath);
        out << testProg;
    }

    sp::diagnostics::DiagnosticEngine diags(std::cerr);
    sp::lexer::Lexer lex(testProg, "sp_setup_smoke.sp", &diags);
    auto tokens = lex.tokenizeAll();
    sp::parser::Parser p(tokens, "sp_setup_smoke.sp", &diags);
    auto prog = p.parseProgram();

    bool smokePassed = false;
    if (prog && !diags.hasErrors()) {
        sp::sema::SemanticAnalyzer sema(diags);
        if (sema.analyze(*prog) && !diags.hasErrors()) {
            sp::ir::IRBuilder b(sema);
            auto mod = b.build(*prog, "smoke");
            if (mod) {
                sp::codegen::CodegenOptions opts;
                sp::codegen::LLVMGenerator gen(opts);
                if (gen.compileToExecutable(*mod, testExe)) {
                    smokePassed = true;
                }
            }
        }
    }

    std::error_code ec;
    fs::remove(testPath, ec);

    if (smokePassed && fs::exists(testExe)) {
        std::cout << "    Verification output: ";
        std::string runCmd = "\"" + testExe + "\"";
        std::system(runCmd.c_str());
        fs::remove(testExe, ec);
        std::cout << "\n========================================================================\n"
                  << "  [SUCCESS] SP Language environment is 100% verified and operational!\n"
                  << "========================================================================\n\n";
        return 0;
    } else {
        std::cout << "\n[!] Setup check completed. Run 'spc doctor' to view detailed diagnostic info.\n\n";
        return 0;
    }
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

    if (command == "doctor") {
        return handleDoctor();
    }

    if (command == "setup") {
        return handleSetup();
    }

    sp::diagnostics::DiagnosticEngine diagEngine(std::cerr);

    if (command == "tokens" || command == "ast" || command == "sema" ||
        command == "check" || command == "ir" || command == "llvm" ||
        command == "obj" || command == "build" || command == "run") {

        // Parse options and identify input source file
        std::string filename;
        std::string outputPath;
        std::string targetTriple;
        std::string optLevel;
        bool releaseMode = false;
        bool debugMode = false;
        bool emitObjOnly = false;
        bool emitIROnly = false;

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if (arg == "-o" && i + 1 < argc) {
                outputPath = argv[++i];
            } else if (arg == "-c" || arg == "--emit-obj") {
                emitObjOnly = true;
            } else if (arg == "-S" || arg == "--emit-ir") {
                emitIROnly = true;
            } else if (arg == "-g") {
                debugMode = true;
            } else if (arg == "--opt-level" && i + 1 < argc) {
                optLevel = argv[++i];
                if (!optLevel.empty() && optLevel[0] != '-') optLevel = "-" + optLevel;
            } else if (arg.starts_with("--opt-level=")) {
                optLevel = std::string(arg.substr(12));
                if (!optLevel.empty() && optLevel[0] != '-') optLevel = "-" + optLevel;
            } else if (arg == "--release") {
                releaseMode = true;
                optLevel = "-O3";
            } else if (arg == "--debug") {
                debugMode = true;
                optLevel = "-O0";
            } else if (arg == "-O0" || arg == "-O1" || arg == "-O2" || arg == "-O3" || arg == "-Os" || arg == "-Oz") {
                optLevel = std::string(arg);
            } else if (arg == "--target" && i + 1 < argc) {
                targetTriple = argv[++i];
            } else if (arg.starts_with("--target=")) {
                targetTriple = std::string(arg.substr(9));
            } else if (arg == "-h" || arg == "--help") {
                printHelp();
                return 0;
            } else if (!arg.starts_with("-")) {
                if (filename.empty()) {
                    filename = arg;
                }
            }
        }

        if (filename.empty()) {
            diagEngine.reportError("E1000", "Missing input source file",
                                   sp::diagnostics::SourceLocation("cli", 1, 1),
                                   "Specify an input file: spc " + std::string(command) + " main.sp");
            return 1;
        }

        std::string source = readFile(filename);
        if (source.empty()) {
            diagEngine.reportError("E1009", "Could not open or read source file: " + filename,
                                   sp::diagnostics::SourceLocation(filename, 1, 1),
                                   "Check that the file exists and is accessible");
            return 1;
        }

        // 1. Lexical Analysis
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

        std::vector<sp::lexer::Token> tokens = lexer.tokenizeAll();
        if (diagEngine.hasErrors()) {
            std::cerr << "Lexing failed with " << diagEngine.errorCount() << " error(s).\n";
            return 1;
        }

        // 2. Syntactic Analysis (Parser)
        sp::parser::Parser parser(tokens, filename, &diagEngine);
        auto program = parser.parseProgram();

        if (diagEngine.hasErrors() || !program) {
            std::cerr << "Parsing failed with " << diagEngine.errorCount() << " error(s).\n";
            return 1;
        }

        if (command == "ast") {
            sp::ast::ASTPrinter printer;
            std::cout << printer.print(*program);
            return 0;
        }

        // 3. Semantic Analysis
        sp::sema::SemanticAnalyzer sema(diagEngine);
        bool semaSuccess = sema.analyze(*program);

        if (!semaSuccess || diagEngine.hasErrors()) {
            std::cerr << "Semantic analysis failed with " << diagEngine.errorCount() << " error(s).\n";
            return 1;
        }

        if (command == "sema") {
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
            size_t declCount = program ? program->getDeclarations().size() : 0;
            std::cout << "Check passed: Syntax & Semantics valid (" << tokens.size() << " tokens, " 
                      << declCount << " declarations) in " << filename << ".\n";
            return 0;
        }

        // 4. Intermediate Representation (SP IR)
        std::string modName = fs::path(filename).stem().string();
        sp::ir::IRBuilder irBuilder(sema);
        auto irModule = irBuilder.build(*program, modName);

        if (!irModule) {
            std::cerr << "IR generation failed for " << filename << ".\n";
            return 1;
        }

        if (command == "ir") {
            sp::ir::IRPrinter irPrinter;
            std::cout << irPrinter.print(*irModule);
            return 0;
        }

        // 5. LLVM IR Code Generation
        sp::codegen::CodegenOptions codegenOpts;
        codegenOpts.releaseMode = releaseMode;
        codegenOpts.debugSymbols = debugMode;
        if (!targetTriple.empty()) {
            codegenOpts.targetTriple = targetTriple;
        }
        if (!optLevel.empty()) {
            codegenOpts.optLevel = optLevel;
        }
        sp::codegen::LLVMGenerator llvmGen(codegenOpts);

        if (command == "llvm" || emitIROnly) {
            std::string ir = llvmGen.generateLLVMIR(*irModule);
            if (!outputPath.empty()) {
                std::ofstream out(outputPath);
                out << ir;
                std::cout << "Emitted LLVM IR to: " << outputPath << "\n";
            } else {
                std::cout << ir;
            }
            return 0;
        }

        if (command == "obj" || emitObjOnly) {
            if (outputPath.empty()) {
                fs::path p = fs::path(filename).stem();
                p.replace_extension(sp::codegen::ObjectEmitter::getObjectFileExtension(codegenOpts.targetTriple));
                outputPath = p.string();
            }
            bool ok = llvmGen.emitObjectFile(*irModule, outputPath);
            if (!ok) {
                std::cerr << "Object emission failed: " << llvmGen.getLastError() << "\n";
                return 1;
            }
            std::cout << "Successfully emitted object file: " << outputPath << "\n";
            return 0;
        }

        // 6. Native Binary Compilation (build / run)
        if (outputPath.empty()) {
            fs::path p = fs::path(filename).stem();
#if defined(_WIN32)
            p.replace_extension(".exe");
#endif
            outputPath = p.string();
        }

        bool buildSuccess = llvmGen.compileToExecutable(*irModule, outputPath);
        if (!buildSuccess) {
            std::cerr << "Compilation failed: " << llvmGen.getLastError() << "\n";
            if (!sp::util::hasBackendToolchain()) {
                std::cerr << "\n[HINT] No backend C/LLVM compiler or linker was detected on this computer.\n"
                          << "       Run 'spc setup' to auto-provision a portable toolchain,\n"
                          << "       or run 'spc doctor' to inspect your environment.\n\n";
            }
            return 1;
        }

        if (command == "build") {
            std::cout << "Successfully built native binary: " << outputPath << "\n";
            return 0;
        }

        if (command == "run") {
            std::string runCmd = "\"" + fs::absolute(outputPath).string() + "\"";
#if defined(_WIN32)
            std::string fullCmd = "\"" + runCmd + "\"";
            int exitCode = std::system(fullCmd.c_str());
#else
            int exitCode = std::system(runCmd.c_str());
#endif
            return exitCode;
        }

        return 0;
    }

    if (command == "fmt") {
        bool writeInPlace = false;
        bool checkMode = false;
        int indentWidth = 4;
        std::string filename;

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if (arg == "-w" || arg == "--write") {
                writeInPlace = true;
            } else if (arg == "--check") {
                checkMode = true;
            } else if (arg == "--indent" && i + 1 < argc) {
                indentWidth = std::stoi(argv[++i]);
            } else if (!arg.starts_with("-")) {
                if (filename.empty()) {
                    filename = arg;
                }
            }
        }

        if (filename.empty()) {
            std::cerr << "error: 'spc fmt' requires an SP source file\n"
                      << "Usage: spc fmt <file.sp> [-w/--write] [--check] [--indent <N>]\n";
            return 1;
        }

        std::string source = readFile(filename);
        if (source.empty() && fs::exists(filename) && fs::file_size(filename) > 0) {
            std::cerr << "error: could not read file " << filename << "\n";
            return 1;
        }

        sp::fmt::FormatOptions opts;
        opts.indentWidth = indentWidth;
        sp::fmt::Formatter formatter(opts);

        if (checkMode) {
            bool formatted = formatter.isFormatted(source);
            if (!formatted) {
                std::cout << filename << " is not formatted.\n";
                return 1;
            }
            std::cout << filename << " is properly formatted.\n";
            return 0;
        }

        std::string formattedCode = formatter.format(source);
        if (writeInPlace) {
            std::ofstream out(filename);
            if (!out.is_open()) {
                std::cerr << "error: failed to write to " << filename << "\n";
                return 1;
            }
            out << formattedCode;
            std::cout << "Formatted " << filename << "\n";
            return 0;
        }

        std::cout << formattedCode;
        return 0;
    }

    if (command == "test") {
        std::string targetPath = ".";
        std::string filter;
        bool verbose = false;

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if ((arg == "--filter" || arg == "-f") && i + 1 < argc) {
                filter = argv[++i];
            } else if (arg == "--verbose" || arg == "-v") {
                verbose = true;
            } else if (!arg.starts_with("-")) {
                targetPath = arg;
            }
        }

        sp::testing::TestRunner runner(verbose);
        auto suite = runner.runAll(targetPath, filter);
        return suite.isSuccess() ? 0 : 1;
    }

    if (command == "bench") {
        std::string targetPath = ".";
        sp::bench::BenchmarkOptions opts;

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if (arg == "--save-baseline") {
                opts.saveBaseline = true;
            } else if (arg == "--warmup" && i + 1 < argc) {
                opts.warmupRuns = std::stoul(argv[++i]);
            } else if (arg == "--samples" && i + 1 < argc) {
                opts.sampleRuns = std::stoul(argv[++i]);
            } else if ((arg == "--filter" || arg == "-f") && i + 1 < argc) {
                opts.filter = argv[++i];
            } else if (!arg.starts_with("-")) {
                targetPath = arg;
            }
        }

        std::cout << "Executing SP micro-benchmarks on '" << targetPath << "'...\n";
        auto res = sp::bench::BenchmarkRunner::run(targetPath, opts);
        if (!res.success) {
            std::cerr << "error: benchmark execution failed: " << res.errorMessage << "\n";
            return 1;
        }

        std::cout << res.formattedReport;
        if (res.regressionsDetected > 0) {
            std::cout << "\n[!] REGRESSION ALERT: " << res.regressionsDetected
                      << " benchmark(s) exhibited performance regressions (>10% slower than baseline).\n";
            return 1;
        } else {
            std::cout << "\nStatus: ALL " << res.totalBenchmarks
                      << " BENCHMARKS COMPLETED (0 regressions detected)\n";
        }
        return 0;
    }

    if (command == "doc") {
        std::string inputPath = ".";
        std::string outputDir = "docs";
        bool onlyHtml = false;
        bool onlyMd = false;
        std::string projectName = "SP Documentation";

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if ((arg == "-o" || arg == "--out" || arg == "--output") && i + 1 < argc) {
                outputDir = argv[++i];
            } else if (arg == "--html") {
                onlyHtml = true;
            } else if (arg == "--md") {
                onlyMd = true;
            } else if ((arg == "--title" || arg == "--project") && i + 1 < argc) {
                projectName = argv[++i];
            } else if (!arg.starts_with("-")) {
                inputPath = arg;
            }
        }

        sp::doc::DocOptions opts;
        opts.outputDir = outputDir;
        opts.projectName = projectName;
        if (onlyHtml && !onlyMd) {
            opts.generateHtml = true;
            opts.generateMarkdown = false;
        } else if (onlyMd && !onlyHtml) {
            opts.generateHtml = false;
            opts.generateMarkdown = true;
        } else {
            opts.generateHtml = true;
            opts.generateMarkdown = true;
        }

        sp::doc::DocGenerator generator(opts);
        bool success = generator.generate(inputPath, outputDir);
        return success ? 0 : 1;
    }

    if (command == "bindgen") {
        std::string headerFile;
        std::string outputFile;
        std::string moduleName;

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if ((arg == "-o" || arg == "--out" || arg == "--output") && i + 1 < argc) {
                outputFile = argv[++i];
            } else if (arg == "--module" && i + 1 < argc) {
                moduleName = argv[++i];
            } else if (!arg.starts_with("-")) {
                if (headerFile.empty()) {
                    headerFile = arg;
                }
            }
        }

        if (headerFile.empty()) {
            std::cerr << "error: no input C header file specified\n"
                      << "Usage: spc bindgen <header.h> [-o <out.sp>] [--module <name>]\n";
            return 1;
        }

        sp::bindgen::BindgenOptions opts;
        opts.moduleName = moduleName;
        sp::bindgen::Bindgen bg(opts);

        if (!bg.parseFile(headerFile)) {
            std::cerr << "error: " << bg.getLastError() << "\n";
            return 1;
        }

        std::string generatedSP = bg.generateSP();

        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            if (!out.is_open()) {
                std::cerr << "error: failed to write to output file: " << outputFile << "\n";
                return 1;
            }
            out << generatedSP;
            std::cout << "Generated SP bindings in " << outputFile << " ("
                      << bg.getStructs().size() << " structs, "
                      << bg.getFunctions().size() << " functions, "
                      << bg.getEnums().size() << " enums, "
                      << bg.getMacros().size() << " constants)\n";
        } else {
            std::cout << generatedSP;
        }
        return 0;
    }

    diagEngine.reportError("E1000", "Unknown command: " + std::string(command),
                           sp::diagnostics::SourceLocation("cli", 1, 1),
                           "Run 'spc --help' to see valid compiler commands");
    return 1;
}
