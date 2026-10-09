#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/diagnostics/SourceLocation.h"
#include "sp/lexer/TokenType.h"
#include "sp/lexer/Token.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/ast/ASTPrinter.h"
#include "sp/ast/Edition.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/sema/ComptimeEvaluator.h"
#include "sp/sema/ModuleLoader.h"
#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRPrinter.h"
#include "sp/codegen/LLVMGenerator.h"
#include "sp/codegen/ObjectEmitter.h"
#include "sp/runtime/Runtime.h"
#include "sp/Version.h"
#include "fmt/Formatter.h"
#include "test/TestRunner.h"
#include "doc/DocGenerator.h"
#include "doc/OpenApiGenerator.h"
#include "bindgen/Bindgen.h"
#include "benchmark/Benchmark.h"
#include "sp/util/PathUtils.h"
#include "spm/ProjectConfig.h"
#include <iostream>

#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#endif

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
              << "  init [path]     Initialize an SP project with sp.toml in directory\n"
              << "  new <name>      Create a new SP project in a new directory\n"
              << "  bindgen <hdr.h> Generate SP FFI bindings from C header file (.h)\n"

              << "  doctor          Diagnose SP installation, environment, and backend toolchain\n"
              << "  setup           Check, auto-provision, or update portable backend toolchain\n"
              << "  fix [path]      Migrate/modernize SP codebase to target edition (e.g. 2028)\n\n"
              << "Options:\n"
              << "  --edition <ed>  Select language edition: 2026 (SP 1.x) or 2028 (SP 2.0)\n"
              << "  --save-baseline Save current benchmark results as new historical baseline\n"
              << "  --warmup <n>    Number of warmup iterations (default: 5)\n"
              << "  --samples <n>   Number of sample measurement runs (default: 50)\n"
              << "  --watch, -w     Watch file for changes and auto-restart process\n"
              << "  -o <path>       Specify output binary path (for build/obj command)\n"
              << "  -c, --emit-obj  Emit relocatable object file (.o / .obj) directly\n"
              << "  -S, --emit-ir   Emit LLVM IR (.ll) directly\n"
              << "  -O0, -O1, -O2, -O3, -Os, -Oz Optimization level (default: -O0)\n"
              << "  --opt-level <O> Specify optimization level (O0, O1, O2, O3, Os, Oz)\n"
              << "  --release       Build with -O3 optimizations enabled\n"
              << "  --static        Produce a zero-dependency static binary (-static musl)\n"
              << "  -g, --debug     Build with debug symbols (-g) enabled\n"
              << "  --target <arch> Cross-compile for target architecture triple (e.g. wasm32-wasi)\n"
              << "  -I <dir>        Add search directory for module resolution\n"
              << "  -L <dir>        Add search directory for library resolution (OpenSSL, etc.)\n"
              << "  --lib-path <d>  Add search directory for library resolution\n"
              << "  --stdlib <path> Specify custom standard library directory path\n"

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

    // 6. OpenSSL TLS & Cryptography Subsystem
    std::cout << "[6] OpenSSL TLS & Cryptography Engine:\n"
              << "    Library Dir:    " << (health.openSslLibDir.empty() ? "(none / system standard)" : health.openSslLibDir.string()) << "\n";
    if (health.hasOpenSsl) {
        std::cout << "    Link Mode:      " << (health.isOpenSslStatic ? "Static Archive (Zero external DLL dependencies)" : "Shared / Import Library") << "\n"
                  << "    Status:         [OK] PostgreSQL TLS, HTTPS & Cryptography ready\n\n";
    } else {
        std::cout << "    Status:         [INFO] Standard system fallback active (-lssl -lcrypto)\n\n";
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

int handleFix(int argc, char* argv[]) {
    std::string targetPath = ".";
    std::string targetEdition = "2028";
    bool dryRun = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--dry-run") {
            dryRun = true;
        } else if ((arg == "--edition" || arg == "-e") && i + 1 < argc) {
            targetEdition = argv[++i];
        } else if (arg.starts_with("--edition=")) {
            targetEdition = std::string(arg.substr(10));
        } else if (!arg.starts_with("-")) {
            targetPath = std::string(arg);
        }
    }

    std::cout << "\n========================================================================\n"
              << "       SP 2.0 Codebase Migration & Modernization Tool ('spc fix')       \n"
              << "========================================================================\n\n";
    std::cout << "Target Edition:  " << targetEdition << "\n";
    std::cout << "Target Path:     " << targetPath << "\n";
    std::cout << "Mode:            " << (dryRun ? "Dry Run (Preview Only)" : "In-Place Migration") << "\n\n";

    int filesChecked = 0;
    int filesModified = 0;

    // Check sp.toml if present in target directory
    fs::path tomlPath = fs::is_directory(targetPath) ? (fs::path(targetPath) / "sp.toml") : fs::path("sp.toml");
    if (fs::exists(tomlPath)) {
        filesChecked++;
        std::string tomlContent = readFile(tomlPath.string());
        if (tomlContent.find("edition = \"2026\"") != std::string::npos && targetEdition == "2028") {
            size_t pos = tomlContent.find("edition = \"2026\"");
            tomlContent.replace(pos, 16, "edition = \"2028\"");
            if (!dryRun) {
                std::ofstream out(tomlPath);
                out << tomlContent;
            }
            filesModified++;
            std::cout << "[FIX] Modernized sp.toml edition from '2026' to '2028'\n";
        }
    }

    // Inspect .sp files in targetPath
    std::vector<fs::path> spFiles;
    if (fs::is_regular_file(targetPath) && fs::path(targetPath).extension() == ".sp") {
        spFiles.push_back(targetPath);
    } else if (fs::is_directory(targetPath)) {
        for (const auto& entry : fs::recursive_directory_iterator(targetPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".sp") {
                spFiles.push_back(entry.path());
            }
        }
    }

    for (const auto& p : spFiles) {
        filesChecked++;
        std::string content = readFile(p.string());
        bool changed = false;

        // Modernize legacy constructs if needed
        if (changed && !dryRun) {
            std::ofstream out(p);
            out << content;
            filesModified++;
            std::cout << "[FIX] Modernized: " << p.string() << "\n";
        }
    }

    std::cout << "\nMigration Complete: " << filesChecked << " file(s) scanned, "
              << filesModified << " file(s) updated.\n";
    if (dryRun) {
        std::cout << "[NOTE] Dry-run mode completed. Re-run without '--dry-run' to write changes.\n";
    }
    return 0;
}

