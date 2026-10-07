#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sp::util {

/// Returns the absolute path of the currently running executable binary (e.g., C:\SP-Language\bin\spc.exe)
std::filesystem::path getExecutablePath();

/// Returns the directory containing the currently running executable binary (e.g., C:\SP-Language\bin)
std::filesystem::path getExecutableDir();

/// Returns the resolved root SP installation directory (SP_HOME).
/// Resolution order:
/// 1. SP_HOME environment variable (if directory exists)
/// 2. Parent directory of running executable (<exeDir>/..) if stdlib, lib, or include exists there
/// 3. %USERPROFILE%/.sp (or ~/.sp on Linux/macOS)
/// 4. %LOCALAPPDATA%/SP-Language
/// 5. Compile-time SP_PROJECT_ROOT (if present and exists)
std::filesystem::path getSPHome();

/// Resolves standard library root directory (containing std.io, std.fs, etc.)
std::filesystem::path getStdlibDir();

/// Resolves the precompiled runtime static library (libsp_runtime.a)
std::filesystem::path getRuntimeLibPath(const std::string& customPath = "");

/// Resolves the runtime include directory (headers like Runtime.h)
std::filesystem::path getRuntimeIncludePath(const std::string& customPath = "");

/// Resolves the runtime source file (Runtime.cpp), if present
std::filesystem::path getRuntimeSourcePath(const std::string& customPath = "");

/// Searches PATH environment variable for a named binary (e.g., "clang++" -> "C:\...\clang++.exe")
std::filesystem::path findInPath(const std::string& binaryName);

/// Resolves the backend C/LLVM compiler (clang++)
std::filesystem::path findClangCompiler(const std::string& customPath = "");

/// Resolves the backend linker (lld-link, ld.lld, or clang++)
std::filesystem::path findLinker(const std::string& targetTriple = "", const std::string& customPath = "");

/// Returns true if a functional backend C/LLVM compiler or linker was detected
bool hasBackendToolchain();

/// Returns a human-readable diagnosis of toolchain and library health
struct ToolchainHealth {
    std::filesystem::path spcPath;
    std::filesystem::path spHome;
    std::filesystem::path stdlibDir;
    std::filesystem::path runtimeLib;
    std::filesystem::path runtimeInc;
    std::filesystem::path clangPath;
    std::filesystem::path linkerPath;
    std::string targetTriple;
    bool hasStdlib{false};
    bool hasRuntimeLib{false};
    bool hasClang{false};
    bool hasLinker{false};
    bool isOperational{false};
};

ToolchainHealth inspectToolchainHealth();

} // namespace sp::util
