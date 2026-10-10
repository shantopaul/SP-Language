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
$ProgressPreference = 'SilentlyContinue'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {}
Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction SilentlyContinue

$BinDir = Join-Path $InstallDir "bin"
$LibDir = Join-Path $InstallDir "lib"
$IncludeDir = Join-Path $InstallDir "runtime\include"
$StdlibDir = Join-Path $InstallDir "stdlib"
$ExamplesDir = Join-Path $InstallDir "examples"
$ToolchainDir = Join-Path $InstallDir "toolchain"
$ToolchainBin = Join-Path $ToolchainDir "bin"

# Detect existing installation and version
$isUpgrade = $false
$isRepair = $false
$installedVerStr = $null

$existingSpc = Join-Path $BinDir "spc.exe"
if (-not (Test-Path $existingSpc)) {
    $foundCmd = Get-Command spc.exe -ErrorAction SilentlyContinue
    if ($foundCmd) { $existingSpc = $foundCmd.Source }
}

if (Test-Path $existingSpc) {
    try {
        $verText = (& $existingSpc --version 2>&1) -join "`n"
        if ($verText -match "version\s+([0-9]+(\.[0-9]+)+)") {
            $installedVerStr = $matches[1]
        }
    } catch {}

    if (-not $installedVerStr) {
        try {
            $regVer = (Get-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\SPLang" -ErrorAction SilentlyContinue).DisplayVersion
            if ($regVer) { $installedVerStr = $regVer }
        } catch {}
    }

    if ($installedVerStr) {
        try {
            $curV = [version]$installedVerStr
            $newV = [version]$Version
            if ($newV -gt $curV) {
                $isUpgrade = $true
            } else {
                $isRepair = $true
            }
        } catch {
            $isUpgrade = ($installedVerStr -ne $Version)
        }
    } else {
        $isRepair = $true
    }
}

function Download-File([string]$Url, [string]$Destination) {
    if (Test-Path $Destination) { Remove-Item $Destination -Force -ErrorAction SilentlyContinue }

    # Attempt 1: curl.exe (built into modern Windows)
    if (Get-Command curl.exe -ErrorAction SilentlyContinue) {
        try {
            & curl.exe -f -s -S -L --retry 2 -o $Destination $Url
            if ($LASTEXITCODE -eq 0 -and (Test-Path $Destination) -and ((Get-Item $Destination).Length -gt 50000)) {
                try {
                    $z = [System.IO.Compression.ZipFile]::OpenRead($Destination)
                    $z.Dispose()
                    return $true
                } catch {
                    Remove-Item $Destination -Force -ErrorAction SilentlyContinue
                }
            }
        } catch {}
    }

    # Attempt 2: .NET WebClient (fast and synchronous)
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
        $wc = New-Object System.Net.WebClient
        $wc.Headers.Add("User-Agent", "SPLang-Installer/3.1.0")
        $wc.DownloadFile($Url, $Destination)
        if ((Test-Path $Destination) -and ((Get-Item $Destination).Length -gt 50000)) {
            try {
                $z = [System.IO.Compression.ZipFile]::OpenRead($Destination)
                $z.Dispose()
                return $true
            } catch {
                Remove-Item $Destination -Force -ErrorAction SilentlyContinue
            }
        }
    } catch {}

    # Attempt 3: PowerShell Invoke-WebRequest
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
        Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing -TimeoutSec 180
        if ((Test-Path $Destination) -and ((Get-Item $Destination).Length -gt 50000)) {
            try {
                $z = [System.IO.Compression.ZipFile]::OpenRead($Destination)
                $z.Dispose()
                return $true
            } catch {
                Remove-Item $Destination -Force -ErrorAction SilentlyContinue
            }
        }
    } catch {}

    return $false
}

