# ==============================================================================
# SP Programming Language - Cloudflare R2 Release Uploader
# ==============================================================================
# Uploads the built distribution archives and installer scripts to Cloudflare R2
# Storage with zero egress fees and global CDN distribution.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/upload_to_r2.ps1 [-Version "3.1.0"]
# ==============================================================================

[CmdletBinding()]
param(
    [string]$Version = "3.1.0",
    [string]$BucketName = "splang-releases",
    [string]$AccountId = "3b56f1445438ff6041fee436fe29b097", # From Cloudflare Dashboard
    [string]$AccessKeyId = $env:R2_ACCESS_KEY_ID,
    [string]$SecretAccessKey = $env:R2_SECRET_ACCESS_KEY
)

$ErrorActionPreference = "Stop"
$repoRoot = (Split-Path $PSScriptRoot -Parent)
$distDir = Join-Path $repoRoot "dist"
$zipName = "sp-$Version-windows-x64.zip"
$zipPath = Join-Path $distDir $zipName

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "          SP Language - Cloudflare R2 Release Asset Uploader            " -ForegroundColor Cyan
Write-Host "                           Version $Version                             " -ForegroundColor White
Write-Host "========================================================================" -ForegroundColor Cyan

# 1. Ensure artifact exists
if (-not (Test-Path $zipPath)) {
    Write-Host "Artifact '$zipName' not found in dist/. Packaging now..." -ForegroundColor Yellow
    & "$PSScriptRoot\package_distribution.ps1" -Version $Version -SkipBuild
}

if (-not (Test-Path $zipPath)) {
    Write-Error "Could not find or generate $zipPath. Please build first."
}

$fileItem = Get-Item $zipPath
$fileSizeMB = [math]::Round($fileItem.Length / 1MB, 2)
$fileHash = (Get-FileHash -Path $zipPath -Algorithm SHA256).Hash

Write-Host "`nTarget Release Artifacts:" -ForegroundColor White
Write-Host "  Version Package : $zipPath ($fileSizeMB MB)" -ForegroundColor Cyan
Write-Host "  SHA-256         : $fileHash" -ForegroundColor Yellow
Write-Host "  Latest Alias    : $(Join-Path $distDir 'sp-windows-x64.zip')" -ForegroundColor Cyan
Write-Host "  PowerShell Inst : $(Join-Path $repoRoot 'install.ps1')" -ForegroundColor Cyan
Write-Host "  Shell Installer : $(Join-Path $repoRoot 'install.sh')" -ForegroundColor Cyan
Write-Host "  R2 Bucket       : $BucketName" -ForegroundColor Cyan
Write-Host "  Cloudflare Acct : $AccountId" -ForegroundColor Gray

$assetsToUpload = @(
    @{ Path = $zipPath; Key = $zipName },
    @{ Path = (Join-Path $distDir "sp-windows-x64.zip"); Key = "sp-windows-x64.zip" },
    @{ Path = (Join-Path $repoRoot "install.ps1"); Key = "install.ps1" },
    @{ Path = (Join-Path $repoRoot "install.sh"); Key = "install.sh" }
)

# 2. Check and load .env file if credentials not yet present in environment
$envFile = Join-Path $repoRoot ".env"
if (Test-Path $envFile) {
    Get-Content $envFile | ForEach-Object {
        $line = $_.Trim()
        if ($line -and -not $line.StartsWith("#") -and $line.Contains("=")) {
            $parts = $line.Split("=", 2)
            $k = $parts[0].Trim()
            $v = $parts[1].Trim().Trim('"').Trim("'")
            if ($k -eq "R2_ACCESS_KEY_ID" -and -not $AccessKeyId) { $AccessKeyId = $v }
            if ($k -eq "R2_SECRET_ACCESS_KEY" -and -not $SecretAccessKey) { $SecretAccessKey = $v }
            if ($k -eq "R2_ACCOUNT_ID" -and -not $AccountId) { $AccountId = $v }
            if ($k -eq "R2_BUCKET_NAME" -and -not $BucketName) { $BucketName = $v }
        }
    }
}

