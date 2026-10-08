# SP Language VS Code Extension — Marketplace Publisher & Packager
param(
    [string]$Pat = $env:VSCE_PAT,
    [switch]$PackageOnly
)

$ErrorActionPreference = "Stop"
$ExtensionDir = $PSScriptRoot
Set-Location $ExtensionDir

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   SP Language VS Code Extension — Visual Studio Marketplace   " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Compile TypeScript extension source
Write-Host "[1/4] Compiling TypeScript extension..." -ForegroundColor Yellow
if (Get-Command npm -ErrorAction SilentlyContinue) {
    npm run compile
} else {
    Write-Host "  npm not found; skipping compile step." -ForegroundColor Gray
}

# 2. Package VSIX
Write-Host "[2/4] Packaging VSIX extension archive..." -ForegroundColor Yellow
$VsixOutput = Join-Path $ExtensionDir "sp-language-support-1.0.0.vsix"
$DistVsix = Join-Path (Join-Path (Split-Path $ExtensionDir -Parent) "..") "dist\sp-vscode.vsix"

if (Get-Command npx -ErrorAction SilentlyContinue) {
    npx @vscode/vsce package --no-dependencies -o $VsixOutput --allow-missing-repository
    Copy-Item $VsixOutput $DistVsix -Force
    Write-Host "  Created VSIX package: $VsixOutput" -ForegroundColor Green
    Write-Host "  Mirrored to: $DistVsix" -ForegroundColor Green
} else {
    Write-Host "  Using pre-packaged VSIX in dist/." -ForegroundColor Gray
}

if ($PackageOnly) {
    Write-Host "`nExtension packaging complete! Package ready at: $DistVsix" -ForegroundColor Green
    exit 0
}

# 3. Publish to Marketplace
Write-Host "[3/4] Publishing to Visual Studio Marketplace..." -ForegroundColor Yellow
if ([string]::IsNullOrWhiteSpace($Pat)) {
    Write-Host "  Note: VSCE_PAT (Personal Access Token) not provided." -ForegroundColor Yellow
    Write-Host "  To publish live to the Visual Studio Marketplace:" -ForegroundColor White
    Write-Host "      .\publish-to-marketplace.ps1 -Pat <your-azure-devops-pat>" -ForegroundColor Cyan
    Write-Host "      or set `$env:VSCE_PAT = '<token>'" -ForegroundColor Cyan
    Write-Host "`nPackage distribution artifact is ready for manual upload at https://marketplace.visualstudio.com/manage" -ForegroundColor Green
    exit 0
}

Write-Host "  Publishing extension under publisher 'shantopaul'..." -ForegroundColor Cyan
npx @vscode/vsce publish -p $Pat --packagePath $VsixOutput
Write-Host "[4/4] Extension published successfully to Visual Studio Marketplace!" -ForegroundColor Green