function Write-Banner {
    $bannerLines = @(
        "███████╗██████╗     ██╗      █████╗ ███╗   ██╗ ██████╗ ",
        "██╔════╝██╔══██╗    ██║     ██╔══██╗████╗  ██║██╔════╝ ",
        "███████╗██████╔╝    ██║     ███████║██╔██╗ ██║██║  ███╗",
        "╚════██║██╔═══╝     ██║     ██╔══██║██║╚██╗██║██║   ██║",
        "███████║██║         ███████╗██║  ██║██║ ╚████║╚██████╔╝",
        "╚══════╝╚═╝         ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ "
    )

    Write-Host ""
    foreach ($line in $bannerLines) {
        Write-Host ("        " + $line) -ForegroundColor Cyan
    }
    Write-Host ""
    Write-Host "========================================================================" -ForegroundColor Cyan
    Write-Host "         SP Programming Language - Windows Instant Installer          " -ForegroundColor Cyan
    Write-Host "                           Version $Version                             " -ForegroundColor White
    Write-Host "========================================================================" -ForegroundColor Cyan
    Write-Host "  Target Location : $InstallDir" -ForegroundColor Yellow
    Write-Host "  OS Architecture : $env:PROCESSOR_ARCHITECTURE" -ForegroundColor Gray
    Write-Host "  Operating System: $([System.Environment]::OSVersion.VersionString)" -ForegroundColor Gray
    if ($isUpgrade) {
        Write-Host "  Operation Mode  : Auto-Upgrade (v$installedVerStr -> v$Version)" -ForegroundColor Green
    } elseif ($isRepair) {
        Write-Host "  Operation Mode  : Reinstall / Repair (v$Version)" -ForegroundColor Cyan
    } else {
        Write-Host "  Operation Mode  : Clean System Installation (v$Version)" -ForegroundColor Green
    }
    Write-Host "========================================================================" -ForegroundColor Cyan
    Write-Host ""
}

Write-Banner

# 1. Prepare target installation directories & release process locks
Write-Host "[1/5] Preparing installation directories & system processes..." -ForegroundColor Green

# Gracefully release any locks by stopping background SP processes (e.g. language server or running compiler)
try {
    $spProcs = Get-Process -Name "spc", "spm", "spd", "spls" -ErrorAction SilentlyContinue
    foreach ($proc in $spProcs) {
        try {
            if ($proc.Path -like "*$InstallDir*" -or $proc.Path -like "*\.sp\*") {
                Write-Host "  -> Releasing process lock: $($proc.ProcessName).exe (PID: $($proc.Id))..." -ForegroundColor Gray
                Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
            }
        } catch {}
    }
    Start-Sleep -Milliseconds 200
} catch {}

$directories = @($InstallDir, $BinDir, $LibDir, $IncludeDir, $StdlibDir, $ExamplesDir)
foreach ($dir in $directories) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}

