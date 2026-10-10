// ==============================================================================
// SP Programming Language - Complete System Uninstaller (uninstall.exe)
// ==============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#endif

namespace fs = std::filesystem;

#if defined(_WIN32)
static void removeRegistryEnvironmentValue(const std::string& name) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, name.c_str());
        RegCloseKey(hKey);
    }
}

static std::string getRegistryUserPath() {
    HKEY hKey;
    char buffer[32768];
    DWORD bufferSize = sizeof(buffer);
    DWORD type = 0;
    std::string result;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "Path", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS) {
            result = std::string(buffer, bufferSize > 0 && buffer[bufferSize - 1] == '\0' ? bufferSize - 1 : bufferSize);
        }
        RegCloseKey(hKey);
    }
    return result;
}

static void setRegistryUserPath(const std::string& newPath) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, 
                       reinterpret_cast<const BYTE*>(newPath.c_str()), 
                       static_cast<DWORD>(newPath.length() + 1));
        RegCloseKey(hKey);
    }
}

static void cleanUserPath(const std::vector<std::string>& patternsToRemove) {
    std::string currentPath = getRegistryUserPath();
    if (currentPath.empty()) return;

    std::vector<std::string> entries;
    size_t start = 0;
    while (start < currentPath.size()) {
        size_t end = currentPath.find(';', start);
        if (end == std::string::npos) end = currentPath.size();
        std::string part = currentPath.substr(start, end - start);
        if (!part.empty()) {
            entries.push_back(part);
        }
        start = end + 1;
    }

    std::vector<std::string> keptEntries;
    bool modified = false;

    for (const auto& entry : entries) {
        std::string lowerEntry = entry;
        std::transform(lowerEntry.begin(), lowerEntry.end(), lowerEntry.begin(), ::tolower);
        
        bool shouldRemove = false;
        for (const auto& pat : patternsToRemove) {
            std::string lowerPat = pat;
            std::transform(lowerPat.begin(), lowerPat.end(), lowerPat.begin(), ::tolower);
            if (lowerEntry.find(lowerPat) != std::string::npos) {
                shouldRemove = true;
                break;
            }
        }

        if (!shouldRemove) {
            keptEntries.push_back(entry);
        } else {
            modified = true;
        }
    }

    if (modified) {
        std::string updatedPath;
        for (size_t i = 0; i < keptEntries.size(); ++i) {
            if (i > 0) updatedPath += ";";
            updatedPath += keptEntries[i];
        }
        setRegistryUserPath(updatedPath);
    }
}

static void broadcastEnvironmentChange() {
    DWORD_PTR result = 0;
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 
                        reinterpret_cast<LPARAM>("Environment"), 
                        SMTO_ABORTIFHUNG, 2000, &result);
}

static void removeUninstallRegistryKey() {
    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SPLang");
}
#endif

int main(int argc, char* argv[]) {
    bool silent = false;
    bool isChildRunner = false;
    fs::path targetDirFromArg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-y" || arg == "--yes" || arg == "-f" || arg == "--force" || arg == "-s" || arg == "--silent") {
            silent = true;
        } else if (arg == "--child-runner" && i + 1 < argc) {
            isChildRunner = true;
            targetDirFromArg = argv[++i];
        }
    }

    // Determine target installation directory
    fs::path installDir = targetDirFromArg;
    if (installDir.empty()) {
        const char* envHome = std::getenv("SP_HOME");
        if (envHome && *envHome && fs::exists(envHome)) {
            installDir = fs::absolute(envHome);
        } else {
#if defined(_WIN32)
            const char* userProfile = std::getenv("USERPROFILE");
            if (userProfile && *userProfile) {
                fs::path defSp = fs::path(userProfile) / ".sp";
                if (fs::exists(defSp)) {
                    installDir = defSp;
                }
            }
#else
            const char* home = std::getenv("HOME");
            if (home && *home) {
                fs::path defSp = fs::path(home) / ".sp";
                if (fs::exists(defSp)) {
                    installDir = defSp;
                }
            }
#endif
        }

        if (installDir.empty() || !fs::exists(installDir)) {
            try {
                fs::path exePath = fs::canonical(argv[0]);
                if (exePath.filename() == "uninstall.exe" || exePath.filename() == "uninstall") {
                    installDir = exePath.parent_path();
                    if (installDir.filename() == "bin") {
                        installDir = installDir.parent_path();
                    }
                }
            } catch (...) {}
        }
    }

    if (installDir.empty() || !fs::exists(installDir)) {
        std::cerr << "[!] Error: Could not locate SP Language installation directory.\n";
        return 1;
    }

    if (!isChildRunner) {
        std::cout << "\n========================================================================\n"
                  << "         SP Programming Language - Complete System Uninstaller\n"
                  << "========================================================================\n\n";
        std::cout << "Target Installation to Remove: " << installDir.string() << "\n\n";

        if (!silent) {
            std::cout << "Are you sure you want to completely uninstall SP Language from this PC? [y/N]: ";
            std::string reply;
            std::getline(std::cin, reply);
            if (reply != "y" && reply != "Y" && reply != "yes" && reply != "YES") {
                std::cout << "\nUninstallation canceled. No files were modified.\n\n";
                return 0;
            }
        }
    }

