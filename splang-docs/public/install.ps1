# ==============================================================================
# SP Programming Language - Universal Windows Installer
# ==============================================================================
# One-command installation for any Windows Desktop (Windows 10, 11, Server).
# Zero pre-requisites required. Automatically sets up compiler, package manager,
# standard library, runtime, static OpenSSL libraries, environment variables,
# and auto-provisions portable backend LLVM Clang toolchains if missing.
#
# Usage (Online One-Liner):
#   irm https://dl.splang.shantopaul.com/install.ps1 | iex
#   irm https://splang.shantopaul.com/install.ps1 | iex
#
# Usage (Custom Drive / Path):
#   powershell -ExecutionPolicy Bypass -File install.ps1 -InstallDir "D:\SP-Language"
# ==============================================================================

[CmdletBinding()]
param(
    [string]$InstallDir = $(if ($env:SP_INSTALL_DIR) { $env:SP_INSTALL_DIR } elseif ($env:USERPROFILE) { Join-Path $env:USERPROFILE ".sp" } else { "C:\SP-Language" }),
    [string]$Version = "3.1.0",
    [switch]$Force,
    [switch]$SkipToolchain
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
    Write-Host "  Operating System: $([System.Environment]::OSVersion.VersionString)" -ForegroundColor Gray
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
$ToolchainBin = Join-Path $ToolchainDir "bin"

$directories = @($InstallDir, $BinDir, $LibDir, $IncludeDir, $StdlibDir, $ExamplesDir)
foreach ($dir in $directories) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}

# 2. Deploy SP Binaries, Standard Library & Runtime
Write-Host "`n[2/5] Deploying SP Language Core Components..." -ForegroundColor Green

# Scan for local repository build artifacts (only when executed from a saved local script file)
$repoRoot = $null
$isLocalRepo = $false

if (-not [string]::IsNullOrEmpty($PSScriptRoot)) {
    $candidateRoots = @($PSScriptRoot)
    try {
        $parent = Split-Path $PSScriptRoot -Parent
        if (-not [string]::IsNullOrEmpty($parent)) {
            $candidateRoots += $parent
        }
    } catch {}

    foreach ($cand in $candidateRoots) {
        if (-not [string]::IsNullOrEmpty($cand)) {
            $candBin = Join-Path $cand "build\bin"
            if (Test-Path (Join-Path $candBin "spc.exe")) {
                $repoRoot = $cand
                $isLocalRepo = $true
                break
            }
        }
    }
}

