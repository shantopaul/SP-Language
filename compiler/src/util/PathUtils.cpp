#include "sp/util/PathUtils.h"
#include <cstdlib>
#include <sstream>
#include <vector>
#include <iostream>
#include <optional>

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
        for (const auto& folder : {"SP-Language", "SP-Lang", "splang", "Program Files/SP-Language"}) {
            fs::path p = fs::path(d) / folder;
            if (fs::exists(p / "stdlib") || fs::exists(p / "lib") || fs::exists(p / "bin")) {
                return fs::absolute(p);
            }
        }
    }
#else
    const char* home = std::getenv("HOME");
    if (home && *home) {
        fs::path p = fs::path(home) / ".sp";
        if (fs::exists(p)) return fs::absolute(p);
    }
    const std::vector<std::string> unixDirs = {
        "/usr/local/share/sp",
        "/usr/share/sp",
        "/opt/sp",
        "/usr/local"
    };
    for (const auto& u : unixDirs) {
        fs::path p(u);
        if (fs::exists(p / "stdlib")) return fs::absolute(p);
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

#if defined(_WIN32)
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        fs::path p = fs::path(userProfile) / ".sp" / "stdlib";
        if (fs::exists(p)) return fs::absolute(p);
    }
    const char* localApp = std::getenv("LOCALAPPDATA");
    if (localApp && *localApp) {
        fs::path p = fs::path(localApp) / "SP-Language" / "stdlib";
        if (fs::exists(p)) return fs::absolute(p);
    }

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
        for (const auto& folder : {"SP-Language", "SP-Lang", "splang", "Program Files/SP-Language"}) {
            fs::path p = fs::path(d) / folder / "stdlib";
            if (fs::exists(p)) return fs::absolute(p);
        }
    }
#else
    const char* home = std::getenv("HOME");
    if (home && *home) {
        fs::path p = fs::path(home) / ".sp" / "stdlib";
        if (fs::exists(p)) return fs::absolute(p);
    }
    const std::vector<std::string> unixDirs = {
        "/usr/local/share/sp/stdlib",
        "/usr/share/sp/stdlib",
        "/opt/sp/stdlib"
    };
    for (const auto& u : unixDirs) {
        if (fs::exists(u)) return fs::absolute(u);
    }
#endif

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
    if (fs::exists(exeDir / ".." / "build" / "lib" / "libsp_runtime.a")) {
        return fs::absolute(exeDir / ".." / "build" / "lib" / "libsp_runtime.a");
    }
    if (fs::exists(exeDir / "build" / "lib" / "libsp_runtime.a")) {
        return fs::absolute(exeDir / "build" / "lib" / "libsp_runtime.a");
    }

#if defined(_WIN32)
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        fs::path p = fs::path(userProfile) / ".sp" / "lib" / "libsp_runtime.a";
        if (fs::exists(p)) return fs::absolute(p);
    }
    const char* localApp = std::getenv("LOCALAPPDATA");
    if (localApp && *localApp) {
        fs::path p = fs::path(localApp) / "SP-Language" / "lib" / "libsp_runtime.a";
        if (fs::exists(p)) return fs::absolute(p);
    }

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
        for (const auto& folder : {"SP-Language", "SP-Lang", "splang", "Program Files/SP-Language"}) {
            fs::path p1 = fs::path(d) / folder / "lib" / "libsp_runtime.a";
            if (fs::exists(p1)) return fs::absolute(p1);
            fs::path p2 = fs::path(d) / folder / "build" / "lib" / "libsp_runtime.a";
            if (fs::exists(p2)) return fs::absolute(p2);
        }
    }
#else
    const char* homeDir = std::getenv("HOME");
    if (homeDir && *homeDir) {
        fs::path p = fs::path(homeDir) / ".sp" / "lib" / "libsp_runtime.a";
        if (fs::exists(p)) return fs::absolute(p);
    }
    const std::vector<std::string> unixLibDirs = {
        "/usr/local/lib/libsp_runtime.a",
        "/usr/lib/libsp_runtime.a",
        "/opt/sp/lib/libsp_runtime.a"
    };
    for (const auto& u : unixLibDirs) {
        if (fs::exists(u)) return fs::absolute(u);
    }