#if defined(_WIN32)
    // If running from inside the install directory, copy to %TEMP% and delegate.
    // This unlocks uninstall.exe itself so the install directory can be deleted immediately.
    if (!isChildRunner) {
        std::error_code ec;
        fs::path tempExe = fs::temp_directory_path() / "sp_uninstall_runner.exe";
        fs::remove(tempExe, ec);

        try {
            fs::copy_file(argv[0], tempExe, fs::copy_options::overwrite_existing, ec);
        } catch (...) {}

        if (fs::exists(tempExe)) {
            std::string childCmd = "\"" + tempExe.string() + "\" --child-runner \"" + installDir.string() + "\" -y";
            STARTUPINFOA si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi;
            ZeroMemory(&pi, sizeof(pi));
            std::vector<char> cmdBuf(childCmd.begin(), childCmd.end());
            cmdBuf.push_back('\0');

            if (CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return 0;
            }
        }
    }
#endif

    std::cout << "\n[*] Removing SP Programming Language...\n";

#if defined(_WIN32)
    if (isChildRunner) {
        Sleep(1000);
    }

    // 1. Clean Environment Variables
    std::cout << "  -> Removing SP_HOME environment variable...\n";
    removeRegistryEnvironmentValue("SP_HOME");

    std::cout << "  -> Cleaning User PATH...\n";
    std::vector<std::string> patterns = {
        installDir.string(),
        "\\.sp\\bin",
        "\\.sp\\toolchain\\bin",
        "\\.sp",
        "\\SP-Language\\bin",
        "\\SP-Language"
    };
    cleanUserPath(patterns);

    std::cout << "  -> Broadcasting system environment change...\n";
    broadcastEnvironmentChange();

    std::cout << "  -> Unregistering from Windows Installed Apps...\n";
    removeUninstallRegistryKey();
#endif

    // 2. Remove files and entire directory
    std::cout << "  -> Deleting installed files and directory: " << installDir.string() << "...\n";
    std::error_code ec;

    // Retry up to 10 times with small backoff in case file handles take a moment to release
    for (int retry = 0; retry < 10; ++retry) {
        fs::remove_all(installDir, ec);
        if (!fs::exists(installDir, ec)) {
            break;
        }
#if defined(_WIN32)
        Sleep(300);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
#endif
    }

    if (fs::exists(installDir, ec)) {
        // Fallback: force remove via cmd.exe
#if defined(_WIN32)
        std::string fallbackCmd = "cmd.exe /c ping 127.0.0.1 -n 2 > nul & rmdir /s /q \"" + installDir.string() + "\"";
        std::system(fallbackCmd.c_str());
#else
        std::string fallbackCmd = "rm -rf \"" + installDir.string() + "\"";
        std::system(fallbackCmd.c_str());
#endif
    }

    std::cout << "\n========================================================================\n"
              << "       SP Programming Language Successfully Uninstalled!\n"
              << "========================================================================\n"
              << "All compiler binaries, package managers, runtimes, libraries, and\n"
              << "environment variables have been cleanly removed from this computer.\n"
              << "========================================================================\n\n";

#if defined(_WIN32)
    if (isChildRunner) {
        fs::path selfExe = fs::canonical(argv[0], ec);
        if (!selfExe.empty() && fs::exists(selfExe, ec)) {
            std::string delCmd = "cmd.exe /c ping 127.0.0.1 -n 2 > nul & del /f /q \"" + selfExe.string() + "\"";
            STARTUPINFOA si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi;
            ZeroMemory(&pi, sizeof(pi));
            std::vector<char> cmdBuf(delCmd.begin(), delCmd.end());
            cmdBuf.push_back('\0');
            CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
            if (pi.hProcess) CloseHandle(pi.hProcess);
            if (pi.hThread) CloseHandle(pi.hThread);
        }
    }
#endif

    return 0;
}