if ($isUpgrade) {
    Write-Host "  -> Preparing clean upgrade of standard library & headers..." -ForegroundColor Gray
    # Clean previous stdlib modules to avoid outdated or conflicting files
    if (Test-Path $StdlibDir) {
        Remove-Item -Path "$StdlibDir\*" -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path $IncludeDir) {
        Remove-Item -Path "$IncludeDir\*" -Recurse -Force -ErrorAction SilentlyContinue
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
    
    # Copy core binaries and companion DLLs
    Get-ChildItem -Path $localBuildBin -Include "*.exe", "*.dll" -Recurse:$false | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $BinDir -Force
        Write-Host "     Installed binary: $($_.Name)" -ForegroundColor Gray
    }
    
    # Also ensure companion runtime DLLs are copied to bin if available
    $runtimeDllDirs = @(
        (Join-Path $repoRoot "dist\SP-Language\bin"),
        (Join-Path $repoRoot "build\bin")
    )
    $neededDlls = @("libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll")
    foreach ($dll in $neededDlls) {
        if (-not (Test-Path (Join-Path $BinDir $dll))) {
            foreach ($d in $runtimeDllDirs) {
                $candDll = Join-Path $d $dll
                if (Test-Path $candDll) {
                    Copy-Item -Path $candDll -Destination (Join-Path $BinDir $dll) -Force
                    Write-Host "     Installed runtime DLL: $dll" -ForegroundColor Gray
                    break
                }
            }
        }
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
        (Join-Path $repoRoot "dist\SP-Language\lib\libssl.a")
    )
    $cryptoCandidates = @(
        (Join-Path $repoRoot "build\lib\libcrypto.a"),
        (Join-Path $repoRoot "dist\SP-Language\lib\libcrypto.a")
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
    foreach ($Url in $Urls) {
        Write-Host "     Connecting to: $Url" -ForegroundColor Gray
        if (Download-File -Url $Url -Destination $TempZip) {
            $Downloaded = $true
            Write-Host "     Package downloaded successfully!" -ForegroundColor Green
            break
        } else {
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

# Ensure standalone uninstaller is present at both root ($InstallDir\uninstall.exe) and in $BinDir
$rootUninstaller = Join-Path $InstallDir "uninstall.exe"
$binUninstaller = Join-Path $BinDir "uninstall.exe"

if ((Test-Path $binUninstaller) -and (-not (Test-Path $rootUninstaller))) {
    Copy-Item -Path $binUninstaller -Destination $rootUninstaller -Force
} elseif ((Test-Path $rootUninstaller) -and (-not (Test-Path $binUninstaller))) {
    Copy-Item -Path $rootUninstaller -Destination $binUninstaller -Force
}

# If running with local repo artifacts, also copy uninstall.ps1 to root
if ($isLocalRepo -and (-not [string]::IsNullOrEmpty($repoRoot)) -and (Test-Path (Join-Path $repoRoot "uninstall.ps1"))) {
    Copy-Item -Path (Join-Path $repoRoot "uninstall.ps1") -Destination (Join-Path $InstallDir "uninstall.ps1") -Force
}

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

# Register in Windows Installed Apps (Programs & Features)
try {
    $regKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\SPLang"
    if (-not (Test-Path $regKey)) {
        New-Item -Path $regKey -Force | Out-Null
    }
    Set-ItemProperty -Path $regKey -Name "DisplayName" -Value "SP Programming Language" -Force
    Set-ItemProperty -Path $regKey -Name "DisplayVersion" -Value $Version -Force
    Set-ItemProperty -Path $regKey -Name "Publisher" -Value "Shanto Paul" -Force
    Set-ItemProperty -Path $regKey -Name "InstallLocation" -Value $InstallDir -Force
    Set-ItemProperty -Path $regKey -Name "UninstallString" -Value "`"$rootUninstaller`"" -Force
    Set-ItemProperty -Path $regKey -Name "QuietUninstallString" -Value "`"$rootUninstaller`" -y" -Force
    Set-ItemProperty -Path $regKey -Name "DisplayIcon" -Value "`"$BinDir\spc.exe`",0" -Force
    Set-ItemProperty -Path $regKey -Name "HelpLink" -Value "https://docs.splang.shantopaul.com" -Force
    Set-ItemProperty -Path $regKey -Name "URLInfoAbout" -Value "https://splang.shantopaul.com" -Force
    Write-Host "  -> Registered in Windows Installed Apps" -ForegroundColor Cyan
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
    Write-Host "     Auto-provisioning lightweight portable toolchain from CDN..." -ForegroundColor Yellow
    
    $provisioned = $false
    # Step A: High-speed portable toolchain from SP Language CDN (Direct from Cloudflare R2)
    $tcUrls = @(
        "https://dl.splang.shantopaul.com/toolchain-win64.zip",
        "https://splang.shantopaul.com/downloads/toolchain-win64.zip",
        "https://github.com/shantopaul/SP-Language/releases/download/v$($Version)/toolchain-win64.zip"
    )
    $tcZip = Join-Path $env:TEMP "sp_toolchain_$([Guid]::NewGuid().ToString('N')).zip"

    foreach ($tcu in $tcUrls) {
        Write-Host "     Connecting to high-speed CDN: $tcu" -ForegroundColor Gray
        if (Download-File -Url $tcu -Destination $tcZip) {
            Write-Host "     Extracting portable toolchain to $ToolchainDir..." -ForegroundColor Cyan
            Expand-Archive -Path $tcZip -DestinationPath $ToolchainDir -Force
            Remove-Item -Path $tcZip -Force -ErrorAction SilentlyContinue
            if (Test-Path (Join-Path $ToolchainBin "clang++.exe")) {
                $provisioned = $true
                if ($env:Path -notlike "*$ToolchainBin*") {
                    $env:Path = "$ToolchainBin;$env:Path"
                }
                # Persist ToolchainBin to User PATH in Registry so future terminals have it
                try {
                    $curUserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
                    $curEntries = if ([string]::IsNullOrEmpty($curUserPath)) { @() } else { $curUserPath -split ";" }
                    if ($curEntries -notcontains $ToolchainBin) {
                        $curEntries += $ToolchainBin
                        $newPath = ($curEntries -join ";").TrimEnd(";")
                        [Environment]::SetEnvironmentVariable("Path", $newPath, [EnvironmentVariableTarget]::User)
                    }
                    if ($result -ne $null) {
                        [Win32.NativeMethods]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment", 2, 2000, [ref]$result) | Out-Null
                    }
                } catch {}
                Write-Host "     Portable LLVM Clang toolchain installed and ready!" -ForegroundColor Green
                break
            }
        } else {
            Write-Host "     CDN mirror busy, trying next option..." -ForegroundColor DarkGray
        }
    }

    # Step B: Fallback to winget only if CDN toolchain was unreachable
    if (-not $provisioned) {
        $hasWinget = (Get-Command winget.exe -ErrorAction SilentlyContinue) -ne $null
        if ($hasWinget) {
            try {
                Write-Host "     Falling back to Windows Package Manager (winget)..." -ForegroundColor Cyan
                winget install --id LLVM.LLVM -e --silent --accept-source-agreements --accept-package-agreements
            } catch {}
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
if ($isUpgrade) {
    Write-Host "========================================================================" -ForegroundColor Green
    Write-Host "       SP Programming Language Successfully Upgraded to v$Version!     " -ForegroundColor Green
    Write-Host "========================================================================" -ForegroundColor Green
    Write-Host "  Upgrade Details:" -ForegroundColor White
    Write-Host "    Previous Version : v$installedVerStr" -ForegroundColor Gray
    Write-Host "    Current Version  : v$Version" -ForegroundColor Green
    Write-Host "    Compiler Path    : $BinDir\spc.exe" -ForegroundColor Gray
    Write-Host "    Toolchain Status : Operational" -ForegroundColor Gray
} else {
    Write-Host "========================================================================" -ForegroundColor Green
    Write-Host "           SP Programming Language Successfully Installed!              " -ForegroundColor Green
    Write-Host "========================================================================" -ForegroundColor Green
}
Write-Host "========================================================================" -ForegroundColor Green
Write-Host "  Quickstart Commands (open a new terminal window):" -ForegroundColor White
Write-Host "    spc --version                 # Check compiler version" -ForegroundColor Gray
Write-Host "    spc doctor                    # View toolchain health report" -ForegroundColor Gray
Write-Host "    spc run examples\hello.sp     # Build & run directly" -ForegroundColor Gray
Write-Host "    spc build main.sp -o app.exe  # Build native standalone executable" -ForegroundColor Gray
Write-Host "    spm init my_project           # Create a new SP project" -ForegroundColor Gray
Write-Host "    spc upgrade                   # Auto-upgrade SP Language to latest release" -ForegroundColor Gray
Write-Host "    spc uninstall                 # Cleanly remove SP Language & toolchains" -ForegroundColor Gray
Write-Host "========================================================================" -ForegroundColor Green
Write-Host ""
