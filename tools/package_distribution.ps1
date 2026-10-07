# ==============================================================================
# SP Programming Language - Windows Distribution & Installer Packager
# ==============================================================================
# Builds release artifacts and packages:
#   1. Standalone portable archive: dist/sp-1.0.0-windows-x64.zip
#   2. Windows GUI Installer:       dist/sp-setup.exe (if Inno Setup is available)
# ==============================================================================

[CmdletBinding()]
param(
    [string]$Version = "1.0.0",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = (Split-Path $PSScriptRoot -Parent)

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "         Packaging SP Programming Language v$Version Distribution       " -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan

# 1. Build release binaries if needed
if (-not $SkipBuild) {
    Write-Host "`n[1/4] Building core targets (Release)..." -ForegroundColor Green
    $buildDir = Join-Path $repoRoot "build"
    cmake --build $buildDir --config Release --target spc spm sp_runtime
}

# 2. Prepare staging directory
Write-Host "`n[2/4] Preparing distribution layout in dist/..." -ForegroundColor Green
$distDir = Join-Path $repoRoot "dist"
$stageDir = Join-Path $distDir "SP-Language"

if (Test-Path $stageDir) {
    Remove-Item -Path $stageDir -Recurse -Force
}
New-Item -ItemType Directory -Path "$stageDir\bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageDir\lib" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageDir\runtime\include" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageDir\stdlib" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageDir\examples" -Force | Out-Null

# Copy binaries
$binFiles = @("spc.exe", "spm.exe", "spd.exe", "spls.exe")
foreach ($bin in $binFiles) {
    $srcBin = Join-Path $repoRoot "build\bin\$bin"
    if (Test-Path $srcBin) {
        Copy-Item -Path $srcBin -Destination "$stageDir\bin" -Force
        Write-Host "  -> Bundled binary: $bin" -ForegroundColor Gray
    }
}

# Copy runtime library
$runtimeLib = Join-Path $repoRoot "build\lib\libsp_runtime.a"
if (Test-Path $runtimeLib) {
    Copy-Item -Path $runtimeLib -Destination "$stageDir\lib" -Force
    Write-Host "  -> Bundled runtime: libsp_runtime.a" -ForegroundColor Gray
}

# Copy runtime headers
$runtimeHeaders = Join-Path $repoRoot "runtime\include"
if (Test-Path $runtimeHeaders) {
    Copy-Item -Path "$runtimeHeaders\*" -Destination "$stageDir\runtime\include" -Recurse -Force
    Write-Host "  -> Bundled runtime headers" -ForegroundColor Gray
}

# Copy stdlib
$stdlibDir = Join-Path $repoRoot "stdlib"
if (Test-Path $stdlibDir) {
    Copy-Item -Path "$stdlibDir\*" -Destination "$stageDir\stdlib" -Recurse -Force
    Write-Host "  -> Bundled standard library modules" -ForegroundColor Gray
}

# Copy examples
$examplesDir = Join-Path $repoRoot "examples"
if (Test-Path $examplesDir) {
    Copy-Item -Path "$examplesDir\*" -Destination "$stageDir\examples" -Recurse -Force
    Write-Host "  -> Bundled examples" -ForegroundColor Gray
}

# Copy installer and docs
Copy-Item -Path (Join-Path $repoRoot "install.ps1") -Destination $stageDir -Force
if (Test-Path (Join-Path $repoRoot "README.md")) {
    Copy-Item -Path (Join-Path $repoRoot "README.md") -Destination $stageDir -Force
}
if (Test-Path (Join-Path $repoRoot "LICENSE")) {
    Copy-Item -Path (Join-Path $repoRoot "LICENSE") -Destination $stageDir -Force
}

# 3. Create zip archive
Write-Host "`n[3/4] Creating distribution ZIP archive..." -ForegroundColor Green
$zipOutput = Join-Path $distDir "sp-$Version-windows-x64.zip"
if (Test-Path $zipOutput) {
    Remove-Item -Path $zipOutput -Force
}

Compress-Archive -Path "$stageDir\*" -DestinationPath $zipOutput -CompressionLevel Optimal
$zipItem = Get-Item $zipOutput
$zipHash = (Get-FileHash -Path $zipOutput -Algorithm SHA256).Hash

Write-Host "  -> Archive created: $zipOutput" -ForegroundColor Cyan
Write-Host "     Size:   $([math]::Round($zipItem.Length / 1MB, 2)) MB" -ForegroundColor Gray
Write-Host "     SHA256: $zipHash" -ForegroundColor Yellow

# 4. Compile Inno Setup GUI installer if iscc is present
Write-Host "`n[4/4] Checking for Inno Setup compiler (iscc.exe)..." -ForegroundColor Green
$issFile = Join-Path $repoRoot "installer\sp_installer.iss"
$iscc = Get-Command iscc.exe -ErrorAction SilentlyContinue

if ($iscc -and (Test-Path $issFile)) {
    Write-Host "  -> Building native Windows installer: sp-setup.exe..." -ForegroundColor Cyan
    & $iscc.Source "/DAppVersion=$Version" "/O$distDir" $issFile
    $exeOutput = Join-Path $distDir "sp-setup.exe"
    if (Test-Path $exeOutput) {
        $exeHash = (Get-FileHash -Path $exeOutput -Algorithm SHA256).Hash
        Write-Host "  -> Installer generated: $exeOutput" -ForegroundColor Green
        Write-Host "     SHA256: $exeHash" -ForegroundColor Yellow
    }
} else {
    Write-Host "  -> iscc.exe not in PATH. Inno Setup installer script is ready at:" -ForegroundColor DarkGray
    Write-Host "     $issFile" -ForegroundColor DarkGray
}

Write-Host "`n========================================================================" -ForegroundColor Green
Write-Host "                 Distribution Packaging Complete!                       " -ForegroundColor Green
Write-Host "========================================================================" -ForegroundColor Green