#endif

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
        std::string s(envClang);
        std::string lower = s;
        for (auto& c : lower) c = std::tolower(static_cast<unsigned char>(c));
        if (lower.find("g++") == std::string::npos && lower.find("gcc") == std::string::npos) {
            return fs::absolute(s);
        }
    }

    const char* envClang2 = std::getenv("CLANG");
    if (envClang2 && *envClang2 && fs::exists(envClang2)) {
        std::string s(envClang2);
        std::string lower = s;
        for (auto& c : lower) c = std::tolower(static_cast<unsigned char>(c));
        if (lower.find("g++") == std::string::npos && lower.find("gcc") == std::string::npos) {
            return fs::absolute(s);
        }
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

    // 2. Multi-drive scan and standard system installation paths on Windows
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
        if (!foundDrive) {
            drives.insert(drives.begin(), exeDrive);
        }
    }

    for (const auto& d : drives) {
        fs::path p1 = fs::path(d) / "Program Files" / "LLVM" / "bin" / "clang++.exe";
        if (fs::exists(p1)) return fs::absolute(p1);
        fs::path p2 = fs::path(d) / "Program Files" / "LLVM" / "bin" / "clang.exe";
        if (fs::exists(p2)) return fs::absolute(p2);
        fs::path p3 = fs::path(d) / "Program Files (x86)" / "LLVM" / "bin" / "clang++.exe";
        if (fs::exists(p3)) return fs::absolute(p3);
        fs::path p4 = fs::path(d) / "msys64" / "ucrt64" / "bin" / "clang++.exe";
        if (fs::exists(p4)) return fs::absolute(p4);
        fs::path p5 = fs::path(d) / "msys64" / "clang64" / "bin" / "clang++.exe";
        if (fs::exists(p5)) return fs::absolute(p5);
        fs::path p6 = fs::path(d) / "msys64" / "mingw64" / "bin" / "clang++.exe";
        if (fs::exists(p6)) return fs::absolute(p6);
        fs::path p7 = fs::path(d) / "mingw64" / "bin" / "clang++.exe";
        if (fs::exists(p7)) return fs::absolute(p7);
        fs::path p8 = fs::path(d) / "LLVM" / "bin" / "clang++.exe";
        if (fs::exists(p8)) return fs::absolute(p8);
        fs::path p9 = fs::path(d) / "LLVM" / "bin" / "clang.exe";
        if (fs::exists(p9)) return fs::absolute(p9);
    }

    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        fs::path prof(userProfile);
        fs::path sc1 = prof / "scoop" / "apps" / "llvm" / "current" / "bin" / "clang++.exe";
        if (fs::exists(sc1)) return fs::absolute(sc1);
        fs::path sc2 = prof / "scoop" / "apps" / "mingw-winlibs-llvm" / "current" / "bin" / "clang++.exe";
        if (fs::exists(sc2)) return fs::absolute(sc2);
        fs::path sc3 = prof / "scoop" / "apps" / "msys2" / "current" / "ucrt64" / "bin" / "clang++.exe";
        if (fs::exists(sc3)) return fs::absolute(sc3);
        fs::path sc4 = prof / "scoop" / "apps" / "msys2" / "current" / "clang64" / "bin" / "clang++.exe";
        if (fs::exists(sc4)) return fs::absolute(sc4);
    }

    // 3. Check PATH strictly for LLVM Clang (never g++)
    auto inPath = findInPath("clang++");
    if (!inPath.empty()) return inPath;

    auto inPathClang = findInPath("clang");
    if (!inPathClang.empty()) return inPathClang;

    return "";
#else
    auto inPath = findInPath("clang++");
    if (!inPath.empty()) return inPath;

    auto inPathClang = findInPath("clang");
    if (!inPathClang.empty()) return inPathClang;

    const std::vector<std::string> unixPaths = {
        "/usr/bin/clang++",
        "/usr/local/bin/clang++",
        "/opt/homebrew/bin/clang++",
        "/usr/bin/clang",
        "/usr/local/bin/clang"
    };
    for (const auto& u : unixPaths) {
        if (fs::exists(u)) return fs::absolute(u);
    }

    return "";
#endif
}

