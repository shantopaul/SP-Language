#include "sp/codegen/LLVMGenerator.h"
#include "sp/ir/IRPrinter.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace sp::codegen {

LLVMGenerator::LLVMGenerator(CodegenOptions options)
    : m_options(std::move(options)) {
    if (m_options.targetTriple.empty()) {
        m_options.targetTriple = getHostTargetTriple();
    }
    if (m_options.clangPath.empty()) {
        m_options.clangPath = findClangCompiler();
    }
    if (m_options.runtimeSourcePath.empty()) {
        m_options.runtimeSourcePath = findRuntimeSourcePath();
    }
    if (m_options.runtimeIncludePath.empty()) {
        m_options.runtimeIncludePath = findRuntimeIncludePath();
    }
}

std::string LLVMGenerator::getHostTargetTriple() {
#if defined(_WIN32)
  #if defined(__MINGW32__) || defined(__MINGW64__) || defined(__clang__)
    return "x86_64-w64-windows-gnu";
  #else
    return "x86_64-pc-windows-msvc";
  #endif
#elif defined(__APPLE__)
  #if defined(__aarch64__) || defined(__arm64__)
    return "arm64-apple-darwin";
  #else
    return "x86_64-apple-darwin";
  #endif
#elif defined(__linux__)
    return "x86_64-unknown-linux-gnu";
#else
    return "x86_64-unknown-linux-gnu";
#endif
}

std::string LLVMGenerator::findClangCompiler() {
    const char* envClang = std::getenv("SP_CLANG");
    if (envClang && fs::exists(envClang)) {
        return envClang;
    }

    const char* envClang2 = std::getenv("CLANG");
    if (envClang2 && fs::exists(envClang2)) {
        return envClang2;
    }

    // Check Android NDK LLVM toolchain path if present
    const std::vector<std::string> knownPaths = {
        "K:/AndroidDev/tools/sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe",
        "K:/AndroidDev/tools/sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe",
        "C:/msys64/ucrt64/bin/clang++.exe",
        "C:/msys64/mingw64/bin/clang++.exe",
        "C:/Program Files/LLVM/bin/clang++.exe",
        "C:/Program Files (x86)/LLVM/bin/clang++.exe"
    };

    for (const auto& path : knownPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    // Default to clang++ in PATH
#if defined(_WIN32)
    return "clang++.exe";
#else
    return "clang++";
#endif
}

std::string LLVMGenerator::findRuntimeSourcePath() {
    const char* envRt = std::getenv("SP_RUNTIME_SRC");
    if (envRt && fs::exists(envRt)) {
        return envRt;
    }

    const char* envHome = std::getenv("SP_HOME");
    if (envHome && fs::exists(envHome)) {
        fs::path p1 = fs::path(envHome) / "share" / "sp" / "runtime" / "Runtime.cpp";
        if (fs::exists(p1)) return fs::absolute(p1).string();
        fs::path p2 = fs::path(envHome) / "runtime" / "src" / "Runtime.cpp";
        if (fs::exists(p2)) return fs::absolute(p2).string();
    }

#ifdef SP_PROJECT_ROOT
    fs::path projRt = fs::path(SP_PROJECT_ROOT) / "runtime" / "src" / "Runtime.cpp";
    if (fs::exists(projRt)) {
        return fs::absolute(projRt).string();
    }
#endif

    const std::vector<std::string> candidates = {
        "runtime/src/Runtime.cpp",
        "../runtime/src/Runtime.cpp",
        "../../runtime/src/Runtime.cpp"
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return fs::absolute(candidate).string();
        }
    }

    return "runtime/src/Runtime.cpp";
}

