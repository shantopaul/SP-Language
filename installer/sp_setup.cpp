#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

#define IDR_SP_PAYLOAD 101

namespace {

std::wstring getEnvVar(const std::wstring& name) {
    DWORD size = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
    if (size == 0) return L"";
    std::wstring result(size, L'\0');
    GetEnvironmentVariableW(name.c_str(), &result[0], size);
    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}

bool extractPayloadResource(const std::wstring& outPath) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_SP_PAYLOAD), (LPCWSTR)RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hMem = LoadResource(NULL, hRes);
    if (!hMem) return false;

    DWORD size = SizeofResource(NULL, hRes);
    const void* data = LockResource(hMem);
    if (!data || size == 0) return false;

    HANDLE hFile = CreateFileW(outPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);

    return ok && (written == size);
}

bool executeCommandHidden(const std::wstring& cmd) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(L'\0');

    if (!CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 60000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

void broadcastEnvironmentChange() {
    DWORD_PTR dwResult = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 2000, &dwResult);
}

bool setRegistryValue(HKEY hRoot, const std::wstring& subkey, const std::wstring& valueName, const std::wstring& data) {
    HKEY hKey;
    LONG lRes = RegCreateKeyExW(hRoot, subkey.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (lRes != ERROR_SUCCESS) return false;

    lRes = RegSetValueExW(hKey, valueName.empty() ? NULL : valueName.c_str(), 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(data.c_str()),
                          static_cast<DWORD>((data.length() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    return lRes == ERROR_SUCCESS;
}

bool appendToUserPath(const std::wstring& newEntry) {
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ | KEY_WRITE, &hKey);
    if (lRes != ERROR_SUCCESS) return false;

    DWORD type = 0;
    DWORD size = 0;
    RegQueryValueExW(hKey, L"Path", NULL, &type, NULL, &size);

    std::wstring currentPath;
    if (size > 0) {
        currentPath.resize(size / sizeof(wchar_t));
        RegQueryValueExW(hKey, L"Path", NULL, &type, reinterpret_cast<BYTE*>(&currentPath[0]), &size);
        while (!currentPath.empty() && currentPath.back() == L'\0') {
            currentPath.pop_back();
        }
    }

    // Check if already in PATH
    if (currentPath.find(newEntry) == std::wstring::npos) {
        if (!currentPath.empty() && currentPath.back() != L';') {
            currentPath += L';';
        }
        currentPath += newEntry;

        RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ,
                       reinterpret_cast<const BYTE*>(currentPath.c_str()),
                       static_cast<DWORD>((currentPath.length() + 1) * sizeof(wchar_t)));
    }

    RegCloseKey(hKey);
    return true;
}

void registerFileAssociations(const std::wstring& spcExePath) {
    setRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\.sp", L"", L"SPLanguageSource");
    setRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\SPLanguageSource", L"", L"SP Source File");
    setRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\SPLanguageSource\\DefaultIcon", L"", L"\"" + spcExePath + L"\",0");
    setRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\SPLanguageSource\\shell\\open\\command", L"",
                     L"\"" + spcExePath + L"\" run \"%1\"");
    setRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\SPLanguageSource\\shell\\build\\command", L"",
                     L"\"" + spcExePath + L"\" build \"%1\"");
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

    std::string cmdLine(lpCmdLine ? lpCmdLine : "");
    bool silent = (cmdLine.find("/S") != std::string::npos ||
                   cmdLine.find("/silent") != std::string::npos ||
                   cmdLine.find("--silent") != std::string::npos);

    // Determine target installation directory
    std::wstring localAppData = getEnvVar(L"LOCALAPPDATA");
    if (localAppData.empty()) {
        localAppData = getEnvVar(L"USERPROFILE") + L"\\AppData\\Local";
    }
    std::wstring installDir = localAppData + L"\\SP-Language";
    std::wstring binDir = installDir + L"\\bin";
    std::wstring spcExe = binDir + L"\\spc.exe";

    if (!silent) {
        std::wstring welcomeMsg =
            L"Welcome to the SP Programming Language Setup Wizard!\n\n"
            L"This will install SP Language v3.1.0 on your system:\n"
            L"• Location: " + installDir + L"\n\n"
            L"Features:\n"
            L"  ✓ Native SP compiler (spc) & package manager (spm)\n"
            L"  ✓ Complete Standard Library & precompiled runtime\n"
            L"  ✓ Configures SP_HOME & User PATH automatically\n"
            L"  ✓ Associates .sp file extension (Run with SP)\n"
            L"  ✓ Zero external dependencies required\n\n"
            L"Do you want to proceed with the installation?";

        int res = MessageBoxW(NULL, welcomeMsg.c_str(), L"SP Programming Language Setup",
                              MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
        if (res != IDYES) {
            return 0;
        }
    }

    // 1. Create target directories
    std::error_code ec;
    fs::create_directories(installDir, ec);
    fs::create_directories(binDir, ec);

    // 2. Extract payload
    std::wstring tempDir = getEnvVar(L"TEMP");
    if (tempDir.empty()) tempDir = L"C:\\Windows\\Temp";
    std::wstring tempZip = tempDir + L"\\sp_setup_payload.zip";

    bool extractedPayload = extractPayloadResource(tempZip);
    if (!extractedPayload) {
        // Fallback: download package if embedded resource was not included
        std::wstring psDownload = L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \""
                                  L"[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; "
                                  L"Invoke-WebRequest -Uri 'https://dl.splang.shantopaul.com/sp-3.1.0-windows-x64.zip' "
                                  L"-OutFile '" + tempZip + L"' -UseBasicParsing\"";
        executeCommandHidden(psDownload);
    }

    if (fs::exists(tempZip)) {
        // Extract archive using tar.exe (native in Windows 10/11)
        std::wstring tarCmd = L"tar.exe -xf \"" + tempZip + L"\" -C \"" + installDir + L"\"";
        bool tarOk = executeCommandHidden(tarCmd);

        if (!tarOk || !fs::exists(spcExe)) {
            // Fallback: PowerShell Expand-Archive
            std::wstring psExpand = L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \""
                                    L"Expand-Archive -Path '" + tempZip + L"' -DestinationPath '" + installDir + L"' -Force\"";
            executeCommandHidden(psExpand);
        }

        // Clean up temporary payload zip
        fs::remove(tempZip, ec);
    }

    // 3. Configure SP_HOME in HKCU\Environment
    setRegistryValue(HKEY_CURRENT_USER, L"Environment", L"SP_HOME", installDir);

    // 4. Append bin directory to User PATH
    appendToUserPath(binDir);

    // 5. Broadcast WM_SETTINGCHANGE
    broadcastEnvironmentChange();

    // 6. Register .sp file associations
    if (fs::exists(spcExe)) {
        registerFileAssociations(spcExe);
    }

    // 7. Auto-configure toolchain (run spc setup in background)
    if (fs::exists(spcExe)) {
        std::wstring setupCmd = L"\"" + spcExe + L"\" setup";
        executeCommandHidden(setupCmd);
    }

    // 8. Completed successfully
    if (!silent) {
        std::wstring successMsg =
            L"SP Programming Language v3.1.0 has been installed successfully!\n\n"
            L"Installation Path: " + installDir + L"\n\n"
            L"Quickstart (open a new Command Prompt or PowerShell):\n"
            L"  spc --version         # Check compiler version\n"
            L"  spc doctor            # View system health\n"
            L"  spc run hello.sp      # Build & run directly\n"
            L"  spm init my_app       # Create a new project\n\n"
            L"You can also double-click any .sp file to run it immediately.";

        MessageBoxW(NULL, successMsg.c_str(), L"SP Language - Installation Complete",
                    MB_OK | MB_ICONINFORMATION);
    }

    return 0;
}
