#include "sp/util/PathUtils.h"
#include <cstdlib>
#include <sstream>
#include <vector>
#include <iostream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

namespace sp::util {

fs::path getExecutablePath() {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0) return "";
    while (size == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    buffer.resize(size);
    return fs::path(buffer);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        std::error_code ec;
        auto p = fs::canonical(buffer.c_str(), ec);
        return ec ? fs::path(buffer) : p;
    }
    return "";
#elif defined(__linux__)
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (count != -1) {
        buffer[count] = '\0';
        return fs::path(buffer);
    }
    return "";
#else
    return "";
#endif
}

fs::path getExecutableDir() {
    auto exe = getExecutablePath();
    if (!exe.empty()) {
        return exe.parent_path();
    }
    return fs::current_path();
}

fs::path getSPHome() {
    // 1. Check SP_HOME environment variable
    const char* envHome = std::getenv("SP_HOME");
    if (envHome && *envHome) {
        fs::path p(envHome);
        if (fs::exists(p)) {
            return fs::absolute(p);
        }
    }

    // 2. Relative to running binary: <exeDir>/..
    auto exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        auto parent = exeDir.parent_path();
        if (fs::exists(parent / "stdlib") || fs::exists(parent / "lib") || fs::exists(parent / "include")) {
            return fs::absolute(parent);
        }
    }

    // 3. Check ~/.sp or %USERPROFILE%/.sp
#if defined(_WIN32)
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        fs::path p = fs::path(userProfile) / ".sp";
        if (fs::exists(p)) return fs::absolute(p);
    }
    const char* localApp = std::getenv("LOCALAPPDATA");
    if (localApp && *localApp) {
        fs::path p = fs::path(localApp) / "SP-Language";
        if (fs::exists(p)) return fs::absolute(p);
    }
    if (fs::exists("C:\\SP-Language")) {
        return fs::path("C:\\SP-Language");
    }
#else
    const char* home = std::getenv("HOME");
    if (home && *home) {
        fs::path p = fs::path(home) / ".sp";
        if (fs::exists(p)) return fs::absolute(p);
    }
#endif

    // 4. Compile-time SP_PROJECT_ROOT
#ifdef SP_PROJECT_ROOT
    fs::path proj(SP_PROJECT_ROOT);
    if (fs::exists(proj)) {
        return fs::absolute(proj);
    }
#endif

    return getExecutableDir();
}

fs::path getStdlibDir() {
    auto home = getSPHome();
    if (!home.empty() && fs::exists(home / "stdlib")) {
        return fs::absolute(home / "stdlib");
    }

    auto exeDir = getExecutableDir();
    if (fs::exists(exeDir / "stdlib")) {
        return fs::absolute(exeDir / "stdlib");
    }
    if (fs::exists(exeDir / ".." / "stdlib")) {
        return fs::absolute(exeDir / ".." / "stdlib");
    }

#ifdef SP_PROJECT_ROOT
    fs::path proj(SP_PROJECT_ROOT);
    if (fs::exists(proj / "stdlib")) {
        return fs::absolute(proj / "stdlib");
    }
#endif

    return "";
}

fs::path getRuntimeLibPath(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        return fs::absolute(customPath);
    }

    const char* envLib = std::getenv("SP_RUNTIME_LIB");
    if (envLib && *envLib) {
        fs::path p(envLib);
        if (fs::exists(p)) return fs::absolute(p);
    }

    auto home = getSPHome();
    if (!home.empty()) {
        if (fs::exists(home / "lib" / "libsp_runtime.a")) {
            return fs::absolute(home / "lib" / "libsp_runtime.a");
        }
        if (fs::exists(home / "build" / "lib" / "libsp_runtime.a")) {
            return fs::absolute(home / "build" / "lib" / "libsp_runtime.a");
        }
    }

    auto exeDir = getExecutableDir();
    if (fs::exists(exeDir / ".." / "lib" / "libsp_runtime.a")) {
        return fs::absolute(exeDir / ".." / "lib" / "libsp_runtime.a");
    }
    if (fs::exists(exeDir / "lib" / "libsp_runtime.a")) {
        return fs::absolute(exeDir / "lib" / "libsp_runtime.a");
    }

