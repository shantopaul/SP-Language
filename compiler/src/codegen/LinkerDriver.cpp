#include "sp/codegen/LinkerDriver.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "sp/util/PathUtils.h"

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
    auto p = util::getRuntimeLibPath(customPath);
    if (!p.empty() && fs::exists(p)) {
        return p.string();
    }
    return "libsp_runtime.a";
}

std::string LinkerDriver::findLinker(const std::string& targetTriple, const std::string& customPath) {
    auto p = util::findLinker(targetTriple, customPath);
    if (!p.empty() && fs::exists(p)) {
        return p.string();
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

    // Enable multi-threading runtime libraries
    cmd << "-pthread ";
    // Multi-tier OpenSSL discovery (environment vars, SP_HOME, package managers, system drives)
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
#if defined(_WIN32)
    cmd << "-lws2_32 -lcrypt32 ";
#endif


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

    // Static linkage if requested
    if (m_options.isStatic) {
        cmd << "-static ";
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
