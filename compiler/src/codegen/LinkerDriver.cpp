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
    return "";
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

    // Precompiled runtime library verification
    std::string rtLib = m_options.runtimeLibPath;
    if (rtLib.empty()) {
        rtLib = findPrecompiledRuntimeLib();
    }
    if (rtLib.empty() || !fs::exists(rtLib)) {
        m_lastError = "Precompiled runtime library 'libsp_runtime.a' was not found on this system. "
                      "Run 'spc setup' or check toolchain health with 'spc doctor'.";
        return false;
    }

    std::string linkerName = fs::path(linker).filename().string();
    for (auto& c : linkerName) c = std::tolower(static_cast<unsigned char>(c));
    bool isMsvc = (linkerName.find("lld-link") != std::string::npos || linkerName == "link.exe");
    bool isClang = (linkerName.find("clang") != std::string::npos);

    std::ostringstream cmd;
    cmd << "\"" << linker << "\" ";

    if (isMsvc) {
        // Output binary path in MSVC syntax
        cmd << "/out:\"" << outputBinaryPath << "\" ";
        cmd << "/nologo /subsystem:console ";

        // Input object files
        for (const auto& obj : objectFiles) {
            cmd << "\"" << obj << "\" ";
        }

        // Precompiled runtime library
        cmd << "\"" << rtLib << "\" ";

        // OpenSSL libraries in MSVC syntax
        auto sslInfo = util::findOpenSsl(m_options.librarySearchPaths);
        if (sslInfo.found) {
            cmd << "/libpath:\"" << sslInfo.libDir.string() << "\" ";
            if (!sslInfo.sslLib.empty() && !sslInfo.cryptoLib.empty()) {
                cmd << "\"" << sslInfo.sslLib.string() << "\" \"" << sslInfo.cryptoLib.string() << "\" ";
            } else {
                cmd << "libssl.lib libcrypto.lib ";
            }
        }

        // Search paths
        for (const auto& spath : m_options.librarySearchPaths) {
            cmd << "/libpath:\"" << spath << "\" ";
        }

        // Standard Windows system libraries
#if defined(_WIN32)
        cmd << "ws2_32.lib crypt32.lib advapi32.lib user32.lib shell32.lib ";
#endif

        // Additional libraries
        for (const auto& lib : m_options.additionalLibraries) {
            if (lib.ends_with(".lib") || lib.ends_with(".a")) {
                cmd << "\"" << lib << "\" ";
            } else {
                cmd << lib << ".lib ";
            }
        }
    } else {
        // GNU / Clang syntax
        if (isClang && !m_options.targetTriple.empty()) {
            cmd << "--target=" << m_options.targetTriple << " ";
        }

        // Input object files
        for (const auto& obj : objectFiles) {
            cmd << "\"" << obj << "\" ";
        }

        // Precompiled runtime library
        cmd << "\"" << rtLib << "\" ";

        if (isClang) {
            cmd << "-pthread ";
        }

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
        cmd << "-lws2_32 -lcrypt32 -lbcrypt -lsecur32 -ladvapi32 -luser32 -lshell32 ";
#endif

        for (const auto& spath : m_options.librarySearchPaths) {
            cmd << "-L\"" << spath << "\" ";
        }

        for (const auto& lib : m_options.additionalLibraries) {
            cmd << "-l" << lib << " ";
        }

        if (m_options.stripSymbols) {
            cmd << "-s ";
        }

        if (m_options.isStatic) {
            cmd << "-static ";
        }

        cmd << "-o \"" << outputBinaryPath << "\"";
    }

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