#ifdef SP_PROJECT_ROOT
    fs::path proj(SP_PROJECT_ROOT);
    if (fs::exists(proj / "build" / "lib" / "libsp_runtime.a")) {
        return fs::absolute(proj / "build" / "lib" / "libsp_runtime.a");
    }
    if (fs::exists(proj / "lib" / "libsp_runtime.a")) {
        return fs::absolute(proj / "lib" / "libsp_runtime.a");
    }
#endif

    const std::vector<std::string> candidates = {
        "build/lib/libsp_runtime.a",
        "../build/lib/libsp_runtime.a",
        "../../build/lib/libsp_runtime.a",
        "lib/libsp_runtime.a",
        "../lib/libsp_runtime.a"
    };

    for (const auto& c : candidates) {
        if (fs::exists(c)) {
            return fs::absolute(c);
        }
    }

    return "";
}

fs::path getRuntimeIncludePath(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        return fs::absolute(customPath);
    }

    const char* envInc = std::getenv("SP_RUNTIME_INC");
    if (envInc && *envInc) {
        fs::path p(envInc);
        if (fs::exists(p)) return fs::absolute(p);
    }

    auto home = getSPHome();
    if (!home.empty()) {
        if (fs::exists(home / "include")) return fs::absolute(home / "include");
        if (fs::exists(home / "runtime" / "include")) return fs::absolute(home / "runtime" / "include");
    }

    auto exeDir = getExecutableDir();
    if (fs::exists(exeDir / ".." / "include")) return fs::absolute(exeDir / ".." / "include");
    if (fs::exists(exeDir / "include")) return fs::absolute(exeDir / "include");

#ifdef SP_PROJECT_ROOT
    fs::path proj(SP_PROJECT_ROOT);
    if (fs::exists(proj / "runtime" / "include")) {
        return fs::absolute(proj / "runtime" / "include");
    }
#endif

    const std::vector<std::string> candidates = {
        "runtime/include",
        "../runtime/include",
        "../../runtime/include",
        "include",
        "../include"
    };

    for (const auto& c : candidates) {
        if (fs::exists(c)) {
            return fs::absolute(c);
        }
    }

    return "";
}

fs::path getRuntimeSourcePath(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        return fs::absolute(customPath);
    }

    const char* envSrc = std::getenv("SP_RUNTIME_SRC");
    if (envSrc && *envSrc) {
        fs::path p(envSrc);
        if (fs::exists(p)) return fs::absolute(p);
    }

    auto home = getSPHome();
    if (!home.empty()) {
        if (fs::exists(home / "share" / "sp" / "runtime" / "Runtime.cpp")) {
            return fs::absolute(home / "share" / "sp" / "runtime" / "Runtime.cpp");
        }
        if (fs::exists(home / "runtime" / "src" / "Runtime.cpp")) {
            return fs::absolute(home / "runtime" / "src" / "Runtime.cpp");
        }
    }

#ifdef SP_PROJECT_ROOT
    fs::path proj(SP_PROJECT_ROOT);
    if (fs::exists(proj / "runtime" / "src" / "Runtime.cpp")) {
        return fs::absolute(proj / "runtime" / "src" / "Runtime.cpp");
    }
#endif

    const std::vector<std::string> candidates = {
        "runtime/src/Runtime.cpp",
        "../runtime/src/Runtime.cpp",
        "../../runtime/src/Runtime.cpp"
    };

    for (const auto& c : candidates) {
        if (fs::exists(c)) {
            return fs::absolute(c);
        }
    }

    return "";
}

fs::path findInPath(const std::string& binaryName) {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv || !*pathEnv) return "";

    std::stringstream ss(pathEnv);
    std::string item;
