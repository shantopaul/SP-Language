#include "sp/codegen/LinkerDriver.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace sp::codegen {

LinkerDriver::LinkerDriver(LinkerDriverOptions options)
    : m_options(std::move(options)) {
    if (m_options.linkerPath.empty()) {
        m_options.linkerPath = findLinker(m_options.targetTriple);
    }
    if (m_options.runtimeLibPath.empty()) {
        m_options.runtimeLibPath = findPrecompiledRuntimeLib();
    }
}

std::string LinkerDriver::findPrecompiledRuntimeLib(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        return fs::absolute(customPath).string();
    }

    const char* envLib = std::getenv("SP_RUNTIME_LIB");
    if (envLib && fs::exists(envLib)) {
        return fs::absolute(envLib).string();
    }

#ifdef SP_PROJECT_ROOT
    fs::path projLib = fs::path(SP_PROJECT_ROOT) / "build" / "lib" / "libsp_runtime.a";
    if (fs::exists(projLib)) {
        return fs::absolute(projLib).string();
    }
    fs::path projLib2 = fs::path(SP_PROJECT_ROOT) / "lib" / "libsp_runtime.a";
    if (fs::exists(projLib2)) {
        return fs::absolute(projLib2).string();
    }
#endif

    const char* envHome = std::getenv("SP_HOME");
    if (envHome && fs::exists(envHome)) {
        fs::path p1 = fs::path(envHome) / "lib" / "libsp_runtime.a";
        if (fs::exists(p1)) return fs::absolute(p1).string();
    }

    const std::vector<std::string> candidates = {
        "build/lib/libsp_runtime.a",
        "../build/lib/libsp_runtime.a",
        "../../build/lib/libsp_runtime.a",
        "lib/libsp_runtime.a",
        "../lib/libsp_runtime.a"
    };

    for (const auto& c : candidates) {
        if (fs::exists(c)) {
            return fs::absolute(c).string();
        }
    }

    return "libsp_runtime.a";
}

std::string LinkerDriver::findLinker(const std::string& targetTriple, const std::string& customPath) {
    (void)targetTriple;
    if (!customPath.empty()) {
        if (fs::exists(customPath)) return customPath;
    }

    const char* envLinker = std::getenv("SP_LINKER");
    if (envLinker && fs::exists(envLinker)) {
        return envLinker;
    }

    const char* envClang = std::getenv("SP_CLANG");
    if (envClang && fs::exists(envClang)) {
        return envClang;
    }

    const char* envClang2 = std::getenv("CLANG");
    if (envClang2 && fs::exists(envClang2)) {
        return envClang2;
    }

    // Known toolchain directories
    const std::vector<std::string> knownPaths = {
        "K:/AndroidDev/tools/sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe",
        "K:/AndroidDev/tools/sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/ld.lld.exe",
        "C:/msys64/ucrt64/bin/clang++.exe",
        "C:/msys64/mingw64/bin/clang++.exe",
        "C:/Program Files/LLVM/bin/clang++.exe",
        "C:/Program Files/LLVM/bin/lld-link.exe"
    };

    for (const auto& path : knownPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }

#if defined(_WIN32)
    return "clang++.exe";
#else
    return "clang++";
#endif
}

bool LinkerDriver::linkSingle(const std::string& objectFile, const std::string& outputBinaryPath) {
    return link({objectFile}, outputBinaryPath);
}

bool LinkerDriver::link(const std::vector<std::string>& objectFiles, const std::string& outputBinaryPath) {
    if (objectFiles.empty()) {
        m_lastError = "No input object files provided for linking";
        return false;
    }

    std::string linker = m_options.linkerPath;
    if (linker.empty()) {
        linker = findLinker(m_options.targetTriple);
    }

    std::ostringstream cmd;
    cmd << "\"" << linker << "\" ";

    // Pass target triple if available
    if (!m_options.targetTriple.empty()) {
        cmd << "--target=" << m_options.targetTriple << " ";
    }

    // Input object files
    for (const auto& obj : objectFiles) {
        cmd << "\"" << obj << "\" ";
    }

    // Precompiled runtime library
    std::string rtLib = m_options.runtimeLibPath;
    if (rtLib.empty()) {
        rtLib = findPrecompiledRuntimeLib();
    }
    if (fs::exists(rtLib)) {
        cmd << "\"" << rtLib << "\" ";
    }

    // Search paths
    for (const auto& spath : m_options.librarySearchPaths) {
        cmd << "-L\"" << spath << "\" ";
    }

    // Additional libraries
    for (const auto& lib : m_options.additionalLibraries) {
        cmd << "-l" << lib << " ";
    }

    // Strip symbols if requested
    if (m_options.stripSymbols) {
        cmd << "-s ";
    }

    // Output binary path
    cmd << "-o \"" << outputBinaryPath << "\"";

#if defined(_WIN32)
    std::string fullCmd = "\"" + cmd.str() + "\"";
    int exitCode = std::system(fullCmd.c_str());
#else
    int exitCode = std::system(cmd.str().c_str());
#endif

    if (exitCode != 0) {
        m_lastError = "Linker invocation failed with exit code: " + std::to_string(exitCode);
        return false;
    }

    if (!fs::exists(outputBinaryPath)) {
        m_lastError = "Expected output binary was not created: " + outputBinaryPath;
        return false;
    }

    return true;
}

} // namespace sp::codegen
