# ==============================================================================
# SP Programming Language - Universal Windows Installer
# ==============================================================================
# One-command installation for any Windows Desktop (Windows 10, 11, Server).
# Zero pre-requisites required. Automatically sets up compiler, standard library,
# runtime, environment variables, and auto-provisions portable backend toolchains
# if missing.
#
# Usage (Online One-Liner):
#   irm https://splang.shantopaul.com/install.ps1 | iex
#
# Usage (Local Repository):
#   powershell -ExecutionPolicy Bypass -File install.ps1 [-InstallDir "C:\SP-Language"]
# ==============================================================================

[CmdletBinding()]
param(
    [string]$InstallDir = "$env:USERPROFILE\.sp",
    [string]$Version = "1.0.0",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Write-Banner {
    Write-Host ""
    Write-Host "========================================================================" -ForegroundColor Cyan
    Write-Host "         SP Programming Language - Windows Instant Installer          " -ForegroundColor Cyan
    Write-Host "                           Version $Version                             " -ForegroundColor White
    Write-Host "========================================================================" -ForegroundColor Cyan
    Write-Host "  Target Location : $InstallDir" -ForegroundColor Yellow
    Write-Host "  OS Architecture : $env:PROCESSOR_ARCHITECTURE" -ForegroundColor Gray
    Write-Host "========================================================================" -ForegroundColor Cyan
    Write-Host ""
}

Write-Banner

# 1. Prepare target installation directories
Write-Host "[1/5] Preparing installation directories..." -ForegroundColor Green
$BinDir = Join-Path $InstallDir "bin"
$LibDir = Join-Path $InstallDir "lib"
$IncludeDir = Join-Path $InstallDir "runtime\include"
$StdlibDir = Join-Path $InstallDir "stdlib"
$ExamplesDir = Join-Path $InstallDir "examples"
$ToolchainDir = Join-Path $InstallDir "toolchain"

$directories = @($InstallDir, $BinDir, $LibDir, $IncludeDir, $StdlibDir, $ExamplesDir)
foreach ($dir in $directories) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}

# 2. Deploy SP Binaries and Standard Library
Write-Host "`n[2/5] Deploying SP Language Core Components..." -ForegroundColor Green

$repoRoot = $PSScriptRoot
if ($repoRoot -ne "" -and -not (Test-Path (Join-Path $repoRoot "build\bin\spc.exe"))) {
    $parent = Split-Path $repoRoot -Parent
    if (Test-Path (Join-Path $parent "build\bin\spc.exe")) {
        $repoRoot = $parent
    } elseif (Test-Path (Join-Path (Split-Path $parent -Parent) "build\bin\spc.exe")) {
        $repoRoot = Split-Path $parent -Parent
    }
}

$localBuildBin = if ($repoRoot -ne "") { Join-Path $repoRoot "build\bin" } else { "" }
$isLocalRepo = ($localBuildBin -ne "" -and (Test-Path (Join-Path $localBuildBin "spc.exe")))

if ($isLocalRepo) {
    Write-Host "  -> Installing from local repository build artifacts..." -ForegroundColor Cyan
    
    # Copy binaries
    Get-ChildItem -Path $localBuildBin -Filter "*.exe" | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $BinDir -Force
        Write-Host "     Installed binary: $($_.Name)" -ForegroundColor Gray
    }
    
    # Copy runtime library
    $localRuntimeLib = Join-Path $repoRoot "build\lib\libsp_runtime.a"
    if (Test-Path $localRuntimeLib) {
        Copy-Item -Path $localRuntimeLib -Destination $LibDir -Force
        Write-Host "     Installed runtime library: libsp_runtime.a" -ForegroundColor Gray
    }
    
    # Copy runtime headers
    $localHeaders = Join-Path $repoRoot "runtime\include"
    if (Test-Path $localHeaders) {
        Copy-Item -Path "$localHeaders\*" -Destination $IncludeDir -Recurse -Force
        Write-Host "     Installed runtime headers" -ForegroundColor Gray
    }
    
    # Copy standard library
    $localStdlib = Join-Path $repoRoot "stdlib"
    if (Test-Path $localStdlib) {
        Copy-Item -Path "$localStdlib\*" -Destination $StdlibDir -Recurse -Force
        Write-Host "     Installed standard library modules" -ForegroundColor Gray
    }

    # Copy examples
    $localExamples = Join-Path $repoRoot "examples"
    if (Test-Path $localExamples) {
        Copy-Item -Path "$localExamples\*" -Destination $ExamplesDir -Recurse -Force
    }
} else {
    Write-Host "  -> Downloading SP distribution package ($Version)..." -ForegroundColor Cyan
    $ZipName = "sp-$Version-windows-x64.zip"
    $TempZip = Join-Path $env:TEMP $ZipName
    
    $Urls = @(
        "https://downloads.splang.shantopaul.com/$ZipName",
        "https://splang.shantopaul.com/downloads/$ZipName",
        "https://dl.splang.shantopaul.com/$ZipName",
        "https://github.com/shantopaul/SP-Language/releases/download/v$Version/$ZipName"
    )
    
    $Downloaded = $false
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    foreach ($Url in $Urls) {
        try {
            Write-Host "     Connecting to: $Url" -ForegroundColor Gray
            Invoke-WebRequest -Uri $Url -OutFile $TempZip -UseBasicParsing -TimeoutSec 90
            if ((Test-Path $TempZip) -and ((Get-Item $TempZip).Length -gt 1000)) {
                $Downloaded = $true
                Write-Host "     Package downloaded successfully!" -ForegroundColor Green
                break
            }
        } catch {
            Write-Host "     Mirror unreachable, attempting next mirror..." -ForegroundColor DarkGray
        }
    }
    
    if ($Downloaded) {
        Write-Host "     Extracting package to $InstallDir..." -ForegroundColor Cyan
        Expand-Archive -Path $TempZip -DestinationPath $InstallDir -Force
        Remove-Item -Path $TempZip -Force -ErrorAction SilentlyContinue
    } else {
        if (Test-Path "C:\SP-Language\bin\spc.exe") {
            Write-Host "     Using pre-existing local binaries from C:\SP-Language..." -ForegroundColor Yellow
            Copy-Item -Path "C:\SP-Language\*" -Destination $InstallDir -Recurse -Force
        } else {
            throw "Failed to download SP Language distribution package from all release mirrors."
        }
    }
}

