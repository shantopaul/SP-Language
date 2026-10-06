# SP Programming Language Installer for Windows
# Usage: powershell -ExecutionPolicy Bypass -File install.ps1 [-InstallDir "C:\SP-Language"]

param(
    [string]$InstallDir = "C:\SP-Language"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "       SP Programming Language - Installation Setup       " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "Target Directory: $InstallDir" -ForegroundColor Yellow

$scriptRoot = $PSScriptRoot

# 1. Build using CMake & Ninja
Write-Host "`n[1/4] Building SP Compiler in Release mode..." -ForegroundColor Green
$cmakeCmd = $null
$cmakeObj = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeObj) {
    $cmakeCmd = $cmakeObj.Source
} elseif (Test-Path "K:\AndroidDev\tools\sdk\cmake\3.22.1\bin\cmake.exe") {
    $cmakeCmd = "K:\AndroidDev\tools\sdk\cmake\3.22.1\bin\cmake.exe"
    $cmakeDir = Split-Path $cmakeCmd
    if ($env:Path -notlike "*$cmakeDir*") {
        $env:Path = "$cmakeDir;$env:Path"
    }
} else {
    Write-Error "CMake was not found in PATH or known locations. Please install CMake."
}

# Ensure Ninja and Clang are in path if present in Android SDK
if (Test-Path "K:\AndroidDev\tools\sdk\cmake\3.22.1\bin") {
    $binDir = "K:\AndroidDev\tools\sdk\cmake\3.22.1\bin"
    if ($env:Path -notlike "*$binDir*") { $env:Path = "$binDir;$env:Path" }
}
if (Test-Path "K:\AndroidDev\tools\sdk\ndk\28.2.13676358\toolchains\llvm\prebuilt\windows-x86_64\bin") {
    $ndkBin = "K:\AndroidDev\tools\sdk\ndk\28.2.13676358\toolchains\llvm\prebuilt\windows-x86_64\bin"
    if ($env:Path -notlike "*$ndkBin*") { $env:Path = "$ndkBin;$env:Path" }
}

& $cmakeCmd -B "$scriptRoot\build" -G "Ninja" -DCMAKE_BUILD_TYPE=Release
& $cmakeCmd --build "$scriptRoot\build" --target spc spm

# 2. Install to target directory
Write-Host "`n[2/4] Installing SP toolchain into $InstallDir..." -ForegroundColor Green
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}

& $cmakeCmd --install "$scriptRoot\build" --prefix $InstallDir

# 3. Configure SP_HOME and PATH in User Environment Variables
Write-Host "`n[3/4] Configuring Environment Variables (SP_HOME & PATH)..." -ForegroundColor Green
[Environment]::SetEnvironmentVariable("SP_HOME", $InstallDir, [EnvironmentVariableTarget]::User)
$currentPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
$spBinDir = "$InstallDir\bin"

if ($currentPath -notlike "*$spBinDir*") {
    $newPath = "$currentPath;$spBinDir"
    [Environment]::SetEnvironmentVariable("Path", $newPath, [EnvironmentVariableTarget]::User)
    Write-Host "Added $spBinDir to User PATH." -ForegroundColor Yellow
} else {
    Write-Host "$spBinDir is already in User PATH." -ForegroundColor Yellow
}

# Update current session environment
$env:SP_HOME = $InstallDir
$env:Path = "$env:Path;$spBinDir"

# 4. Verify Installation
Write-Host "`n[4/4] Verifying Installation..." -ForegroundColor Green
$spcExe = "$spBinDir\spc.exe"
$spmExe = "$spBinDir\spm.exe"
if ((Test-Path $spcExe) -and (Test-Path $spmExe)) {
    & $spcExe --version
    & $spmExe --version
    Write-Host "`n==========================================================" -ForegroundColor Green
    Write-Host "  Installation Complete!                                  " -ForegroundColor Green
    Write-Host "  You can now open any terminal and type:                 " -ForegroundColor Green
    Write-Host "      spc run main.sp                                     " -ForegroundColor White
    Write-Host "      spc build main.sp -o myapp.exe                      " -ForegroundColor White
    Write-Host "      spm init myproject                                  " -ForegroundColor White
    Write-Host "==========================================================" -ForegroundColor Green
} else {
    Write-Error "spc.exe or spm.exe not found in $spBinDir after installation."
}