fs::path findLinker(const std::string& targetTriple, const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        return fs::absolute(customPath);
    }

    const char* envLinker = std::getenv("SP_LINKER");
    if (envLinker && *envLinker && fs::exists(envLinker)) {
        return fs::absolute(envLinker);
    }

    bool isMsvc = (targetTriple.find("msvc") != std::string::npos);

    // 1. Check bundled toolchain in SP_HOME/toolchain/bin
    auto home = getSPHome();
    if (!home.empty()) {
#if defined(_WIN32)
        if (isMsvc) {
            if (fs::exists(home / "toolchain" / "bin" / "lld-link.exe")) return fs::absolute(home / "toolchain" / "bin" / "lld-link.exe");
            if (fs::exists(home / "toolchain" / "bin" / "clang++.exe")) return fs::absolute(home / "toolchain" / "bin" / "clang++.exe");
        } else {
            if (fs::exists(home / "toolchain" / "bin" / "clang++.exe")) return fs::absolute(home / "toolchain" / "bin" / "clang++.exe");
            if (fs::exists(home / "toolchain" / "bin" / "ld.lld.exe")) return fs::absolute(home / "toolchain" / "bin" / "ld.lld.exe");
            if (fs::exists(home / "toolchain" / "bin" / "lld-link.exe")) return fs::absolute(home / "toolchain" / "bin" / "lld-link.exe");
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

    auto exeDir = getExecutableDir();
#if defined(_WIN32)
    if (fs::exists(exeDir / ".." / "toolchain" / "bin" / "clang++.exe")) {
        return fs::absolute(exeDir / ".." / "toolchain" / "bin" / "clang++.exe");
    }
    if (fs::exists(exeDir / "toolchain" / "bin" / "clang++.exe")) {
        return fs::absolute(exeDir / "toolchain" / "bin" / "clang++.exe");
    }
    if (fs::exists(exeDir / ".." / "toolchain" / "bin" / "lld-link.exe")) {
        return fs::absolute(exeDir / ".." / "toolchain" / "bin" / "lld-link.exe");
    }
    if (fs::exists(exeDir / "toolchain" / "bin" / "lld-link.exe")) {
        return fs::absolute(exeDir / "toolchain" / "bin" / "lld-link.exe");
    }
#endif

    // 2. Multi-drive scan and standard system installations
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
        if (isMsvc) {
            fs::path p1 = fs::path(d) / "Program Files" / "LLVM" / "bin" / "lld-link.exe";
            if (fs::exists(p1)) return fs::absolute(p1);
        } else {
            fs::path p3 = fs::path(d) / "Program Files" / "LLVM" / "bin" / "clang++.exe";
            if (fs::exists(p3)) return fs::absolute(p3);
            fs::path p2 = fs::path(d) / "Program Files" / "LLVM" / "bin" / "ld.lld.exe";
            if (fs::exists(p2)) return fs::absolute(p2);
        }
        fs::path p4 = fs::path(d) / "msys64" / "ucrt64" / "bin" / "clang++.exe";
        if (fs::exists(p4)) return fs::absolute(p4);
        fs::path p5 = fs::path(d) / "msys64" / "clang64" / "bin" / "clang++.exe";
        if (fs::exists(p5)) return fs::absolute(p5);
        fs::path p6 = fs::path(d) / "msys64" / "mingw64" / "bin" / "clang++.exe";
        if (fs::exists(p6)) return fs::absolute(p6);
        fs::path p7 = fs::path(d) / "msys64" / "ucrt64" / "bin" / "ld.lld.exe";
        if (fs::exists(p7)) return fs::absolute(p7);
        fs::path p8 = fs::path(d) / "msys64" / "clang64" / "bin" / "ld.lld.exe";
        if (fs::exists(p8)) return fs::absolute(p8);
        fs::path p9 = fs::path(d) / "msys64" / "mingw64" / "bin" / "ld.lld.exe";
        if (fs::exists(p9)) return fs::absolute(p9);
        fs::path p10 = fs::path(d) / "Program Files" / "LLVM" / "bin" / "lld-link.exe";
        if (fs::exists(p10)) return fs::absolute(p10);
    }

    if (isMsvc) {
        auto inPathLld = findInPath("lld-link");
        if (!inPathLld.empty()) return inPathLld;
    } else {
        auto inPathClang = findInPath("clang++");
        if (!inPathClang.empty()) return inPathClang;
        auto inPathLd = findInPath("ld.lld");
        if (!inPathLd.empty()) return inPathLd;
        auto inPathLld = findInPath("lld-link");
        if (!inPathLld.empty()) return inPathLld;
    }

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

OpenSslPaths findOpenSsl(const std::vector<std::string>& customSearchPaths) {
    auto checkDir = [](const fs::path& dir) -> std::optional<OpenSslPaths> {
        if (dir.empty()) return std::nullopt;
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return std::nullopt;

        OpenSslPaths paths;
        paths.libDir = fs::absolute(dir);

#if defined(_WIN32)
        // 1. Static archives (libssl.a and libcrypto.a) - HIGHEST PREFERENCE for portable standalone binaries
        fs::path staticSsl = dir / "libssl.a";
        fs::path staticCrypto = dir / "libcrypto.a";
        if (fs::exists(staticSsl, ec) && fs::exists(staticCrypto, ec)) {
            paths.sslLib = fs::absolute(staticSsl);
            paths.cryptoLib = fs::absolute(staticCrypto);
            paths.isStatic = true;
            paths.found = true;
            return paths;
        }

        // 2. MinGW import libraries (libssl.dll.a and libcrypto.dll.a)
        fs::path dllSsl = dir / "libssl.dll.a";
        fs::path dllCrypto = dir / "libcrypto.dll.a";
        if (fs::exists(dllSsl, ec) && fs::exists(dllCrypto, ec)) {
            paths.sslLib = fs::absolute(dllSsl);
            paths.cryptoLib = fs::absolute(dllCrypto);
            paths.isStatic = false;
            paths.found = true;
            return paths;
        }

        // 3. MSVC import libraries (libssl.lib and libcrypto.lib)
        fs::path msvcSsl = dir / "libssl.lib";
        fs::path msvcCrypto = dir / "libcrypto.lib";
        if (fs::exists(msvcSsl, ec) && fs::exists(msvcCrypto, ec)) {
            paths.sslLib = fs::absolute(msvcSsl);
            paths.cryptoLib = fs::absolute(msvcCrypto);
            paths.isStatic = false;
            paths.found = true;
            return paths;
        }
#else
        // 1. Static archives first
        fs::path staticSsl = dir / "libssl.a";
        fs::path staticCrypto = dir / "libcrypto.a";
        if (fs::exists(staticSsl, ec) && fs::exists(staticCrypto, ec)) {
            paths.sslLib = fs::absolute(staticSsl);
            paths.cryptoLib = fs::absolute(staticCrypto);
            paths.isStatic = true;
            paths.found = true;
            return paths;
        }

        // 2. Shared libraries (.so / .dylib)
#if defined(__APPLE__)
        fs::path sharedSsl = dir / "libssl.dylib";
        fs::path sharedCrypto = dir / "libcrypto.dylib";
#else
        fs::path sharedSsl = dir / "libssl.so";
        fs::path sharedCrypto = dir / "libcrypto.so";
#endif
        if (fs::exists(sharedSsl, ec) && fs::exists(sharedCrypto, ec)) {
            paths.sslLib = fs::absolute(sharedSsl);
            paths.cryptoLib = fs::absolute(sharedCrypto);
            paths.isStatic = false;
            paths.found = true;
            return paths;
        }
#endif
        return std::nullopt;
    };

    // Tier 1: User explicitly passed custom paths (e.g., via CLI -L / --lib-path)
    for (const auto& p : customSearchPaths) {
        if (auto res = checkDir(p)) return *res;
    }

    // Tier 2: Standard cross-platform environment variables
    const char* envRoot = std::getenv("OPENSSL_ROOT_DIR");
    if (!envRoot) envRoot = std::getenv("OPENSSL_DIR");
    if (envRoot && *envRoot) {
        fs::path root(envRoot);
        if (auto res = checkDir(root / "lib")) return *res;
        if (auto res = checkDir(root / "lib64")) return *res;
        if (auto res = checkDir(root)) return *res;
    }

    const char* envLib = std::getenv("OPENSSL_LIB_DIR");
    if (envLib && *envLib) {
        if (auto res = checkDir(envLib)) return *res;
    }

    // Check LIBRARY_PATH or LIB env vars
    for (const char* envVar : {"LIBRARY_PATH", "LIB"}) {
        const char* val = std::getenv(envVar);
        if (val && *val) {
            std::stringstream ss(val);
            std::string item;
#if defined(_WIN32)
            const char sep = ';';
#else
            const char sep = ':';
#endif
            while (std::getline(ss, item, sep)) {
                if (!item.empty()) {
                    if (auto res = checkDir(item)) return *res;
                }
            }
        }
    }

    // Tier 3: SP Lang self-contained / portable toolchain paths
    auto spHome = getSPHome();
    if (!spHome.empty()) {
        if (auto res = checkDir(spHome / "lib")) return *res;
        if (auto res = checkDir(spHome / "toolchain" / "lib")) return *res;
        if (auto res = checkDir(spHome / "toolchain" / "x86_64-w64-mingw32" / "lib")) return *res;
    }

    auto exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        if (auto res = checkDir(exeDir / ".." / "lib")) return *res;
        if (auto res = checkDir(exeDir / "lib")) return *res;
        if (auto res = checkDir(exeDir / ".." / "toolchain" / "lib")) return *res;
    }

    // Tier 4: Windows package managers (Scoop, vcpkg, Chocolatey)
#if defined(_WIN32)
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        fs::path prof(userProfile);
        if (auto res = checkDir(prof / "scoop" / "apps" / "openssl" / "current" / "lib")) return *res;
        if (auto res = checkDir(prof / "scoop" / "apps" / "mingw-winlibs" / "current" / "lib")) return *res;
        if (auto res = checkDir(prof / "scoop" / "apps" / "mingw-winlibs-ucrt" / "current" / "lib")) return *res;
        if (auto res = checkDir(prof / "scoop" / "apps" / "msys2" / "current" / "ucrt64" / "lib")) return *res;
        if (auto res = checkDir(prof / "scoop" / "apps" / "msys2" / "current" / "mingw64" / "lib")) return *res;
        if (auto res = checkDir(prof / "vcpkg" / "installed" / "x64-windows" / "lib")) return *res;
        if (auto res = checkDir(prof / "vcpkg" / "installed" / "x64-windows-static" / "lib")) return *res;
    }

    const char* vcpkgRoot = std::getenv("VCPKG_ROOT");
    if (vcpkgRoot && *vcpkgRoot) {
        fs::path vr(vcpkgRoot);
        if (auto res = checkDir(vr / "installed" / "x64-windows" / "lib")) return *res;
        if (auto res = checkDir(vr / "installed" / "x64-windows-static" / "lib")) return *res;
    }

    const char* chocoInstall = std::getenv("ChocolateyInstall");
    if (chocoInstall && *chocoInstall) {
        fs::path ch(chocoInstall);
        if (auto res = checkDir(ch / "lib" / "openssl" / "lib")) return *res;
    }

    // Tier 5: Windows multi-drive detection across known system installations
    std::vector<std::string> drives = {"C:", "D:", "E:", "F:"};
    if (!exeDir.empty() && exeDir.has_root_name()) {
        std::string exeDrive = exeDir.root_name().string();
        bool foundDrive = false;
        for (const auto& d : drives) {
            if (_stricmp(d.c_str(), exeDrive.c_str()) == 0) {
                foundDrive = true;
                break;
            }
        }
        if (!foundDrive) {
            drives.insert(drives.begin(), exeDrive);
        }
    }

    for (const auto& drive : drives) {
        const std::vector<std::string> relDirs = {
            "/msys64/ucrt64/lib",
            "/msys64/mingw64/lib",
            "/msys64/clang64/lib",
            "/Program Files/OpenSSL-Win64/lib",
            "/Program Files/OpenSSL/lib",
            "/OpenSSL-Win64/lib",
            "/OpenSSL/lib",
            "/mingw64/lib"
        };
        for (const auto& rel : relDirs) {
            fs::path p = drive + rel;
            if (auto res = checkDir(p)) return *res;
        }
    }