# Ensure hello.sp example exists
$HelloSp = Join-Path $ExamplesDir "hello.sp"
if (-not (Test-Path $HelloSp)) {
    @"
import std.io;

fn main() {
    println("Hello, World from SP Programming Language!");
    println("Native compilation on Windows Desktop is fully operational.");
}
"@ | Out-File -FilePath $HelloSp -Encoding utf8
}

# 3. Environment Variables Configuration
Write-Host "`n[3/5] Configuring System / User Environment Variables..." -ForegroundColor Green
[Environment]::SetEnvironmentVariable("SP_HOME", $InstallDir, [EnvironmentVariableTarget]::User)
$env:SP_HOME = $InstallDir
Write-Host "  -> SP_HOME set to: $InstallDir" -ForegroundColor Cyan

$UserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
$PathEntries = if ([string]::IsNullOrEmpty($UserPath)) { @() } else { $UserPath -split ";" }

$PathsToAdd = @($BinDir)
$toolchainBin = Join-Path $ToolchainDir "bin"
if (Test-Path $toolchainBin) {
    $PathsToAdd += $toolchainBin
}

$PathModified = $false
foreach ($p in $PathsToAdd) {
    if ($PathEntries -notcontains $p) {
        $PathEntries += $p
        $PathModified = $true
        Write-Host "  -> Added to PATH: $p" -ForegroundColor Cyan
    } else {
        Write-Host "  -> Already in PATH: $p" -ForegroundColor Gray
    }
    if ($env:Path -notlike "*$p*") {
        $env:Path = "$p;$env:Path"
    }
}

if ($PathModified) {
    $NewUserPath = ($PathEntries -join ";").TrimEnd(";")
    [Environment]::SetEnvironmentVariable("Path", $NewUserPath, [EnvironmentVariableTarget]::User)
}

# 4. Backend Toolchain Auto-Discovery & Provisioning
Write-Host "`n[4/5] Checking Native C/LLVM Backend Toolchain..." -ForegroundColor Green

$hasClang = (Get-Command clang++.exe -ErrorAction SilentlyContinue) -ne $null
$hasClangC = (Get-Command clang.exe -ErrorAction SilentlyContinue) -ne $null
$hasGcc = (Get-Command gcc.exe -ErrorAction SilentlyContinue) -ne $null
$hasBundled = (Test-Path (Join-Path $toolchainBin "clang++.exe")) -or (Test-Path (Join-Path $toolchainBin "clang.exe"))

if ($hasClang -or $hasClangC -or $hasGcc -or $hasBundled) {
    Write-Host "  -> Backend C/LLVM toolchain is detected and ready." -ForegroundColor Green
} else {
    Write-Host "  -> No backend C/LLVM compiler detected on this PC." -ForegroundColor Yellow
    Write-Host "     Auto-provisioning portable Windows toolchain for instant out-of-the-box native compilation..." -ForegroundColor Yellow
    
    $SpcExe = Join-Path $BinDir "spc.exe"
    if (Test-Path $SpcExe) {
        & $SpcExe setup
    }
}

# 5. Self-Diagnosis and Smoke Test
Write-Host "`n[5/5] Performing System Verification & Smoke Test..." -ForegroundColor Green

$SpcExe = Join-Path $BinDir "spc.exe"
if (Test-Path $SpcExe) {
    & $SpcExe doctor
    
    Write-Host "`n[*] Executing native smoke test ($HelloSp)..." -ForegroundColor Cyan
    try {
        & $SpcExe run $HelloSp
    } catch {
        Write-Host "Notice: Smoke test invocation finished. Run 'spc run $HelloSp' to re-test." -ForegroundColor Gray
    }
} else {
    Write-Error "spc.exe was not found in $BinDir"
}

Write-Host ""
Write-Host "========================================================================" -ForegroundColor Green
Write-Host "           SP Programming Language Successfully Installed!              " -ForegroundColor Green
Write-Host "========================================================================" -ForegroundColor Green
Write-Host "  Quickstart Commands (open a new terminal window):" -ForegroundColor White
Write-Host "    spc --version                 # Check compiler version" -ForegroundColor Gray
Write-Host "    spc doctor                    # View toolchain health report" -ForegroundColor Gray
Write-Host "    spc run examples\hello.sp     # Build & run directly" -ForegroundColor Gray
Write-Host "    spc build main.sp -o app.exe  # Build native standalone executable" -ForegroundColor Gray
Write-Host "    spm init my_project           # Create a new SP project" -ForegroundColor Gray
Write-Host "========================================================================" -ForegroundColor Green
Write-Host ""
