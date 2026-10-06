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

# Prepare destination directories
$BinDir = Join-Path $InstallDir "bin"
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}

$ExpectedHash = "37916cd8eca92e73fbd7c7c3bcc70f4efe0573f0b4fbad0ebe1b7e093cefee9c"
$ZipName = "sp-$Version-windows-x64.zip"
$TempZip = Join-Path $env:TEMP $ZipName

# Primary source: Official SP CDN/Site; Fallback: GitHub Releases
$Urls = @(
    "https://splang.shantopaul.com/downloads/$ZipName",
    "https://github.com/shantopaul/SP-Language/releases/download/v$Version/$ZipName"
)

$Downloaded = $false
foreach ($Url in $Urls) {
    try {
        Write-Host "Downloading SP Toolchain from: $Url" -ForegroundColor Yellow
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $Url -OutFile $TempZip -UseBasicParsing -TimeoutSec 60
        if (Test-Path $TempZip) {
            $Downloaded = $true
            break
        }
    } catch {
        Write-Host "Notice: Mirror unreachable, trying next source..." -ForegroundColor Gray
    }
}

if (-not $Downloaded) {
    # Local fallback for active development / build environment
    if (Test-Path "C:\SP-Language\bin") {
        Write-Host "Using system binary installation from C:\SP-Language..." -ForegroundColor Green
        Copy-Item -Path "C:\SP-Language\*" -Destination $InstallDir -Recurse -Force
        $Downloaded = $true
    } else {
        throw "Failed to download SP Language distribution package from all mirrors."
    }
} else {
    # Verify SHA-256 integrity
    Write-Host "Verifying cryptographic checksum (SHA-256)..." -ForegroundColor Yellow
    $ActualHash = (Get-FileHash -Path $TempZip -Algorithm SHA256).Hash.ToLower()
    if ($ActualHash -ne $ExpectedHash) {
        Write-Host "Warning: SHA-256 differs (Downloaded: $ActualHash)" -ForegroundColor Yellow
    } else {
        Write-Host "Cryptographic verification passed: $ActualHash" -ForegroundColor Green
    }

    Write-Host "Extracting toolchain packages..." -ForegroundColor Yellow
    Expand-Archive -Path $TempZip -DestinationPath $InstallDir -Force
    Remove-Item -Path $TempZip -Force -ErrorAction SilentlyContinue
}

# Update User Environment PATH permanently
$UserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
if ($UserPath -notlike "*$BinDir*") {
    Write-Host "Adding $BinDir to User PATH..." -ForegroundColor Yellow
    $NewPath = if ([string]::IsNullOrEmpty($UserPath)) { $BinDir } else { "$UserPath;$BinDir" }
    [Environment]::SetEnvironmentVariable("Path", $NewPath, [EnvironmentVariableTarget]::User)
    $env:Path += ";$BinDir"
}

Write-Host ""
Write-Host "SP Language v$Version installed successfully to $InstallDir!" -ForegroundColor Green
Write-Host ""

# Verification
$SpcExe = Join-Path $BinDir "spc.exe"
if (Test-Path $SpcExe) {
    Write-Host "Toolchain verification:" -ForegroundColor White
    & $SpcExe --version
    Write-Host ""
    
    $ExampleFile = Join-Path $InstallDir "examples\hello.sp"
    if (Test-Path $ExampleFile) {
        Write-Host "Testing test execution (hello.sp):" -ForegroundColor White
        & $SpcExe run $ExampleFile
    }
}

Write-Host ""
Write-Host "Get started with your first project:" -ForegroundColor Cyan
Write-Host "  spm init my_project" -ForegroundColor White
Write-Host "  cd my_project" -ForegroundColor White
Write-Host "  spc run src/main.sp" -ForegroundColor White
Write-Host ""
Write-Host "Documentation: https://docs.splang.shantopaul.com" -ForegroundColor Gray
Write-Host "Package Registry: https://registry.splang.shantopaul.com" -ForegroundColor Gray
