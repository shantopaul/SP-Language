#include "sp/codegen/ObjectEmitter.h"
#include "sp/ir/IRPrinter.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace sp::codegen {

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

    // 1. Check environment variables
    std::string envVarName = "SP_" + toolName;
    for (auto& c : envVarName) c = static_cast<char>(std::toupper(c));
    const char* envTool = std::getenv(envVarName.c_str());
    if (envTool && fs::exists(envTool)) return envTool;

    if (toolName == "clang" || toolName == "clang++") {
        const char* cEnv = std::getenv("CLANG");
        if (cEnv && fs::exists(cEnv)) return cEnv;
    }

    // 2. Known local toolchain directories
    const std::vector<std::string> knownDirs = {
        "K:/AndroidDev/tools/sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin",
        "C:/msys64/ucrt64/bin",
        "C:/msys64/mingw64/bin",
        "C:/Program Files/LLVM/bin",
        "C:/Program Files (x86)/LLVM/bin"
    };

    for (const auto& dir : knownDirs) {
        fs::path p = fs::path(dir) / (toolName +
#if defined(_WIN32)
            ".exe"
#else
            ""
#endif
        );
        if (fs::exists(p)) return p.string();
    }

    // 3. PATH fallback
#if defined(_WIN32)
    return toolName + ".exe";
#else
    return toolName;
#endif
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
    // Write temporary IR file next to target object
    std::string tempIRPath = outputObjPath + ".tmp.ll";
    {
        std::ofstream irFile(tempIRPath, std::ios::out | std::ios::trunc);
        if (!irFile.is_open()) {
            m_lastError = "Failed to create temporary IR file: " + tempIRPath;
            return false;
        }
        irFile << llvmIR;
    }

    std::string clangTool = findLLVMTool("clang", m_options.llvmToolsPath);

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
    std::error_code ec;
    fs::remove(tempIRPath, ec);

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
    fs::path tempObj = fs::temp_directory_path() / ("sp_mem_obj_" + std::to_string(std::rand()) + ".o");
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
    fs::path tempObj = fs::temp_directory_path() / ("sp_mem_obj_" + std::to_string(std::rand()) + ".o");
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