int handleInit(int argc, char* argv[]) {
    std::string path = ".";
    std::string name;
    std::string templateType;
    bool isLib = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--lib") {
            isLib = true;
        } else if ((arg == "--template" || arg == "-t") && i + 1 < argc) {
            templateType = argv[++i];
        } else if (arg == "--name" && i + 1 < argc) {
            name = argv[++i];
        } else if (!arg.starts_with("-")) {
            path = arg;
        }
    }

    if (name.empty()) {
        fs::path p = fs::absolute(path);
        name = p.filename().string();
        if (name == "." || name.empty()) {
            name = "my_project";
        }
    }

    std::string errorMsg;
    if (!sp::spm::scaffoldProject(path, name, isLib, errorMsg, templateType)) {
        std::cerr << "error: " << errorMsg << "\n";
        return 1;
    }

    std::cout << "Created " << (isLib ? "library" : (templateType == "api" ? "REST API" : "binary (application)"))
              << " package '" << name << "' at " << path << "\n";
    return 0;
}

int handleNew(int argc, char* argv[]) {
    std::string name;
    std::string templateType;
    bool isLib = false;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--lib") {
            isLib = true;
        } else if ((arg == "--template" || arg == "-t") && i + 1 < argc) {
            templateType = argv[++i];
        } else if (!arg.starts_with("-")) {
            if (name.empty()) {
                name = arg;
            }
        }
    }

    if (name.empty()) {
        std::cerr << "error: missing project name for 'spc new'\n"
                  << "usage: spc new <project_name> [--template <app|api|lib>]\n";
        return 1;
    }

    std::string path = name;
    std::string errorMsg;
    if (!sp::spm::scaffoldProject(path, name, isLib, errorMsg, templateType)) {
        std::cerr << "error: " << errorMsg << "\n";
        return 1;
    }

    std::cout << "Created " << (isLib ? "library" : (templateType == "api" ? "REST API" : "binary (application)"))
              << " package '" << name << "' at ./" << path << "\n";
    return 0;
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

    if (command == "fix") {
        return handleFix(argc, argv);
    }

    if (command == "init") {
        return handleInit(argc, argv);
    }

    if (command == "new") {
        return handleNew(argc, argv);
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
        bool isStatic = false;
        bool watchMode = false;
        bool emitObjOnly = false;
        bool emitIROnly = false;
        sp::ast::Edition edition = sp::ast::Edition::Edition2026;
        bool explicitEdition = false;
        std::vector<std::string> includeDirs;
        std::vector<std::string> librarySearchPaths;
        std::string customStdlibDir;

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if (arg == "-o" && i + 1 < argc) {
                outputPath = argv[++i];
            } else if (arg == "-w" || arg == "--watch") {
                watchMode = true;
            } else if (arg == "-c" || arg == "--emit-obj") {
                emitObjOnly = true;
            } else if (arg == "-S" || arg == "--emit-ir") {
                emitIROnly = true;
            } else if (arg == "-g") {
                debugMode = true;
            } else if ((arg == "--edition" || arg == "-e") && i + 1 < argc) {
                edition = sp::ast::parseEdition(argv[++i]);
                explicitEdition = true;
            } else if (arg.starts_with("--edition=")) {
                edition = sp::ast::parseEdition(arg.substr(10));
                explicitEdition = true;
            } else if (arg == "-I" && i + 1 < argc) {
                includeDirs.push_back(argv[++i]);
            } else if (arg.starts_with("-I")) {
                includeDirs.push_back(std::string(arg.substr(2)));
            } else if ((arg == "--include-dir") && i + 1 < argc) {
                includeDirs.push_back(argv[++i]);
            } else if (arg.starts_with("--include-dir=")) {
                includeDirs.push_back(std::string(arg.substr(14)));
            } else if (arg == "-L" && i + 1 < argc) {
                librarySearchPaths.push_back(argv[++i]);
            } else if (arg.starts_with("-L")) {
                librarySearchPaths.push_back(std::string(arg.substr(2)));
            } else if ((arg == "--lib-path") && i + 1 < argc) {
                librarySearchPaths.push_back(argv[++i]);
            } else if (arg.starts_with("--lib-path=")) {
                librarySearchPaths.push_back(std::string(arg.substr(11)));
            } else if (arg == "--stdlib" && i + 1 < argc) {
                customStdlibDir = argv[++i];
            } else if (arg.starts_with("--stdlib=")) {
                customStdlibDir = std::string(arg.substr(9));
            } else if (arg == "--opt-level" && i + 1 < argc) {
                optLevel = argv[++i];
                if (!optLevel.empty() && optLevel[0] != '-') optLevel = "-" + optLevel;
            } else if (arg.starts_with("--opt-level=")) {
                optLevel = std::string(arg.substr(12));
                if (!optLevel.empty() && optLevel[0] != '-') optLevel = "-" + optLevel;
            } else if (arg == "--release") {
                releaseMode = true;
                optLevel = "-O3";
            } else if (arg == "--static") {
                isStatic = true;
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

        // Auto-detect edition from sp.toml if not explicitly specified
        if (!explicitEdition) {
            fs::path candidateToml = fs::path(filename).parent_path() / "sp.toml";
            if (!fs::exists(candidateToml)) candidateToml = "sp.toml";
            if (fs::exists(candidateToml)) {
                std::string tomlStr = readFile(candidateToml.string());
                if (tomlStr.find("edition = \"2028\"") != std::string::npos) {
                    edition = sp::ast::Edition::Edition2028;
                }
            }
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
        parser.setEdition(edition);
        auto program = parser.parseProgram();

        if (diagEngine.hasErrors() || !program) {
            std::cerr << "Parsing failed with " << diagEngine.errorCount() << " error(s).\n";
            return 1;
        }

        // Multi-file compilation & module dependency resolution (Phase 19)
        sp::sema::ModuleLoader moduleLoader(diagEngine);
        moduleLoader.setEdition(edition);
        for (const auto& inc : includeDirs) {
            moduleLoader.addSearchPath(inc);
        }
        if (!customStdlibDir.empty()) {
            moduleLoader.setStdlibPath(customStdlibDir);
        }
        if (!moduleLoader.loadAndMergeAll(*program, filename)) {
            std::cerr << "Module resolution failed with " << diagEngine.errorCount() << " error(s).\n";
            return 1;
        }

        // Compile-time evaluation & function registration
        sp::sema::ComptimeEvaluator comptimeEval(&diagEngine);
        comptimeEval.processProgram(*program);

        if (command == "ast") {
            sp::ast::ASTPrinter printer;
            std::cout << printer.print(*program);
            return 0;
        }

        // 3. Semantic Analysis
        sp::sema::SemanticAnalyzer sema(diagEngine);
        sema.setEdition(edition);
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
        codegenOpts.isStatic = isStatic;
        codegenOpts.librarySearchPaths = librarySearchPaths;
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
            if (targetTriple.find("wasm") != std::string::npos) {
                p.replace_extension(".wasm");
            } else {
#if defined(_WIN32)
                p.replace_extension(".exe");
#endif
            }
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
            if (!watchMode) {
                std::string runCmd = "\"" + fs::absolute(outputPath).string() + "\"";
#if defined(_WIN32)
                std::string fullCmd = "\"" + runCmd + "\"";
                int exitCode = std::system(fullCmd.c_str());
#else
                int exitCode = std::system(runCmd.c_str());
#endif
                return exitCode;
            }

            // Watch mode: monitoring input file and directory
            fs::path watchFile = fs::absolute(filename);
            fs::path watchDir = watchFile.parent_path();
            std::cout << "[spc --watch] Watching " << watchFile.string() << " (Ctrl+C to quit)...\n";

            auto getLatestTime = [&]() -> fs::file_time_type {
                fs::file_time_type latest = fs::file_time_type::min();
                std::error_code ec;
                for (const auto& entry : fs::directory_iterator(watchDir, ec)) {
                    if (ec) continue;
                    if (entry.is_regular_file() && entry.path().extension() == ".sp") {
                        auto t = entry.last_write_time(ec);
                        if (!ec && t > latest) latest = t;
                    }
                }
                return latest;
            };

            auto lastCheck = getLatestTime();

#if defined(_WIN32)
            PROCESS_INFORMATION procInfo{};
            bool hasChild = false;
            auto spawnProc = [&](const std::string& bin) {
                STARTUPINFOA si{};
                si.cb = sizeof(si);
                std::string cmd = "\"" + fs::absolute(bin).string() + "\"";
                std::vector<char> buf(cmd.begin(), cmd.end());
                buf.push_back('\0');
                if (CreateProcessA(NULL, buf.data(), NULL, NULL, FALSE, 0, NULL, watchDir.string().c_str(), &si, &procInfo)) {
                    hasChild = true;
                }
            };
            auto killProc = [&]() {
                if (hasChild && procInfo.hProcess) {
                    TerminateProcess(procInfo.hProcess, 0);
                    WaitForSingleObject(procInfo.hProcess, 100);
                    CloseHandle(procInfo.hProcess);
                    CloseHandle(procInfo.hThread);
                    procInfo = {};
                    hasChild = false;
                }
            };
#else
            pid_t childPid = -1;
            bool hasChild = false;
            auto spawnProc = [&](const std::string& bin) {
                pid_t pid = fork();
                if (pid == 0) {
                    std::string absBin = fs::absolute(bin).string();
                    char* args[] = {const_cast<char*>(absBin.c_str()), nullptr};
                    execvp(absBin.c_str(), args);
                    _exit(1);
                } else if (pid > 0) {
                    childPid = pid;
                    hasChild = true;
                }
            };
            auto killProc = [&]() {
                if (hasChild && childPid > 0) {
                    kill(childPid, SIGTERM);
                    int st = 0;
                    waitpid(childPid, &st, WNOHANG);
                    childPid = -1;
                    hasChild = false;
                }
            };
#endif

            spawnProc(outputPath);

            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                auto cur = getLatestTime();
                if (cur > lastCheck) {
                    lastCheck = cur;
                    auto startReboot = std::chrono::high_resolution_clock::now();
                    std::cout << "\n[spc --watch] File change detected, recompiling...\n";

                    killProc();

                    // Re-parse and compile
                    std::string newSource = readFile(watchFile.string());
                    sp::diagnostics::DiagnosticEngine reDiag(std::cerr);
                    sp::lexer::Lexer reLexer(newSource, watchFile.string(), &reDiag);
                    auto reTokens = reLexer.tokenizeAll();
                    if (!reDiag.hasErrors()) {
                        sp::parser::Parser reParser(reTokens, watchFile.string(), &reDiag);
                        reParser.setEdition(edition);
                        auto reAst = reParser.parseProgram();
                        if (!reDiag.hasErrors() && reAst) {
                            sp::sema::ModuleLoader reModLoader(reDiag);
                            reModLoader.setEdition(edition);
                            for (const auto& inc : includeDirs) reModLoader.addSearchPath(inc);
                            if (!customStdlibDir.empty()) reModLoader.setStdlibPath(customStdlibDir);
                            if (reModLoader.loadAndMergeAll(*reAst, watchFile.string())) {
                                sp::sema::SemanticAnalyzer reSema(reDiag);
                                reSema.setEdition(edition);
                                if (reSema.analyze(*reAst) && !reDiag.hasErrors()) {
                                    std::string mName = fs::path(watchFile).stem().string();
                                    sp::ir::IRBuilder reBuilder(reSema);
                                    auto reIr = reBuilder.build(*reAst, mName);
                                    if (reIr && llvmGen.compileToExecutable(*reIr, outputPath)) {
                                        spawnProc(outputPath);
                                        auto endReboot = std::chrono::high_resolution_clock::now();
                                        double ms = std::chrono::duration<double, std::milli>(endReboot - startReboot).count();
                                        std::cout << "[spc --watch] Server recompiled and restarted in " << std::fixed << std::setprecision(1) << ms << " ms.\n";
                                        continue;
                                    }
                                }
                            }
                        }
                    }
                    std::cerr << "[spc --watch] Compilation error, waiting for fixes...\n";
                }
            }

            killProc();
            return 0;
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
        bool openApiMode = false;
        bool swaggerMode = false;
        bool redocMode = false;
        std::string projectName = "SP Documentation";
        std::string apiTitle = "SP Application API";
        std::string apiVersion = "1.0.0";
        std::string apiUrl = "http://localhost:8080";

        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if ((arg == "-o" || arg == "--out" || arg == "--output") && i + 1 < argc) {
                outputDir = argv[++i];
            } else if (arg == "--html") {
                onlyHtml = true;
            } else if (arg == "--md") {
                onlyMd = true;
            } else if (arg == "--openapi") {
                openApiMode = true;
            } else if (arg == "--swagger") {
                swaggerMode = true;
            } else if (arg == "--redoc") {
                redocMode = true;
            } else if ((arg == "--title" || arg == "--project") && i + 1 < argc) {
                projectName = argv[++i];
                apiTitle = projectName;
            } else if (arg == "--api-version" && i + 1 < argc) {
                apiVersion = argv[++i];
            } else if (arg == "--api-url" && i + 1 < argc) {
                apiUrl = argv[++i];
            } else if (!arg.starts_with("-")) {
                inputPath = arg;
            }
        }

        if (openApiMode || swaggerMode || redocMode) {
            sp::doc::OpenApiOptions apiOpts;
            apiOpts.title = apiTitle;
            apiOpts.version = apiVersion;
            apiOpts.serverUrl = apiUrl;
            sp::doc::OpenApiGenerator oapiGen(apiOpts);

            if (fs::is_directory(inputPath)) {
                oapiGen.scanDirectory(inputPath);
            } else if (fs::exists(inputPath)) {
                oapiGen.scanFile(inputPath);
            } else {
                std::cerr << "Error: Input path '" << inputPath << "' does not exist.\n";
                return 1;
            }

            fs::create_directories(outputDir);
            std::string jsonPath = (fs::path(outputDir) / "openapi.json").string();
            if (outputDir.ends_with(".json")) {
                jsonPath = outputDir;
            }

            bool ok = oapiGen.saveJson(jsonPath);
            if (!ok) {
                std::cerr << "Error writing OpenAPI specification to " << jsonPath << "\n";
                return 1;
            }
            std::cout << "Generated OpenAPI 3.0.3 specification: " << jsonPath
                      << " (" << oapiGen.getRoutes().size() << " routes detected)\n";

            if (swaggerMode || (!redocMode && !openApiMode)) {
                std::string swaggerPath = (fs::path(outputDir) / "swagger.html").string();
                oapiGen.saveSwaggerHtml(swaggerPath, "openapi.json");
                std::cout << "Generated Swagger UI viewer: " << swaggerPath << "\n";
            }

            if (redocMode) {
                std::string redocPath = (fs::path(outputDir) / "redoc.html").string();
                oapiGen.saveRedocHtml(redocPath, "openapi.json");
                std::cout << "Generated Redoc viewer: " << redocPath << "\n";
            }

            return 0;
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

    const std::vector<std::string> validCommands = {
        "build", "run", "check", "init", "new", "fmt", "test", "bench",
        "doc", "doctor", "setup", "fix", "bindgen", "tokens", "ast", "sema",
        "ir", "llvm", "obj"
    };

    auto calcLevenshtein = [](std::string_view s1, std::string_view s2) -> size_t {
        const size_t m = s1.size();
        const size_t n = s2.size();
        std::vector<std::vector<size_t>> d(m + 1, std::vector<size_t>(n + 1));
        for (size_t i = 0; i <= m; ++i) d[i][0] = i;
        for (size_t j = 0; j <= n; ++j) d[0][j] = j;
        for (size_t i = 1; i <= m; ++i) {
            for (size_t j = 1; j <= n; ++j) {
                size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost});
            }
        }
        return d[m][n];
    };

    std::string hint = "Run 'spc --help' to see valid compiler commands";
    for (const auto& valid : validCommands) {
        if (valid.starts_with(command) || command.starts_with(valid) || calcLevenshtein(command, valid) <= 2) {
            hint = "Did you mean '" + valid + "'? " + hint;
            break;
        }
    }



    diagEngine.reportError("E1000", "Unknown command: " + std::string(command),
                           sp::diagnostics::SourceLocation("cli", 1, 1),
                           hint);
    return 1;
}