if ($isLocalRepo) {
    Write-Host "  -> Installing from local repository build artifacts: $repoRoot" -ForegroundColor Cyan
    $localBuildBin = Join-Path $repoRoot "build\bin"
    
    # Copy core binaries
    Get-ChildItem -Path $localBuildBin -Filter "*.exe" | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $BinDir -Force
        Write-Host "     Installed binary: $($_.Name)" -ForegroundColor Gray
    }
    
    # Copy precompiled runtime static library
    $localRuntimeLib = Join-Path $repoRoot "build\lib\libsp_runtime.a"
    if (Test-Path $localRuntimeLib) {
        Copy-Item -Path $localRuntimeLib -Destination $LibDir -Force
        Write-Host "     Installed runtime library: libsp_runtime.a" -ForegroundColor Gray
    }
    
    # Copy or bundle static OpenSSL libraries (libssl.a, libcrypto.a)
    $sslCandidates = @(
        (Join-Path $repoRoot "build\lib\libssl.a"),
        "C:\msys64\ucrt64\lib\libssl.a",
        "C:\msys64\mingw64\lib\libssl.a",
        "C:\msys64\clang64\lib\libssl.a"
    )
    $cryptoCandidates = @(
        (Join-Path $repoRoot "build\lib\libcrypto.a"),
        "C:\msys64\ucrt64\lib\libcrypto.a",
        "C:\msys64\mingw64\lib\libcrypto.a",
        "C:\msys64\clang64\lib\libcrypto.a"
    )

    foreach ($s in $sslCandidates) {
        if (Test-Path $s) {
            Copy-Item -Path $s -Destination (Join-Path $LibDir "libssl.a") -Force
            Write-Host "     Installed OpenSSL library: libssl.a" -ForegroundColor Gray
            break
        }
    }

    foreach ($c in $cryptoCandidates) {
        if (Test-Path $c) {
            Copy-Item -Path $c -Destination (Join-Path $LibDir "libcrypto.a") -Force
            Write-Host "     Installed OpenSSL library: libcrypto.a" -ForegroundColor Gray
            break
        }
    }
    
    # Copy runtime headers
    $localHeaders = Join-Path $repoRoot "runtime\include"
    if (Test-Path $localHeaders) {
        Copy-Item -Path "$localHeaders\*" -Destination $IncludeDir -Recurse -Force
        Write-Host "     Installed runtime headers" -ForegroundColor Gray
    }
    
    # Copy standard library modules
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
    $ZipNames = @(
        "sp-$Version-windows-x64.zip",
        "sp-windows-x64.zip",
        "sp-1.0.0-windows-x64.zip"
    )
    $TempZip = Join-Path $env:TEMP "sp_download_$([Guid]::NewGuid().ToString('N')).zip"
    
    $ts = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $Urls = @()
    foreach ($zn in $ZipNames) {
        $Urls += "https://dl.splang.shantopaul.com/$($zn)?v=$($Version)&t=$($ts)"
        $Urls += "https://splang.shantopaul.com/downloads/$($zn)?v=$($Version)&t=$($ts)"
        $Urls += "https://downloads.splang.shantopaul.com/$($zn)?v=$($Version)&t=$($ts)"
        $Urls += "https://github.com/shantopaul/SP-Language/releases/download/v$($Version)/$($zn)"
        $Urls += "https://github.com/shantopaul/SP-Language/releases/latest/download/$($zn)"
    }
    
    $Downloaded = $false
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
    foreach ($Url in $Urls) {
        try {
            Write-Host "     Connecting to: $Url" -ForegroundColor Gray
            Invoke-WebRequest -Uri $Url -OutFile $TempZip -UseBasicParsing -TimeoutSec 60
            if ((Test-Path $TempZip) -and ((Get-Item $TempZip).Length -gt 50000)) {
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
        # Fallback check: check if C:\SP-Language or ~/.sp has pre-existing binaries
        $fallbackPaths = @("C:\SP-Language", "D:\SP-Language", "$env:LOCALAPPDATA\SP-Language")
        $foundFallback = $false
        foreach ($fb in $fallbackPaths) {
            try {
                if (-not [string]::IsNullOrEmpty($fb) -and (Test-Path $fb -ErrorAction SilentlyContinue)) {
                    $candSpc = Join-Path $fb "bin\spc.exe"
                    if (Test-Path $candSpc -ErrorAction SilentlyContinue) {
                        Write-Host "     Using pre-existing local distribution from $fb..." -ForegroundColor Yellow
                        Copy-Item -Path "$fb\*" -Destination $InstallDir -Recurse -Force
                        $foundFallback = $true
                        break
                    }
                }
            } catch {}
        }
        if (-not $foundFallback) {
            throw "Failed to download SP Language distribution package from all release mirrors."
        }
    }
}

# Ensure hello.sp example exists
$HelloSp = Join-Path $ExamplesDir "hello.sp"
    $helloCode = @"
import std.io;

fn main() {
    println("Hello, World from SP Programming Language!");
    println("Native compilation on Windows Desktop is fully operational.");
}
"@
    [System.IO.File]::WriteAllText($HelloSp, $helloCode, [System.Text.Encoding]::ASCII)

# 3. Environment Variables Configuration
Write-Host "`n[3/5] Configuring System / User Environment Variables..." -ForegroundColor Green
[Environment]::SetEnvironmentVariable("SP_HOME", $InstallDir, [EnvironmentVariableTarget]::User)
$env:SP_HOME = $InstallDir
Write-Host "  -> SP_HOME set to: $InstallDir" -ForegroundColor Cyan

$UserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
$PathEntries = if ([string]::IsNullOrEmpty($UserPath)) { @() } else { $UserPath -split ";" }

$PathsToAdd = @($BinDir)
if (Test-Path $ToolchainBin) {
    $PathsToAdd += $ToolchainBin
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

# Broadcast WM_SETTINGCHANGE so new shells recognize PATH immediately without restarting PC
try {
    Add-Type -Namespace Win32 -Name NativeMethods -MemberDefinition @"
[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
"@ -ErrorAction SilentlyContinue | Out-Null
    $HWND_BROADCAST = [IntPtr]0xffff
    $WM_SETTINGCHANGE = 0x001a
    $result = [UIntPtr]::Zero
    [Win32.NativeMethods]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment", 2, 2000, [ref]$result) | Out-Null
} catch {}

# 4. Backend Toolchain Auto-Discovery & Provisioning
Write-Host "`n[4/5] Checking Native C/LLVM Backend Toolchain..." -ForegroundColor Green

$hasClang = (Get-Command clang++.exe -ErrorAction SilentlyContinue) -ne $null
$hasClangC = (Get-Command clang.exe -ErrorAction SilentlyContinue) -ne $null
$hasBundled = (Test-Path (Join-Path $ToolchainBin "clang++.exe")) -or (Test-Path (Join-Path $ToolchainBin "clang.exe"))

# Also check common standard installation paths
if (-not ($hasClang -or $hasClangC -or $hasBundled)) {
    $standardClangPaths = @(
        "C:\Program Files\LLVM\bin\clang++.exe",
        "C:\Program Files (x86)\LLVM\bin\clang++.exe",
        "C:\msys64\ucrt64\bin\clang++.exe",
        "C:\msys64\clang64\bin\clang++.exe",
        "D:\Program Files\LLVM\bin\clang++.exe"
    )
    foreach ($candClang in $standardClangPaths) {
        if (Test-Path $candClang) {
            $hasClang = $true
            $foundDir = Split-Path $candClang -Parent
            if ($env:Path -notlike "*$foundDir*") {
                $env:Path = "$foundDir;$env:Path"
            }
            break
        }
    }
}

if ($hasClang -or $hasClangC -or $hasBundled) {
    Write-Host "  -> Backend LLVM Clang toolchain is detected and ready." -ForegroundColor Green
} elseif (-not $SkipToolchain) {
    Write-Host "  -> No backend LLVM Clang compiler detected on this PC." -ForegroundColor Yellow
    Write-Host "     Auto-provisioning portable Windows toolchain for instant out-of-the-box native compilation..." -ForegroundColor Yellow
    
    $SpcExe = Join-Path $BinDir "spc.exe"
    if (Test-Path $SpcExe) {
        Write-Host "     Invoking 'spc setup'..." -ForegroundColor Cyan
        & $SpcExe setup
    } else {
        # Check if winget can install LLVM
        $hasWinget = (Get-Command winget.exe -ErrorAction SilentlyContinue) -ne $null
        if ($hasWinget) {
            try {
                Write-Host "     Installing LLVM Clang via Windows Package Manager (winget)..." -ForegroundColor Cyan
                winget install --id LLVM.LLVM -e --silent --accept-source-agreements --accept-package-agreements
            } catch {
                Write-Host "     winget installation deferred. You can run 'spc setup' anytime." -ForegroundColor DarkGray
            }
        } else {
            Write-Host "     Toolchain can be provisioned at any time by running 'spc setup'." -ForegroundColor DarkGray
        }
    }
}

# 5. Self-Diagnosis and Safe Smoke Test Execution
Write-Host "`n[5/5] Performing System Verification & Smoke Test..." -ForegroundColor Green

$SpcExe = Join-Path $BinDir "spc.exe"
if (Test-Path $SpcExe) {
    # Run spc doctor to display system health
    & $SpcExe doctor
    
    # Run smoke test in a safe temporary folder (never CWD which might be read-only)
    $tempSmokeSp = Join-Path $env:TEMP "sp_install_smoke_$([Guid]::NewGuid().ToString('N')).sp"
    $smokeCode = @"
import std.io;

fn main() {
    println("SP Native Compilation Verification: SUCCESS!");
}
"@
    [System.IO.File]::WriteAllText($tempSmokeSp, $smokeCode, [System.Text.Encoding]::ASCII)

    Write-Host "`n[*] Executing native smoke test..." -ForegroundColor Cyan
    try {
        & $SpcExe run $tempSmokeSp
    } catch {
        Write-Host "Notice: Smoke test invocation finished. Run 'spc run examples\hello.sp' to re-test." -ForegroundColor Gray
    } finally {
        Remove-Item -Path $tempSmokeSp -Force -ErrorAction SilentlyContinue
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
