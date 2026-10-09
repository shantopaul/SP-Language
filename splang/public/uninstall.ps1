# ==============================================================================
# SP Programming Language - Universal Windows Uninstaller
# ==============================================================================
# Completely uninstalls SP Programming Language from Windows:
# - Cleans environment variables (SP_HOME)
# - Cleans user PATH (.sp\bin, .sp\toolchain\bin)
# - Unregisters from Windows Installed Apps
# - Removes installed files, runtimes, and portable toolchains
#
# Usage (Online One-Liner):
#   irm https://dl.splang.shantopaul.com/uninstall.ps1 | iex
#   irm https://splang.shantopaul.com/uninstall.ps1 | iex
#
# Usage (Local / Custom Path):
#   powershell -ExecutionPolicy Bypass -File uninstall.ps1 -InstallDir "C:\Users\<Name>\.sp" -Force
# ==============================================================================

[CmdletBinding()]
param(
    [string]$InstallDir = $(
        if ($env:SP_HOME -and (Test-Path $env:SP_HOME)) {
            $env:SP_HOME
        } elseif ($env:USERPROFILE -and (Test-Path (Join-Path $env:USERPROFILE ".sp"))) {
            Join-Path $env:USERPROFILE ".sp"
        } elseif (-not [string]::IsNullOrEmpty($PSScriptRoot) -and (Test-Path (Join-Path $PSScriptRoot "bin\spc.exe"))) {
            $PSScriptRoot
        } else {
            Join-Path $env:USERPROFILE ".sp"
        }
    ),
    [switch]$Force,
    [switch]$Yes,
    [switch]$Silent
)

$ErrorActionPreference = "Continue"

Write-Host ""
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "         SP Programming Language - Complete System Uninstaller          " -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "  Target Location : $InstallDir" -ForegroundColor Yellow
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host ""

$Confirmed = $Force -or $Yes -or $Silent
if (-not $Confirmed) {
    $response = Read-Host "Are you sure you want to completely uninstall SP Language from this PC? [y/N]"
    if ($response -notmatch '^(y|yes)$') {
        Write-Host "`nUninstallation canceled. No files or settings were modified.`n" -ForegroundColor Gray
        exit 0
    }
}

Write-Host "`n[*] Removing SP Programming Language..." -ForegroundColor Green

# 1. Remove SP_HOME user environment variable
Write-Host "  -> Removing SP_HOME environment variable..." -ForegroundColor Cyan
[Environment]::SetEnvironmentVariable("SP_HOME", $null, [EnvironmentVariableTarget]::User)
if ($env:SP_HOME) { Remove-Item Env:\SP_HOME -ErrorAction SilentlyContinue }

# 2. Clean User PATH
Write-Host "  -> Cleaning User PATH..." -ForegroundColor Cyan
$UserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
if (-not [string]::IsNullOrEmpty($UserPath)) {
    $Entries = $UserPath -split ";" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $NormalizedInstallDir = $InstallDir.TrimEnd('\', '/')
    
    $Filtered = @()
    foreach ($entry in $Entries) {
        $cleanEntry = $entry.Trim().TrimEnd('\', '/')
        $isSpEntry = $cleanEntry -like "*\.sp\bin" -or
                     $cleanEntry -like "*\.sp\toolchain\bin" -or
                     $cleanEntry -like "*\.sp" -or
                     $cleanEntry -like "*\SP-Language\bin" -or
                     $cleanEntry -eq $NormalizedInstallDir -or
                     $cleanEntry -like "$NormalizedInstallDir\*"
        
        if (-not $isSpEntry) {
            $Filtered += $entry
        } else {
            Write-Host "     Removed from PATH: $entry" -ForegroundColor DarkGray
        }
    }
    
    $NewUserPath = ($Filtered -join ";").TrimEnd(";")
    [Environment]::SetEnvironmentVariable("Path", $NewUserPath, [EnvironmentVariableTarget]::User)
}

# 3. Clean Process PATH so current session doesn't see spc
if ($env:Path) {
    $CurrentEntries = $env:Path -split ";"
    $NewCurrent = $CurrentEntries | Where-Object {
        $_ -notlike "*\.sp\bin*" -and
        $_ -notlike "*\.sp\toolchain\bin*" -and
        $_ -notlike "*$InstallDir\bin*" -and
        $_ -notlike "*$InstallDir\toolchain*"
    }
    $env:Path = ($NewCurrent -join ";").TrimEnd(";")
}

# 4. Unregister from Windows Installed Apps (Programs & Features)
Write-Host "  -> Unregistering from Windows Installed Apps..." -ForegroundColor Cyan
$RegKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\SPLang"
if (Test-Path $RegKey) {
    Remove-Item -Path $RegKey -Recurse -Force -ErrorAction SilentlyContinue
}

# 5. Broadcast WM_SETTINGCHANGE so all windows update environment immediately
Write-Host "  -> Broadcasting system environment change..." -ForegroundColor Cyan
try {
    Add-Type -Namespace Win32 -Name NativeMethodsUninstall -MemberDefinition @"
[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
"@ -ErrorAction SilentlyContinue | Out-Null
    $HWND_BROADCAST = [IntPtr]0xffff
    $WM_SETTINGCHANGE = 0x001a
    $result = [UIntPtr]::Zero
    [Win32.NativeMethodsUninstall]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment", 2, 2000, [ref]$result) | Out-Null
} catch {}

# 6. Delete files and directories
if (Test-Path $InstallDir) {
    Write-Host "  -> Deleting SP installation directory: $InstallDir..." -ForegroundColor Cyan
    
    # Remove subfolders first
    $subfolders = @("bin", "lib", "stdlib", "runtime", "examples", "toolchain", "include", "docs")
    foreach ($sub in $subfolders) {
        $p = Join-Path $InstallDir $sub
        if (Test-Path $p) {
            Remove-Item -Path $p -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    
    # Remove remaining contents
    try {
        Get-ChildItem -Path $InstallDir -Recurse -Force | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path $InstallDir -Recurse -Force -ErrorAction SilentlyContinue
    } catch {}

    # If directory still exists (e.g. running from within it), schedule detached background deletion
    if (Test-Path $InstallDir) {
        Start-Process cmd.exe -ArgumentList "/c timeout /t 2 /nobreak > nul & rmdir /s /q `"$InstallDir`"" -WindowStyle Hidden
    }
}

Write-Host ""
Write-Host "========================================================================" -ForegroundColor Green
Write-Host "       SP Programming Language Successfully Uninstalled!                " -ForegroundColor Green
Write-Host "========================================================================" -ForegroundColor Green
Write-Host "All compiler binaries, package managers, runtimes, libraries, and" -ForegroundColor White
Write-Host "environment variables have been cleanly removed from this computer." -ForegroundColor White
Write-Host "========================================================================" -ForegroundColor Green
Write-Host ""
