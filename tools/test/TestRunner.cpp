#include "TestRunner.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/lexer/Token.h"
#include "sp/lexer/TokenType.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <chrono>
#include <atomic>

namespace fs = std::filesystem;

namespace sp::testing {

static std::atomic<uint64_t> s_testIdCounter{0};

static std::string generateUniqueId() {
    uint64_t cnt = ++s_testIdCounter;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::to_string(now) + "_" + std::to_string(cnt);
}

TestRunner::TestRunner(bool verbose)
    : m_verbose(verbose) {}

std::string TestRunner::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::vector<std::string> TestRunner::discoverTestFiles(const std::string& path) {
    std::vector<std::string> files;
    fs::path p(path);

    if (fs::is_regular_file(p)) {
        if (p.extension() == ".sp") {
            files.push_back(fs::absolute(p).string());
        }
        return files;
    }

    if (fs::is_directory(p)) {
        // If "tests" directory exists within path, check it first
        fs::path testsDir = p / "tests";
        fs::path targetDir = fs::exists(testsDir) && fs::is_directory(testsDir) ? testsDir : p;

        std::error_code ec;
        for (const auto& entry : fs::recursive_directory_iterator(targetDir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".sp") {
                std::string stem = entry.path().stem().string();
                // Include files in tests/ or files matching *test*
                if (targetDir == testsDir || stem.find("test") != std::string::npos) {
                    files.push_back(fs::absolute(entry.path()).string());
                }
            }
        }
    }

    return files;
}

std::vector<TestCase> TestRunner::extractTestsFromFile(const std::string& filePath) {
    std::vector<TestCase> tests;
    std::string source = readFile(filePath);
    if (source.empty()) return tests;

    std::ostringstream diagStream;
    diagnostics::DiagnosticEngine diagEngine(diagStream);
    lexer::Lexer lexer(source, filePath, &diagEngine);
    auto tokens = lexer.tokenizeAll();

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == lexer::TokenType::KeywordTest) {
            if (i + 1 < tokens.size()) {
                const auto& nameTok = tokens[i + 1];
                std::string name;
                if (nameTok.type == lexer::TokenType::StringLiteral) {
                    name = nameTok.lexeme;
                    if (name.size() >= 2 && name.front() == '"' && name.back() == '"') {
                        name = name.substr(1, name.size() - 2);
                    }
                } else if (nameTok.type == lexer::TokenType::Identifier) {
                    name = nameTok.lexeme;
                }

                if (!name.empty()) {
                    std::string funcName = "__sp_test_";
                    for (char c : name) {
                        if (std::isalnum(static_cast<unsigned char>(c))) {
                            funcName += c;
                        } else {
                            funcName += '_';
                        }
                    }

                    tests.push_back(TestCase{
                        name,
                        funcName,
                        filePath,
                        static_cast<int>(tokens[i].location.line)
                    });
                }
            }
        }
    }

    return tests;
}