#if defined(_WIN32)
    const char delimiter = ';';
#else
    const char delimiter = ':';
#endif

    while (std::getline(ss, item, delimiter)) {
        if (item.empty()) continue;
        fs::path candidate = fs::path(item) / binaryName;
        if (fs::exists(candidate)) {
            return fs::absolute(candidate);
        }
#if defined(_WIN32)
        if (!candidate.has_extension()) {
            auto withExe = candidate;
            withExe.replace_extension(".exe");
            if (fs::exists(withExe)) {
                return fs::absolute(withExe);
            }
        }
#endif
    }
    return "";
}

fs::path findClangCompiler(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        return fs::absolute(customPath);
    }

    const char* envClang = std::getenv("SP_CLANG");
    if (envClang && *envClang && fs::exists(envClang)) {
        return fs::absolute(envClang);
    }

    const char* envClang2 = std::getenv("CLANG");
    if (envClang2 && *envClang2 && fs::exists(envClang2)) {
        return fs::absolute(envClang2);
    }

    // 1. Check bundled toolchain in SP_HOME/toolchain/bin
    auto home = getSPHome();
    if (!home.empty()) {
#if defined(_WIN32)
        if (fs::exists(home / "toolchain" / "bin" / "clang++.exe")) {
            return fs::absolute(home / "toolchain" / "bin" / "clang++.exe");
        }
        if (fs::exists(home / "toolchain" / "bin" / "clang.exe")) {
            return fs::absolute(home / "toolchain" / "bin" / "clang.exe");
        }
#else
        if (fs::exists(home / "toolchain" / "bin" / "clang++")) {
            return fs::absolute(home / "toolchain" / "bin" / "clang++");
        }
#endif
    }

    auto exeDir = getExecutableDir();
#if defined(_WIN32)
    if (fs::exists(exeDir / ".." / "toolchain" / "bin" / "clang++.exe")) {
        return fs::absolute(exeDir / ".." / "toolchain" / "bin" / "clang++.exe");
    }
    if (fs::exists(exeDir / "toolchain" / "bin" / "clang++.exe")) {
        return fs::absolute(exeDir / "toolchain" / "bin" / "clang++.exe");
    }
#else
    if (fs::exists(exeDir / ".." / "toolchain" / "bin" / "clang++")) {
        return fs::absolute(exeDir / ".." / "toolchain" / "bin" / "clang++");
    }
#endif

    // 2. Check standard system installation paths on Windows
#if defined(_WIN32)
    const std::vector<std::string> knownPaths = {
        "C:/Program Files/LLVM/bin/clang++.exe",
        "C:/Program Files/LLVM/bin/clang.exe",
        "C:/Program Files (x86)/LLVM/bin/clang++.exe",
        "C:/msys64/ucrt64/bin/clang++.exe",
        "C:/msys64/mingw64/bin/clang++.exe",
        "C:/msys64/clang64/bin/clang++.exe",
        "C:/mingw64/bin/clang++.exe",
        "C:/w64devkit/bin/g++.exe"
    };
    for (const auto& path : knownPaths) {
        if (fs::exists(path)) {
            return fs::absolute(path);
        }
    }

    // 3. Check PATH
    auto inPath = findInPath("clang++");
    if (!inPath.empty()) return inPath;

    auto inPathClang = findInPath("clang");
    if (!inPathClang.empty()) return inPathClang;

    auto inPathGpp = findInPath("g++");
    if (!inPathGpp.empty()) return inPathGpp;

    return "clang++.exe";
#else
    auto inPath = findInPath("clang++");
    if (!inPath.empty()) return inPath;

    auto inPathClang = findInPath("clang");
    if (!inPathClang.empty()) return inPathClang;

    auto inPathGpp = findInPath("g++");
    if (!inPathGpp.empty()) return inPathGpp;

    return "clang++";
#endif
}

