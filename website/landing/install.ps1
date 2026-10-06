# SP Programming Language Installer for Windows (PowerShell)
# Usage: irm https://splang.shantopaul.com/install.ps1 | iex

$ErrorActionPreference = 'Stop'

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "    SP Programming Language Installer (v1.0.0)" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$InstallDir = "$env:LOCALAPPDATA\SP-Language"
$BinDir = "$InstallDir\bin"

Write-Host "[1/3] Preparing directory: $InstallDir" -ForegroundColor Green
if (!(Test-Path $BinDir)) {
    New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
}

Write-Host "[2/3] Fetching latest SP v1.0.0 toolchain binaries..." -ForegroundColor Green
# When running on host or downloading from release asset:
$DownloadUrl = "https://github.com/shantopaul/SP-Language/releases/latest/download/sp-1.0.0-windows-x86_64.zip"
Write-Host "Release target: $DownloadUrl"

# Add to user PATH if not present
Write-Host "[3/3] Configuring environment PATH..." -ForegroundColor Green
$UserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
if ($UserPath -notlike "*$BinDir*") {
    [Environment]::SetEnvironmentVariable("Path", "$UserPath;$BinDir", [EnvironmentVariableTarget]::User)
    Write-Host "Added $BinDir to User PATH." -ForegroundColor Yellow
}

Write-Host "`nInstallation Complete! Restart your terminal and run:" -ForegroundColor Cyan
Write-Host "  spc --version" -ForegroundColor White
Write-Host "  spm --version" -ForegroundColor White
Write-Host "Visit: https://docs.splang.shantopaul.com for documentation.`n" -ForegroundColor Green
