# ==============================================================================
# SP Programming Language - Cloudflare R2 Release Uploader
# ==============================================================================
# Uploads the built distribution archive (dist/sp-*.zip) to Cloudflare R2
# Storage with zero egress fees and global CDN distribution.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/upload_to_r2.ps1 [-Version "1.0.0"]
# ==============================================================================

[CmdletBinding()]
param(
    [string]$Version = "1.0.0",
    [string]$BucketName = "splang-releases",
    [string]$AccountId = "3b56f1445438ff6041fee436fe29b097", # From Cloudflare Dashboard
    [string]$AccessKeyId = $env:R2_ACCESS_KEY_ID,
    [string]$SecretAccessKey = $env:R2_SECRET_ACCESS_KEY
)

$ErrorActionPreference = "Stop"
$repoRoot = (Split-Path $PSScriptRoot -Parent)
$zipName = "sp-$Version-windows-x64.zip"
$zipPath = Join-Path $repoRoot "dist\$zipName"

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "          SP Language - Cloudflare R2 Release Asset Uploader            " -ForegroundColor Cyan
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

Write-Host "`nTarget File : $zipPath" -ForegroundColor White
Write-Host "File Size   : $fileSizeMB MB" -ForegroundColor White
Write-Host "SHA-256     : $fileHash" -ForegroundColor Yellow
Write-Host "R2 Bucket   : $BucketName" -ForegroundColor Cyan
Write-Host "Account ID  : $AccountId" -ForegroundColor Gray

# 2. Check upload methods
# Method A: AWS CLI (if installed)
$awsCmd = Get-Command aws -ErrorAction SilentlyContinue
if ($awsCmd -and $AccessKeyId -and $SecretAccessKey) {
    Write-Host "`n[A] Uploading via AWS CLI to Cloudflare R2..." -ForegroundColor Green
    $env:AWS_ACCESS_KEY_ID = $AccessKeyId
    $env:AWS_SECRET_ACCESS_KEY = $SecretAccessKey
    $endpoint = "https://$AccountId.r2.cloudflarestorage.com"
    
    & aws s3 cp $zipPath "s3://$BucketName/$zipName" --endpoint-url $endpoint
    Write-Host "`n[SUCCESS] File uploaded to R2 bucket '$BucketName'!" -ForegroundColor Green
    exit 0
}

# Method B: Wrangler CLI (if installed)
$wranglerCmd = Get-Command wrangler -ErrorAction SilentlyContinue
if ($wranglerCmd) {
    Write-Host "`n[B] Uploading via Cloudflare Wrangler..." -ForegroundColor Green
    & wrangler r2 object put "$BucketName/$zipName" --file $zipPath
    Write-Host "`n[SUCCESS] File uploaded to R2 bucket '$BucketName'!" -ForegroundColor Green
    exit 0
}

# Method C: Cloudflare Dashboard Manual / Browser Upload Instructions
Write-Host "`n------------------------------------------------------------------------" -ForegroundColor Yellow
Write-Host "  R2 Upload Ready (Cloudflare Dashboard Drag & Drop):" -ForegroundColor Yellow
Write-Host "------------------------------------------------------------------------" -ForegroundColor White
Write-Host "1. In your Cloudflare Dashboard (shown in your screenshot):" -ForegroundColor White
Write-Host "   Click '+ Create bucket' -> Name: '$BucketName'" -ForegroundColor Cyan
Write-Host "2. In Settings -> 'Public access':" -ForegroundColor White
Write-Host "   - Click 'Connect Domain' and add: downloads.splang.shantopaul.com" -ForegroundColor Cyan
Write-Host "   - (Or enable the 'r2.dev subdomain' for immediate public link)" -ForegroundColor Cyan
Write-Host "3. In Objects tab:" -ForegroundColor White
Write-Host "   - Drag & drop this file: $zipPath" -ForegroundColor Green
Write-Host ""
Write-Host "Public download URLs will be:" -ForegroundColor White
Write-Host "   https://downloads.splang.shantopaul.com/$zipName" -ForegroundColor Green
Write-Host "   https://splang.shantopaul.com/downloads/$zipName" -ForegroundColor Green
Write-Host "------------------------------------------------------------------------`n" -ForegroundColor Yellow