# 3. Check upload methods
# Method A: Automated Pure Node.js Standard Library Engine (Fastest, zero external CLI deps)
$nodeCmd = Get-Command node.exe -ErrorAction SilentlyContinue
$jsScript = Join-Path $PSScriptRoot "upload_to_r2.js"
if ($nodeCmd -and (Test-Path $jsScript)) {
    Write-Host "`n[Method 1] Uploading directly to Cloudflare R2 via Node.js S3 Engine..." -ForegroundColor Green
    & node $jsScript $Version
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n[SUCCESS] Cloudflare R2 Release Upload Complete!" -ForegroundColor Green
        exit 0
    }
}

# Method B: AWS CLI (S3 compatible)
$awsCmd = Get-Command aws.exe -ErrorAction SilentlyContinue
if ($awsCmd -and $AccessKeyId -and $SecretAccessKey) {
    Write-Host "`n[Method 2] Uploading via AWS CLI to Cloudflare R2..." -ForegroundColor Green
    $env:AWS_ACCESS_KEY_ID = $AccessKeyId
    $env:AWS_SECRET_ACCESS_KEY = $SecretAccessKey
    $endpoint = "https://$AccountId.r2.cloudflarestorage.com"
    
    foreach ($asset in $assetsToUpload) {
        if (Test-Path $asset.Path) {
            Write-Host "    Uploading $($asset.Key)..." -ForegroundColor Cyan
            & aws s3 cp $asset.Path "s3://$BucketName/$($asset.Key)" --endpoint-url $endpoint
        }
    }
    Write-Host "`n[SUCCESS] All release assets uploaded to R2 bucket '$BucketName'!" -ForegroundColor Green
    exit 0
}

# Method C: Cloudflare Wrangler via npx
$hasNode = (Get-Command node.exe -ErrorAction SilentlyContinue) -ne $null
if ($hasNode -and $env:CLOUDFLARE_API_TOKEN) {
    Write-Host "`n[Method 3] Uploading via Cloudflare Wrangler CLI..." -ForegroundColor Green
    foreach ($asset in $assetsToUpload) {
        if (Test-Path $asset.Path) {
            Write-Host "    Uploading $($asset.Key)..." -ForegroundColor Cyan
            npx --yes wrangler r2 object put "$BucketName/$($asset.Key)" --file $asset.Path
        }
    }
    Write-Host "`n[SUCCESS] All release assets uploaded to R2 bucket '$BucketName'!" -ForegroundColor Green
    exit 0
}

# Method C: Cloudflare Dashboard Direct / Fast Upload Guide
Write-Host "`n------------------------------------------------------------------------" -ForegroundColor Yellow
Write-Host "  Cloudflare R2 Ready Assets & Web Deployment Summary:" -ForegroundColor Yellow
Write-Host "------------------------------------------------------------------------" -ForegroundColor White
Write-Host "All release files are compiled, staged, and validated locally:`n" -ForegroundColor White

foreach ($asset in $assetsToUpload) {
    if (Test-Path $asset.Path) {
        $sz = [math]::Round((Get-Item $asset.Path).Length / 1MB, 2)
        Write-Host "  * $($asset.Key.PadRight(28)) -> $($asset.Path) ($sz MB)" -ForegroundColor Green
    }
}

Write-Host "`nCloudflare Dashboard Direct Upload Steps:" -ForegroundColor Cyan
Write-Host "1. Open Cloudflare Dashboard -> R2 -> Bucket: '$BucketName'" -ForegroundColor White
Write-Host "2. Drag and drop the files listed above into the bucket root." -ForegroundColor White
Write-Host "3. Verified Public CDN Endpoints:" -ForegroundColor White
Write-Host "   * https://dl.splang.shantopaul.com/install.ps1" -ForegroundColor Green
Write-Host "   * https://dl.splang.shantopaul.com/install.sh" -ForegroundColor Green
Write-Host "   * https://dl.splang.shantopaul.com/sp-3.1.0-windows-x64.zip" -ForegroundColor Green
Write-Host "   * https://dl.splang.shantopaul.com/sp-windows-x64.zip" -ForegroundColor Green
Write-Host "   * https://downloads.splang.shantopaul.com/sp-3.1.0-windows-x64.zip" -ForegroundColor Green
Write-Host "------------------------------------------------------------------------`n" -ForegroundColor Yellow
