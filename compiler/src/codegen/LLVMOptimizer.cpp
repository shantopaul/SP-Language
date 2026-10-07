#include "sp/codegen/LLVMOptimizer.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <cstdlib>
#include <cctype>
#include <iostream>

namespace fs = std::filesystem;

namespace sp::codegen {

LLVMOptimizer::LLVMOptimizer(OptimizerOptions options)
    : m_options(std::move(options)) {
}

std::string LLVMOptimizer::optLevelToString(OptLevel level) {
    switch (level) {
        case OptLevel::O0: return "-O0";
        case OptLevel::O1: return "-O1";
        case OptLevel::O2: return "-O2";
        case OptLevel::O3: return "-O3";
        case OptLevel::Os: return "-Os";
        case OptLevel::Oz: return "-Oz";
    }
    return "-O2";
}

OptLevel LLVMOptimizer::parseOptLevel(const std::string& str) {
    std::string s = str;
    for (auto& c : s) c = static_cast<char>(std::tolower(c));
    if (s == "0" || s == "o0" || s == "-o0") return OptLevel::O0;
    if (s == "1" || s == "o1" || s == "-o1") return OptLevel::O1;
    if (s == "2" || s == "o2" || s == "-o2") return OptLevel::O2;
    if (s == "3" || s == "o3" || s == "-o3") return OptLevel::O3;
    if (s == "s" || s == "os" || s == "-os") return OptLevel::Os;
    if (s == "z" || s == "oz" || s == "-oz") return OptLevel::Oz;
    return OptLevel::O2;
}

std::vector<std::string> LLVMOptimizer::getPassPipeline(OptLevel level) {
    switch (level) {
        case OptLevel::O0:
            return {};
        case OptLevel::O1:
            return {"mem2reg", "instcombine", "simplifycfg", "early-cse"};
        case OptLevel::O2:
            return {"sroa", "early-cse", "instcombine", "simplifycfg", "reassociate", "gvn", "mem2reg", "dce", "licm", "loop-unroll", "inline"};
        case OptLevel::O3:
            return {"sroa", "early-cse", "instcombine", "simplifycfg", "reassociate", "gvn", "mem2reg", "dce", "licm", "loop-unroll", "inline", "slp-vectorizer", "loop-vectorize"};
        case OptLevel::Os:
            return {"sroa", "simplifycfg", "instcombine", "dce", "inline-cost-minimizer"};
        case OptLevel::Oz:
            return {"sroa", "simplifycfg", "instcombine", "dce", "strip-debug-declare", "inline-cost-minimizer"};
    }
    return {};
}

std::string LLVMOptimizer::findOptTool() const {
    if (!m_options.customOptPath.empty() && fs::exists(m_options.customOptPath)) {
        return m_options.customOptPath;
    }

    const char* envOpt = std::getenv("SP_OPT");
    if (envOpt && fs::exists(envOpt)) return envOpt;

    const char* envClang = std::getenv("CLANG");
    if (envClang && fs::exists(envClang)) return envClang;

    const std::vector<std::string> searchDirs = {
        "K:/AndroidDev/tools/sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin",
        "C:/msys64/ucrt64/bin",
        "C:/msys64/mingw64/bin",
        "C:/Program Files/LLVM/bin",
        "C:/Program Files (x86)/LLVM/bin"
    };

    // First search for clang/clang++ which reliably supports -x ir -O2 -S -emit-llvm
    for (const auto& dir : searchDirs) {
        fs::path p = fs::path(dir) / ("clang"
#if defined(_WIN32)
            ".exe"
#endif
        );
        if (fs::exists(p)) return p.string();
    }

    // Search for opt
    for (const auto& dir : searchDirs) {
        fs::path p = fs::path(dir) / ("opt"
#if defined(_WIN32)
            ".exe"
#endif
        );
        if (fs::exists(p)) return p.string();
    }

#if defined(_WIN32)
    return "clang.exe";
#else
    return "clang";
#endif
}

std::string LLVMOptimizer::runInProcessPasses(const std::string& inputIR) const {
    if (inputIR.empty()) return inputIR;

    std::istringstream stream(inputIR);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    // In-process Constant Folding pass
    if (m_options.enableConstantFolding) {
        std::regex constBinOp(R"(^\s*(%[A-Za-z0-9_\.]+)\s*=\s*(add|sub|mul|sdiv)\s+i32\s+(-?\d+)\s*,\s*(-?\d+)\s*$)");
        for (auto& l : lines) {
            std::smatch m;
            if (std::regex_search(l, m, constBinOp)) {
                std::string reg = m[1].str();
                std::string op = m[2].str();
                int32_t lhs = std::stoi(m[3].str());
                int32_t rhs = std::stoi(m[4].str());
                int32_t foldedVal = 0;
                bool ok = true;
                if (op == "add") foldedVal = lhs + rhs;
                else if (op == "sub") foldedVal = lhs - rhs;
                else if (op == "mul") foldedVal = lhs * rhs;
                else if (op == "sdiv" && rhs != 0) foldedVal = lhs / rhs;
                else ok = false;

                if (ok) {
                    l = "  " + reg + " = add i32 0, " + std::to_string(foldedVal) + " ; const-folded (" + op + " " + std::to_string(lhs) + ", " + std::to_string(rhs) + ")";
                }
            }
        }
    }

    std::ostringstream ss;
    for (const auto& l : lines) {
        ss << l << "\n";
    }
    return ss.str();
}

bool LLVMOptimizer::optimizeIR(const std::string& inputIR, std::string& outputIR) {
    m_lastError.clear();

    if (m_options.level == OptLevel::O0) {
        outputIR = inputIR;
        return true;
    }

    // Ensure input has target triple and datalayout for optimal LLVM target-machine lowering
    std::string preparedIR = inputIR;
    if (preparedIR.find("target triple") == std::string::npos) {
        preparedIR = "target triple = \"x86_64-w64-windows-gnu\"\n" + preparedIR;
    }

    // 1. Run in-process constant folding passes
    std::string processed = runInProcessPasses(preparedIR);

    // 2. Invoke LLVM optimizer
    std::string tool = findOptTool();
    fs::path tempDir = fs::temp_directory_path();
    static uint64_t s_optCounter = 0;
    uint64_t id = ++s_optCounter;
    fs::path inTmp = tempDir / ("sp_opt_in_" + std::to_string(id) + ".ll");
    fs::path outTmp = tempDir / ("sp_opt_out_" + std::to_string(id) + ".ll");

    {
        std::ofstream inFile(inTmp);
        if (!inFile) {
            m_lastError = "Could not create temporary IR file: " + inTmp.string();
            outputIR = processed;
            return false;
        }
        inFile << processed;
    }

    std::ostringstream cmd;
    std::string optFlag = optLevelToString(m_options.level);

    if (tool.find("opt") != std::string::npos) {
        // opt uses -passes flag in LLVM 17+
        std::string passArg = "default<" + optFlag.substr(1) + ">";
        cmd << "\"" << tool << "\" -S -passes=\"" << passArg << "\" \""
            << inTmp.string() << "\" -o \"" << outTmp.string() << "\"";
    } else {
        // Clang driver supports IR input directly
        cmd << "\"" << tool << "\" -x ir \"" << inTmp.string() << "\" "
            << optFlag << " -S -emit-llvm -o \"" << outTmp.string() << "\"";
    }

#if defined(_WIN32)
    std::string fullCmd = "\"" + cmd.str() + "\"";
    int result = std::system(fullCmd.c_str());
#else
    int result = std::system(cmd.str().c_str());
#endif

    std::error_code ec;
    if (result != 0 || !fs::exists(outTmp)) {
        // Fall back gracefully to in-process passes
        fs::remove(inTmp, ec);
        fs::remove(outTmp, ec);
        outputIR = processed;
        m_lastError = "LLVM optimizer execution failed with exit code: " + std::to_string(result);
        return false;
    }

    // Read optimized IR
    std::ifstream outFile(outTmp);
    if (!outFile) {
        m_lastError = "Failed to open optimized output IR file: " + outTmp.string();
        fs::remove(inTmp, ec);
        fs::remove(outTmp, ec);
        outputIR = processed;
        return false;
    }

    std::stringstream buffer;
    buffer << outFile.rdbuf();
    outputIR = buffer.str();

    fs::remove(inTmp, ec);
    fs::remove(outTmp, ec);
    return true;
}

bool LLVMOptimizer::optimizeIRInPlace(std::string& ir) {
    std::string optimized;
    if (optimizeIR(ir, optimized)) {
        ir = std::move(optimized);
        return true;
    }
    return false;
}

} // namespace sp::codegen