#endif

    // Tier 6: macOS standard paths (Homebrew, MacPorts)
#if defined(__APPLE__)
    const std::vector<std::string> macDirs = {
        "/opt/homebrew/opt/openssl/lib",
        "/opt/homebrew/opt/openssl@3/lib",
        "/opt/homebrew/opt/openssl@1.1/lib",
        "/usr/local/opt/openssl/lib",
        "/usr/local/opt/openssl@3/lib",
        "/opt/homebrew/lib",
        "/opt/local/lib"
    };
    for (const auto& mdir : macDirs) {
        if (auto res = checkDir(mdir)) return *res;
    }
#endif

    // Tier 7: Linux standard paths
#if !defined(_WIN32) && !defined(__APPLE__)
    const std::vector<std::string> linuxDirs = {
        "/usr/lib/x86_64-linux-gnu",
        "/usr/lib/aarch64-linux-gnu",
        "/usr/lib64",
        "/usr/local/lib64",
        "/usr/local/lib",
        "/usr/lib"
    };
    for (const auto& ldir : linuxDirs) {
        if (auto res = checkDir(ldir)) return *res;
    }
#endif

    OpenSslPaths notFound;
    notFound.found = false;
    return notFound;
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

    auto ssl = findOpenSsl();
    h.hasOpenSsl = ssl.found;
    h.openSslLibDir = ssl.libDir;
    h.isOpenSslStatic = ssl.isStatic;

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

