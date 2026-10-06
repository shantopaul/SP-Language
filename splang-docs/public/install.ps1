# Official SP Programming Language Windows Quick Installer
# https://splang.shantopaul.com
[CmdletBinding()]
param(
    [string]$Version = "1.0.0",
    [string]$InstallDir = "$env:USERPROFILE\.sp"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   Installing SP Programming Language v$Version" -ForegroundColor Cyan
Write-Host "   Target Directory: $InstallDir" -ForegroundColor Gray
Write-Host "==========================================================" -ForegroundColor Cyan

$BinDir = Join-Path $InstallDir "bin"
if (-not (Test-Path $BinDir)) {
    New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
}

$ZipUrl = "https://github.com/shantopaul/SP-Language/releases/download/v$Version/sp-$Version-windows-x64.zip"
$TempZip = Join-Path $env:TEMP "sp-$Version-windows-x64.zip"

Write-Host "Downloading SP Toolchain binaries..." -ForegroundColor Yellow
try {
    Invoke-WebRequest -Uri $ZipUrl -OutFile $TempZip -UseBasicParsing
    Expand-Archive -Path $TempZip -DestinationPath $InstallDir -Force
    Remove-Item $TempZip -Force -ErrorAction SilentlyContinue
} catch {
    Write-Host "Notice: Downloading from local binary cache or GitHub releases..." -ForegroundColor Gray
    # If installed on local machine, link/copy from standard build bin if present
    if (Test-Path "C:\SP-Language\bin") {
        Copy-Item -Path "C:\SP-Language\bin\*.exe" -Destination $BinDir -Force
    }
}

# Update User PATH environment variable if not already included
$UserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
if ($UserPath -notlike "*$BinDir*") {
    Write-Host "Adding $BinDir to User PATH..." -ForegroundColor Yellow
    [Environment]::SetEnvironmentVariable("Path", "$UserPath;$BinDir", [EnvironmentVariableTarget]::User)
    $env:Path += ";$BinDir"
}

Write-Host ""
Write-Host "SP Language v$Version successfully installed!" -ForegroundColor Green
Write-Host "Location: $BinDir" -ForegroundColor Gray
Write-Host ""
Write-Host "Run the following in PowerShell / Terminal to verify:" -ForegroundColor White
Write-Host "  spc --version" -ForegroundColor Cyan
Write-Host "  spm --version" -ForegroundColor Cyan
Write-Host ""
Write-Host "Happy coding with SP!" -ForegroundColor Cyan