std::string LLVMGenerator::findRuntimeIncludePath() {
    const char* envInc = std::getenv("SP_RUNTIME_INC");
    if (envInc && fs::exists(envInc)) {
        return envInc;
    }

    const char* envHome = std::getenv("SP_HOME");
    if (envHome && fs::exists(envHome)) {
        fs::path p1 = fs::path(envHome) / "include";
        if (fs::exists(p1)) return fs::absolute(p1).string();
        fs::path p2 = fs::path(envHome) / "runtime" / "include";
        if (fs::exists(p2)) return fs::absolute(p2).string();
    }

#ifdef SP_PROJECT_ROOT
    fs::path projInc = fs::path(SP_PROJECT_ROOT) / "runtime" / "include";
    if (fs::exists(projInc)) {
        return fs::absolute(projInc).string();
    }
#endif

    const std::vector<std::string> candidates = {
        "runtime/include",
        "../runtime/include",
        "../../runtime/include"
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return fs::absolute(candidate).string();
        }
    }

    return "runtime/include";
}

std::string LLVMGenerator::generateLLVMIR(const ir::IRModule& module) {
    std::ostringstream ss;
    ss << "; ModuleID = '" << module.getName() << "'\n";
    ss << "source_filename = \"" << module.getName() << ".sp\"\n";

    if (!m_options.targetTriple.empty()) {
        ss << "target triple = \"" << m_options.targetTriple << "\"\n";
    }

    if (!m_options.dataLayout.empty()) {
        ss << "target datalayout = \"" << m_options.dataLayout << "\"\n";
    }

    ss << "\n";

    ir::IRPrinter printer;
    std::string body = printer.print(module);

    // Skip the first header lines of IRPrinter because we already emitted triple and layout
    size_t secondLinePos = body.find("source_filename = ");
    if (secondLinePos != std::string::npos) {
        size_t nextPos = body.find('\n', secondLinePos);
        if (nextPos != std::string::npos) {
            body = body.substr(nextPos + 1);
        }
    }

    ss << body;
    return ss.str();
}

bool LLVMGenerator::emitLLVMIRToFile(const ir::IRModule& module, const std::string& filePath) {
    std::ofstream out(filePath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        m_lastError = "Could not open file for writing: " + filePath;
        return false;
    }

    out << generateLLVMIR(module);
    return out.good();
}

bool LLVMGenerator::compileToExecutable(const ir::IRModule& module, const std::string& outputExePath) {
    // 1. Emit LLVM IR to a temporary file
    std::string tempLLPath = outputExePath + ".tmp.ll";
    if (!emitLLVMIRToFile(module, tempLLPath)) {
        return false;
    }

    // 2. Resolve clang compiler
    std::string clang = m_options.clangPath;
    if (clang.empty()) {
        clang = findClangCompiler();
    }

    // 3. Resolve runtime source and include
    std::string rtSrc = m_options.runtimeSourcePath;
    std::string rtInc = m_options.runtimeIncludePath;

    // 4. Construct compilation command
    std::ostringstream cmd;
    cmd << "\"" << clang << "\" ";
    cmd << "\"" << tempLLPath << "\" ";

    if (fs::exists(rtSrc)) {
        cmd << "\"" << rtSrc << "\" ";
    }
    if (fs::exists(rtInc)) {
        cmd << "-I \"" << rtInc << "\" ";
    }

    if (m_options.releaseMode) {
        cmd << "-O3 ";
    } else if (m_options.debugSymbols) {
        cmd << "-g -O0 ";
    } else {
        cmd << "-O0 ";
    }

    cmd << "-o \"" << outputExePath << "\"";

    // 5. Execute compilation command
#if defined(_WIN32)
    std::string fullCmd = "\"" + cmd.str() + "\"";
    int exitCode = std::system(fullCmd.c_str());
#else
    int exitCode = std::system(cmd.str().c_str());
#endif

    // 6. Clean up temporary LL file if successful
    if (exitCode == 0) {
        std::error_code ec;
        fs::remove(tempLLPath, ec);
    } else {
        m_lastError = "Clang compilation failed with exit code: " + std::to_string(exitCode);
        return false;
    }

    return true;
}

} // namespace sp::codegen