fs::path findLinker(const std::string& targetTriple, const std::string& customPath) {
    (void)targetTriple;
    if (!customPath.empty() && fs::exists(customPath)) {
        return fs::absolute(customPath);
    }

    const char* envLinker = std::getenv("SP_LINKER");
    if (envLinker && *envLinker && fs::exists(envLinker)) {
        return fs::absolute(envLinker);
    }

    // 1. Check bundled toolchain in SP_HOME/toolchain/bin
    auto home = getSPHome();
    if (!home.empty()) {
#if defined(_WIN32)
        if (fs::exists(home / "toolchain" / "bin" / "lld-link.exe")) {
            return fs::absolute(home / "toolchain" / "bin" / "lld-link.exe");
        }
        if (fs::exists(home / "toolchain" / "bin" / "ld.lld.exe")) {
            return fs::absolute(home / "toolchain" / "bin" / "ld.lld.exe");
        }
        if (fs::exists(home / "toolchain" / "bin" / "clang++.exe")) {
            return fs::absolute(home / "toolchain" / "bin" / "clang++.exe");
        }
#else
        if (fs::exists(home / "toolchain" / "bin" / "ld.lld")) {
            return fs::absolute(home / "toolchain" / "bin" / "ld.lld");
        }
        if (fs::exists(home / "toolchain" / "bin" / "clang++")) {
            return fs::absolute(home / "toolchain" / "bin" / "clang++");
        }
#endif
    }

    // 2. Check standard system installations
#if defined(_WIN32)
    const std::vector<std::string> knownLinkers = {
        "C:/Program Files/LLVM/bin/lld-link.exe",
        "C:/Program Files/LLVM/bin/clang++.exe",
        "C:/msys64/ucrt64/bin/clang++.exe",
        "C:/msys64/mingw64/bin/clang++.exe",
        "C:/msys64/ucrt64/bin/ld.lld.exe",
        "C:/msys64/mingw64/bin/ld.lld.exe"
    };
    for (const auto& path : knownLinkers) {
        if (fs::exists(path)) {
            return fs::absolute(path);
        }
    }

    auto inPathLld = findInPath("lld-link");
    if (!inPathLld.empty()) return inPathLld;

    auto inPathLd = findInPath("ld.lld");
    if (!inPathLd.empty()) return inPathLd;

    return findClangCompiler();
#else
    auto inPathLd = findInPath("ld.lld");
    if (!inPathLd.empty()) return inPathLd;

    return findClangCompiler();
#endif
}

bool hasBackendToolchain() {
    auto clang = findClangCompiler();
    if (!clang.empty() && fs::exists(clang)) {
        return true;
    }
    auto linker = findLinker();
    if (!linker.empty() && fs::exists(linker)) {
        return true;
    }
    return false;
}

ToolchainHealth inspectToolchainHealth() {
    ToolchainHealth h;
    h.spcPath = getExecutablePath();
    h.spHome = getSPHome();
    h.stdlibDir = getStdlibDir();
    h.runtimeLib = getRuntimeLibPath();
    h.runtimeInc = getRuntimeIncludePath();
    h.clangPath = findClangCompiler();
    h.linkerPath = findLinker();

    h.hasStdlib = !h.stdlibDir.empty() && fs::exists(h.stdlibDir);
    h.hasRuntimeLib = !h.runtimeLib.empty() && fs::exists(h.runtimeLib);
    h.hasClang = !h.clangPath.empty() && fs::exists(h.clangPath);
    h.hasLinker = !h.linkerPath.empty() && fs::exists(h.linkerPath);

#if defined(_WIN32)
    h.targetTriple = "x86_64-pc-windows-msvc";
#elif defined(__APPLE__)
    h.targetTriple = "arm64-apple-darwin";
#else
    h.targetTriple = "x86_64-unknown-linux-gnu";
#endif

    h.isOperational = h.hasStdlib && (h.hasRuntimeLib || !h.runtimeInc.empty()) && (h.hasClang || h.hasLinker);
    return h;
}

} // namespace sp::util