TestSuiteResult TestRunner::runFile(const std::string& filePath, const std::string& filter) {
    TestSuiteResult suite;
    auto testCases = extractTestsFromFile(filePath);
    std::string sourceCode = readFile(filePath);

    fs::path tempExeDir = fs::temp_directory_path() / "sp_test_runs";
    std::error_code ec;
    fs::create_directories(tempExeDir, ec);

    if (!testCases.empty()) {
        for (const auto& tc : testCases) {
            if (!filter.empty() && tc.name.find(filter) == std::string::npos) {
                continue;
            }

            suite.total++;
            TestResult tr;
            tr.name = tc.name;
            tr.filePath = filePath;

            // Generate single test runner harness
            std::string harnessSource = sourceCode + "\n\nfn main() {\n    " + tc.functionName + "();\n}\n";

            auto tStart = std::chrono::high_resolution_clock::now();

            std::ostringstream diagStream;
            diagnostics::DiagnosticEngine diagEngine(diagStream);

            lexer::Lexer lexer(harnessSource, filePath, &diagEngine);
            auto tokens = lexer.tokenizeAll();

            parser::Parser parser(tokens, filePath, &diagEngine);
            auto program = parser.parseProgram();

            if (!program || diagEngine.hasErrors()) {
                auto tEnd = std::chrono::high_resolution_clock::now();
                tr.durationMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
                tr.passed = false;
                tr.message = "Syntax error in test: " + diagStream.str();
                suite.failed++;
                suite.results.push_back(tr);
                continue;
            }

            sema::SemanticAnalyzer sema(diagEngine);
            if (!sema.analyze(*program) || diagEngine.hasErrors()) {
                auto tEnd = std::chrono::high_resolution_clock::now();
                tr.durationMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
                tr.passed = false;
                tr.message = "Semantic type error in test: " + diagStream.str();
                suite.failed++;
                suite.results.push_back(tr);
                continue;
            }

            ir::IRBuilder irBuilder(sema);
            auto irModule = irBuilder.build(*program, "test_harness");
            if (!irModule) {
                auto tEnd = std::chrono::high_resolution_clock::now();
                tr.durationMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
                tr.passed = false;
                tr.message = "IR generation failed for test harness";
                suite.failed++;
                suite.results.push_back(tr);
                continue;
            }

            codegen::CodegenOptions opts;
            codegen::LLVMGenerator llvmGen(opts);

            fs::path exePath = tempExeDir / ("test_harness_" + generateUniqueId() +
#if defined(_WIN32)
                ".exe"
#else
                ""
#endif
            );

            if (!llvmGen.compileToExecutable(*irModule, exePath.string())) {
                auto tEnd = std::chrono::high_resolution_clock::now();
                tr.durationMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
                tr.passed = false;
                tr.message = "Compilation failed: " + llvmGen.getLastError();
                suite.failed++;
                suite.results.push_back(tr);
                continue;
            }

            // Execute compiled test harness
            std::string runCmd = "\"" + exePath.string() + "\"";
            int exitCode = std::system(runCmd.c_str());

            auto tEnd = std::chrono::high_resolution_clock::now();
            tr.durationMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

            if (exitCode == 0) {
                tr.passed = true;
                suite.passed++;
            } else {
                tr.passed = false;
                tr.message = "Assertion failed (process returned exit code " + std::to_string(exitCode) + ")";
                suite.failed++;
            }

            fs::remove(exePath, ec);
            suite.results.push_back(tr);
        }
    } else {
        // Standalone test file without explicit "test" blocks (e.g. main() with assertions)
        suite.total++;
        TestResult tr;
        tr.name = fs::path(filePath).stem().string();
        tr.filePath = filePath;

        auto tStart = std::chrono::high_resolution_clock::now();

        std::ostringstream diagStream;
        diagnostics::DiagnosticEngine diagEngine(diagStream);
        lexer::Lexer lexer(sourceCode, filePath, &diagEngine);
        auto tokens = lexer.tokenizeAll();
        parser::Parser parser(tokens, filePath, &diagEngine);
        auto program = parser.parseProgram();

        if (program && !diagEngine.hasErrors()) {
            sema::SemanticAnalyzer sema(diagEngine);
            if (sema.analyze(*program)) {
                ir::IRBuilder irBuilder(sema);
                auto irModule = irBuilder.build(*program, "standalone_test");
                if (irModule) {
                    codegen::CodegenOptions opts;
                    codegen::LLVMGenerator llvmGen(opts);
                    fs::path exePath = tempExeDir / ("standalone_test_" + generateUniqueId() +
#if defined(_WIN32)
                        ".exe"
#else
                        ""
#endif
                    );

                    if (llvmGen.compileToExecutable(*irModule, exePath.string())) {
                        int exitCode = std::system(("\"" + exePath.string() + "\"").c_str());
                        auto tEnd = std::chrono::high_resolution_clock::now();
                        tr.durationMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
                        if (exitCode == 0) {
                            tr.passed = true;
                            suite.passed++;
                        } else {
                            tr.passed = false;
                            tr.message = "Test process failed (exit code " + std::to_string(exitCode) + ")";
                            suite.failed++;
                        }
                        fs::remove(exePath, ec);
                    } else {
                        tr.passed = false;
                        tr.message = "Compilation failed: " + llvmGen.getLastError();
                        suite.failed++;
                    }
                }
            }
        }

        if (!tr.passed && tr.message.empty()) {
            tr.message = "Compilation or parse error in test file";
            suite.failed++;
        }

        suite.results.push_back(tr);
    }

    return suite;
}

TestSuiteResult TestRunner::runAll(const std::string& targetPath, const std::string& filter) {
    TestSuiteResult totalSuite;
    auto files = discoverTestFiles(targetPath);

    if (files.empty()) {
        std::cout << "No SP test files found in " << targetPath << "\n";
        return totalSuite;
    }

    std::cout << "running tests across " << files.size() << " file(s)...\n\n";
    auto totalStart = std::chrono::high_resolution_clock::now();

    for (const auto& file : files) {
        if (m_verbose) {
            std::cout << "--> running test suite in: " << file << "\n";
        }
        auto fileRes = runFile(file, filter);
        for (const auto& r : fileRes.results) {
            std::cout << "test " << std::left << std::setw(36) << r.name << " ... ";
            if (r.passed) {
                std::cout << "ok (" << std::fixed << std::setprecision(1) << r.durationMs << " ms)\n";
            } else {
                std::cout << "FAILED (" << std::fixed << std::setprecision(1) << r.durationMs << " ms)\n";
                if (!r.message.empty()) {
                    std::cout << "    error: " << r.message << "\n";
                }
            }

            totalSuite.total++;
            if (r.passed) totalSuite.passed++;
            else totalSuite.failed++;
            totalSuite.results.push_back(r);
        }
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    totalSuite.totalDurationMs = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

    std::cout << "\ntest result: " << (totalSuite.isSuccess() ? "ok" : "FAILED") << ". "
              << totalSuite.passed << " passed; "
              << totalSuite.failed << " failed; finished in "
              << std::fixed << std::setprecision(1) << totalSuite.totalDurationMs << " ms\n";

    return totalSuite;
}

} // namespace sp::testing
