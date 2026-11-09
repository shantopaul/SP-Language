#include "sp/codegen/ObjectEmitter.h"
#include "sp/ir/IRPrinter.h"
#include "sp/util/PathUtils.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <atomic>
#include <chrono>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace sp::codegen {

// Phase 4 slice D: process id for unique temp-file names. The old
// `sp_ir_<steady_clock>_<per-process nonce>` scheme could collide between
// two processes started at the same time (same clock reading, same nonce
// sequence starting at 0), and `std::rand()` below is unseeded, so parallel
// processes produced identical temp names.
uint64_t currentPid() {
#if defined(_WIN32)
    return static_cast<uint64_t>(GetCurrentProcessId());
#else
    return static_cast<uint64_t>(::getpid());
#endif
}

ObjectEmitter::ObjectEmitter(ObjectEmitterOptions options)
    : m_options(std::move(options)) {
    if (m_options.targetTriple.empty()) {
        m_options.targetTriple = getHostTriple();
    }
}

std::string ObjectEmitter::getHostTriple() {
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

std::string ObjectEmitter::getObjectFileExtension(const std::string& targetTriple) {
    if (targetTriple.find("wasm") != std::string::npos) {
        return ".wasm";
    }
    if (targetTriple.find("windows-msvc") != std::string::npos) {
        return ".obj";
    }
    return ".o";
}

std::string ObjectEmitter::findLLVMTool(const std::string& toolName, const std::string& customPath) {
    if (!customPath.empty()) {
        fs::path p = fs::path(customPath) / (toolName +
#if defined(_WIN32)
            ".exe"
#else
            ""
#endif
        );
        if (fs::exists(p)) return p.string();
        if (fs::exists(customPath)) return customPath;
    }

    // 1. Check environment variables (rejecting GCC fallbacks)
    std::string envVarName = "SP_" + toolName;
    for (auto& c : envVarName) c = static_cast<char>(std::toupper(c));
    const char* envTool = std::getenv(envVarName.c_str());
    if (envTool && fs::exists(envTool)) {
        std::string s(envTool);
        std::string lower = s;
        for (auto& c : lower) c = std::tolower(static_cast<unsigned char>(c));
        if (lower.find("g++") == std::string::npos && lower.find("gcc") == std::string::npos) {
            return s;
        }
    }

    if (toolName == "clang" || toolName == "clang++") {
        const char* cEnv = std::getenv("CLANG");
        if (cEnv && fs::exists(cEnv)) {
            std::string s(cEnv);
            std::string lower = s;
            for (auto& c : lower) c = std::tolower(static_cast<unsigned char>(c));
            if (lower.find("g++") == std::string::npos && lower.find("gcc") == std::string::npos) {
                return s;
            }
        }
    }

    // 2. Check bundled toolchain in SP_HOME/toolchain/bin
    auto home = util::getSPHome();
    if (!home.empty()) {
        fs::path p = home / "toolchain" / "bin" / (toolName +
#if defined(_WIN32)
            ".exe"
#else
            ""
#endif
        );
        if (fs::exists(p)) return p.string();
    }

    auto exeDir = util::getExecutableDir();
    if (!exeDir.empty()) {
        fs::path p = exeDir / ".." / "toolchain" / "bin" / (toolName +
#if defined(_WIN32)
            ".exe"
#else
            ""
#endif
        );
        if (fs::exists(p)) return p.string();
    }

    // 3. Multi-drive scanning on Windows
#if defined(_WIN32)
    std::vector<std::string> drives = {"C:", "D:", "E:", "F:", "G:"};
    if (!exeDir.empty() && exeDir.has_root_name()) {
        std::string exeDrive = exeDir.root_name().string();
        bool foundDrive = false;
        for (const auto& d : drives) {
            if (_stricmp(d.c_str(), exeDrive.c_str()) == 0) {
                foundDrive = true;
                break;
            }
        }
        if (!foundDrive) drives.insert(drives.begin(), exeDrive);
    }

    for (const auto& d : drives) {
        fs::path p1 = fs::path(d) / "Program Files" / "LLVM" / "bin" / (toolName + ".exe");
        if (fs::exists(p1)) return p1.string();
        fs::path p2 = fs::path(d) / "Program Files (x86)" / "LLVM" / "bin" / (toolName + ".exe");
        if (fs::exists(p2)) return p2.string();
        fs::path p3 = fs::path(d) / "msys64" / "ucrt64" / "bin" / (toolName + ".exe");
        if (fs::exists(p3)) return p3.string();
        fs::path p4 = fs::path(d) / "msys64" / "clang64" / "bin" / (toolName + ".exe");
        if (fs::exists(p4)) return p4.string();
        fs::path p5 = fs::path(d) / "msys64" / "mingw64" / "bin" / (toolName + ".exe");
        if (fs::exists(p5)) return p5.string();
        fs::path p6 = fs::path(d) / "mingw64" / "bin" / (toolName + ".exe");
        if (fs::exists(p6)) return p6.string();
        fs::path p7 = fs::path(d) / "LLVM" / "bin" / (toolName + ".exe");
        if (fs::exists(p7)) return p7.string();
    }
#endif

    // 4. PATH search
    auto inPath = util::findInPath(toolName);
    if (!inPath.empty() && fs::exists(inPath)) {
        std::string s = inPath.string();
        std::string lower = s;
        for (auto& c : lower) c = std::tolower(static_cast<unsigned char>(c));
        if (lower.find("g++") == std::string::npos && lower.find("gcc") == std::string::npos) {
            return s;
        }
    }

    return "";
}

bool ObjectEmitter::emitObjectFile(const ir::IRModule& module, const std::string& outputObjPath) {
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

    size_t secondLinePos = body.find("source_filename = ");
    if (secondLinePos != std::string::npos) {
        size_t nextPos = body.find('\n', secondLinePos);
        if (nextPos != std::string::npos) {
            body = body.substr(nextPos + 1);
        }
    }
    ss << body;

    return emitObjectFromIR(ss.str(), outputObjPath);
}

bool ObjectEmitter::emitObjectFromIR(const std::string& llvmIR, const std::string& outputObjPath) {
    // Write temporary IR file into system temp directory to prevent write conflicts
    std::error_code ec;
    fs::path tempDir = fs::temp_directory_path(ec);
    if (ec || !fs::exists(tempDir)) {
        tempDir = ".";
    }
    static std::atomic<uint64_t> s_objNonce{0};
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    // Phase 4 slice D: include the pid; the timestamp + per-process nonce
    // alone could collide across two processes started at the same time.
    std::string tempIRPath = (tempDir / ("sp_ir_" + std::to_string(currentPid()) + "_" +
                                         std::to_string(now) + "_" + std::to_string(++s_objNonce) + ".tmp.ll")).string();
    {
        std::ofstream irFile(tempIRPath, std::ios::out | std::ios::trunc);
        if (!irFile.is_open()) {
            m_lastError = "Failed to create temporary IR file: " + tempIRPath;
            return false;
        }
        irFile << llvmIR;
    }

    std::string clangTool = findLLVMTool("clang", m_options.llvmToolsPath);
    if (clangTool.empty() || !fs::exists(clangTool)) {
        auto clangCompiler = util::findClangCompiler();
        if (!clangCompiler.empty() && fs::exists(clangCompiler)) {
            clangTool = clangCompiler.string();
        }
    }
    if (clangTool.empty() || !fs::exists(clangTool)) {
        std::error_code rmEc;
        fs::remove(tempIRPath, rmEc);
        m_lastError = "No LLVM Clang compiler found on this system. Run 'spc setup' to auto-provision a portable toolchain.";
        return false;
    }

    std::string lowerTool = clangTool;
    for (auto& c : lowerTool) c = std::tolower(static_cast<unsigned char>(c));
    if (lowerTool.find("g++") != std::string::npos || lowerTool.find("gcc") != std::string::npos) {
        std::error_code rmEc;
        fs::remove(tempIRPath, rmEc);
        m_lastError = "Unsupported backend compiler '" + clangTool + "'. SP Lang requires LLVM Clang to compile LLVM IR and handle target triples. Please run 'spc setup'.";
        return false;
    }

    std::ostringstream cmd;
    cmd << "\"" << clangTool << "\" -c ";
    if (!m_options.targetTriple.empty()) {
        cmd << "--target=" << m_options.targetTriple << " ";
    }
    if (!m_options.optLevel.empty()) {
        cmd << m_options.optLevel << " ";
    }
    if (m_options.pic) {
        cmd << "-fPIC ";
    }
    if (m_options.debugSymbols) {
        cmd << "-g ";
    }
    cmd << "-o \"" << outputObjPath << "\" \"" << tempIRPath << "\"";

#if defined(_WIN32)
    std::string fullCmd = "\"" + cmd.str() + "\"";
    int exitCode = std::system(fullCmd.c_str());
#else
    int exitCode = std::system(cmd.str().c_str());
#endif

    // Clean up temporary IR file
    std::error_code rmEc;
    fs::remove(tempIRPath, rmEc);

    if (exitCode != 0) {
        m_lastError = "Object emission failed with exit code: " + std::to_string(exitCode);
        return false;
    }

    if (!fs::exists(outputObjPath)) {
        m_lastError = "Expected output object file was not generated: " + outputObjPath;
        return false;
    }

    return true;
}

bool ObjectEmitter::emitObjectToMemory(const ir::IRModule& module, std::vector<uint8_t>& outBuffer) {
    // Phase 4 slice D: pid + nonce instead of unseeded std::rand(), which
    // produced identical names in parallel processes.
    static std::atomic<uint64_t> s_memNonce{0};
    fs::path tempObj = fs::temp_directory_path() / ("sp_mem_obj_" + std::to_string(currentPid()) + "_" +
                                                    std::to_string(++s_memNonce) + ".o");
    bool ok = emitObjectFile(module, tempObj.string());
    if (!ok) return false;

    std::ifstream file(tempObj, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        m_lastError = "Failed to read generated object file into memory";
        fs::remove(tempObj);
        return false;
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    outBuffer.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(outBuffer.data()), size);
    file.close();

    std::error_code ec;
    fs::remove(tempObj, ec);
    return true;
}

bool ObjectEmitter::emitObjectFromIRToMemory(const std::string& llvmIR, std::vector<uint8_t>& outBuffer) {
    // Phase 4 slice D: pid + nonce instead of unseeded std::rand() (see above).
    static std::atomic<uint64_t> s_memNonce{0};
    fs::path tempObj = fs::temp_directory_path() / ("sp_mem_obj_" + std::to_string(currentPid()) + "_" +
                                                    std::to_string(++s_memNonce) + ".o");
    bool ok = emitObjectFromIR(llvmIR, tempObj.string());
    if (!ok) return false;

    std::ifstream file(tempObj, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        m_lastError = "Failed to read generated object file into memory";
        fs::remove(tempObj);
        return false;
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    outBuffer.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(outBuffer.data()), size);
    file.close();

    std::error_code ec;
    fs::remove(tempObj, ec);
    return true;
}

} // namespace sp::codegen
