#include "sp/codegen/LLVMGenerator.h"
#include "sp/codegen/ObjectEmitter.h"
#include "sp/codegen/LinkerDriver.h"
#include "sp/codegen/DebugInfoGenerator.h"
#include "sp/codegen/LLVMOptimizer.h"
#include "sp/ir/IRPrinter.h"
#include "sp/ir/IRVerifier.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <atomic>
#include <chrono>


#include "sp/util/PathUtils.h"

namespace fs = std::filesystem;

namespace sp::codegen {

LLVMGenerator::LLVMGenerator(CodegenOptions options)
    : m_options(std::move(options)) {
    if (m_options.targetTriple.empty()) {
        m_options.targetTriple = getHostTargetTriple();
    }
    if (m_options.targetTriple.find("wasm") != std::string::npos) {
        if (m_options.dataLayout.empty()) {
            m_options.dataLayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20";
        }
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
    auto p = util::findClangCompiler();
    if (!p.empty() && fs::exists(p)) {
        return p.string();
    }
    return "";
}

std::string LLVMGenerator::findRuntimeSourcePath() {
    auto p = util::getRuntimeSourcePath();
    if (!p.empty() && fs::exists(p)) {
        return p.string();
    }
    return "runtime/src/Runtime.cpp";
}

std::string LLVMGenerator::findRuntimeIncludePath() {
    auto p = util::getRuntimeIncludePath();
    if (!p.empty() && fs::exists(p)) {
        return p.string();
    }
    return "runtime/include";
}

std::string LLVMGenerator::findRuntimeLibPath() {
    return LinkerDriver::findPrecompiledRuntimeLib();
}

std::string LLVMGenerator::generateLLVMIR(const ir::IRModule& module) {
    // Phase 1 (pro-4.0.0): never emit IR text for a module that fails
    // verification. IRBuilder::build() already verifies, so this is
    // defense in depth for modules built by any other path. On failure
    // the last error is set and an empty string is returned; callers
    // check for that (see emitLLVMIRToFile / emitObjectFile).
    ir::IRVerifier verifier;
    if (!verifier.verify(module)) {
        m_lastError = "IR verification failed:";
        for (const auto& error : verifier.getErrors()) {
            m_lastError += "\n  " + error;
            std::cerr << "error[E1000]: IR verification failed: " << error << '\n';
        }
        return "";
    }

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
    // Generate first so a verification failure does not truncate an
    // existing output file.
    std::string ir = generateLLVMIR(module);
    if (ir.empty()) {
        // IR verification failed; m_lastError is already set by
        // generateLLVMIR.
        return false;
    }

    std::ofstream out(filePath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        m_lastError = "Could not open file for writing: " + filePath;
        return false;
    }

    out << ir;
    return out.good();
}

bool LLVMGenerator::emitObjectFile(const ir::IRModule& module, const std::string& outputObjPath) {
    std::string ir = generateLLVMIR(module);

    if (ir.empty()) {
        // IR verification failed; m_lastError is already set by
        // generateLLVMIR. Do not feed empty IR to the backend compiler.
        return false;
    }

    if (m_options.debugSymbols) {
        DebugInfoGenerator dbg(module.getName() + ".sp", "src");
        ir = dbg.injectDebugInfo(ir);
    }

    if (!m_options.optLevel.empty() && m_options.optLevel != "-O0") {
        OptimizerOptions optOpts;
        optOpts.level = LLVMOptimizer::parseOptLevel(m_options.optLevel);
        LLVMOptimizer opt(optOpts);
        opt.optimizeIRInPlace(ir);
    } else if (m_options.releaseMode) {
        OptimizerOptions optOpts;
        optOpts.level = OptLevel::O3;
        LLVMOptimizer opt(optOpts);
        opt.optimizeIRInPlace(ir);
    }

    ObjectEmitterOptions emitOpts;
    emitOpts.targetTriple = m_options.targetTriple;
    emitOpts.dataLayout = m_options.dataLayout;
    emitOpts.optLevel = m_options.optLevel.empty() ? (m_options.releaseMode ? "-O3" : "-O0") : m_options.optLevel;
    emitOpts.debugSymbols = m_options.debugSymbols;

    ObjectEmitter emitter(emitOpts);
    bool ok = emitter.emitObjectFromIR(ir, outputObjPath);
    if (!ok) {
        m_lastError = emitter.getLastError();
    }
    return ok;
}

bool LLVMGenerator::compileToExecutable(const ir::IRModule& module, const std::string& outputExePath) {
    // Determine safe temporary files in system temp directory to prevent write-permission failures
    std::error_code ec;
    fs::path tempDir = fs::temp_directory_path(ec);
    if (ec || !fs::exists(tempDir)) {
        tempDir = ".";
    }
    static std::atomic<uint64_t> s_tempNonce{0};
    uint64_t nonce = ++s_tempNonce;
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string baseTemp = "sp_tmp_" + std::to_string(now) + "_" + std::to_string(nonce);

    // Phase 12 Fast Path: Direct Object Emission + LinkerDriver with precompiled runtime library
    if (m_options.useDirectLinker) {
        std::string rtLib = m_options.runtimeLibPath.empty() ? findRuntimeLibPath() : m_options.runtimeLibPath;
        if (!rtLib.empty() && fs::exists(rtLib)) {
            std::string tempObj = (tempDir / (baseTemp + ObjectEmitter::getObjectFileExtension(m_options.targetTriple))).string();
            if (emitObjectFile(module, tempObj)) {
                LinkerDriverOptions linkOpts;
                linkOpts.targetTriple = m_options.targetTriple;
                linkOpts.runtimeLibPath = rtLib;
                linkOpts.linkerPath = m_options.customLinkerPath;
                linkOpts.isStatic = m_options.isStatic;
                linkOpts.librarySearchPaths = m_options.librarySearchPaths;
                // Phase 39 slice B: bare library names (-> -l<lib>).
                linkOpts.additionalLibraries = m_options.additionalLibraries;

                LinkerDriver linker(linkOpts);
                bool linked = linker.linkSingle(tempObj, outputExePath);
                std::error_code rmEc;
                fs::remove(tempObj, rmEc);

                if (linked && fs::exists(outputExePath)) {
                    return true;
                }
                m_lastError = linker.getLastError();
            }
        }
    }

    // Fallback: Legacy Clang driver compilation with Runtime.cpp
    // 1. Emit LLVM IR to a safe temporary file
    std::string tempLLPath = (tempDir / (baseTemp + ".tmp.ll")).string();
    if (!emitLLVMIRToFile(module, tempLLPath)) {
        // Fallback: attempt writing next to output executable if temp folder was blocked
        tempLLPath = outputExePath + ".tmp.ll";
        if (!emitLLVMIRToFile(module, tempLLPath)) {
            return false;
        }
    }


    // 2. Resolve clang compiler
    std::string clang = m_options.clangPath;
    if (clang.empty()) {
        clang = findClangCompiler();
    }
    if (clang.empty() || !fs::exists(clang)) {
        m_lastError = "No LLVM Clang compiler found on this system. Please run 'spc setup' to auto-provision a portable toolchain, or install LLVM Clang.";
        return false;
    }

    std::string clangLower = clang;
    for (auto& c : clangLower) c = std::tolower(static_cast<unsigned char>(c));
    if (clangLower.find("g++") != std::string::npos || clangLower.find("gcc") != std::string::npos) {
        m_lastError = "Unsupported backend compiler '" + clang + "'. GCC cannot compile LLVM IR and does not support '--target'. Please use LLVM Clang (clang++) or run 'spc setup'.";
        return false;
    }

    // 3. Resolve runtime source and include
    std::string rtSrc = m_options.runtimeSourcePath;
    std::string rtInc = m_options.runtimeIncludePath;

    // 4. Construct compilation command
    std::ostringstream cmd;
    cmd << "\"" << clang << "\" ";
    cmd << "-std=c++20 ";
    cmd << "-pthread ";
#if defined(_WIN32)
    cmd << "-lws2_32 -lcrypt32 -lbcrypt -lsecur32 -ladvapi32 -luser32 -lshell32 ";
#endif
    auto sslInfo = util::findOpenSsl(m_options.librarySearchPaths);
    if (sslInfo.found) {
        cmd << "-L\"" << sslInfo.libDir.string() << "\" ";
        if (!sslInfo.sslLib.empty() && !sslInfo.cryptoLib.empty()) {
            cmd << "\"" << sslInfo.sslLib.string() << "\" \"" << sslInfo.cryptoLib.string() << "\" ";
        } else {
            cmd << "-lssl -lcrypto ";
        }
    } else {
        cmd << "-lssl -lcrypto ";
    }
    for (const auto& spath : m_options.librarySearchPaths) {
        cmd << "-L\"" << spath << "\" ";
    }

    // Phase 39 slice B: bare library names (-> -l<lib>).
    for (const auto& lib : m_options.additionalLibraries) {
        cmd << "-l" << lib << " ";
    }

    if (m_options.isStatic) {
        cmd << "-static ";
    }

    if (!m_options.targetTriple.empty()) {
        cmd << "--target=" << m_options.targetTriple << " ";
    }

    cmd << "\"" << tempLLPath << "\" ";

    std::string rtLib = m_options.runtimeLibPath.empty() ? findRuntimeLibPath() : m_options.runtimeLibPath;
    if (fs::exists(rtLib)) {
        cmd << "\"" << rtLib << "\" ";
    } else if (fs::exists(rtSrc)) {
        cmd << "\"" << rtSrc << "\" ";
    }
    if (fs::exists(rtInc)) {
        cmd << "-I \"" << rtInc << "\" ";
    }

    if (!m_options.optLevel.empty()) {
        cmd << m_options.optLevel << " ";
        if (m_options.debugSymbols) {
            cmd << "-g ";
        }
    } else if (m_options.releaseMode) {
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

    // 6. Clean up temporary LL file
    std::error_code rmTempEc;
    fs::remove(tempLLPath, rmTempEc);

    if (exitCode != 0) {
        m_lastError = "Clang compilation failed with exit code: " + std::to_string(exitCode);
        return false;
    }

    return true;
}

} // namespace sp::codegen
