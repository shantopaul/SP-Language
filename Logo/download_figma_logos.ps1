<#
.SYNOPSIS
    Downloads all high-quality logo assets (SVG & 4x PNG) from Figma file 'SP-Lang-All-Logo'
#>
param(
    [string]$Token,
    [string]$FileUrlOrKey = "iB4qHJ5bj642RvqtfzOwC5"
)

$OutputDir = $PSScriptRoot

# 1. Resolve Token
if (-not $Token) {
    if ($env:FIGMA_TOKEN) {
        $Token = $env:FIGMA_TOKEN
    } elseif ($env:FIGMA_PERSONAL_ACCESS_TOKEN) {
        $Token = $env:FIGMA_PERSONAL_ACCESS_TOKEN
    } elseif (Test-Path "$OutputDir\.env") {
        Get-Content "$OutputDir\.env" | ForEach-Object {
            if ($_ -match "FIGMA_TOKEN=(.+)") { $Token = $matches[1].Trim() }
            if ($_ -match "FIGMA_PERSONAL_ACCESS_TOKEN=(.+)") { $Token = $matches[1].Trim() }
        }
    }
}

if (-not $Token) {
    $Token = Read-Host "Please enter your Figma Personal Access Token (figd_...)"
}

if (-not $Token) {
    Write-Error "A Figma token is required to download assets via API."
    exit 1
}

# 2. Extract File Key
$FileKey = $FileUrlOrKey
if ($FileUrlOrKey -match "figma\.com/(?:design|file)/([^/?]+)") {
    $FileKey = $matches[1]
}

$Headers = @{
    "X-Figma-Token" = $Token
}

Write-Host "Connecting to Figma API for file key: $FileKey ..." -ForegroundColor Cyan

try {
    $fileData = Invoke-RestMethod -Uri "https://api.figma.com/v1/files/$FileKey" -Headers $Headers -Method Get
} catch {
    Write-Host "`nCould not access file directly via API (Error: $_)" -ForegroundColor Red
    Write-Host "`nReasons & Solutions:" -ForegroundColor Yellow
    Write-Host "1. File in Drafts: In Figma, click 'Share' (top right) -> Set to 'Anyone with the link can view'." -ForegroundColor White
    Write-Host "2. Token Scopes: Ensure token has 'files:read' or 'File content: Read' permission." -ForegroundColor White
    Write-Host "3. Alternatively, export directly: Select all in Figma -> Export (SVG & PNG 4x) -> Run .\import_from_downloads.ps1" -ForegroundColor Cyan
    exit 1
}

$nodes = @()
foreach ($page in $fileData.document.children) {
    foreach ($child in $page.children) {
        $nodes += [PSCustomObject]@{
            Id   = $child.id
            Name = $child.name
        }
    }
}

if ($nodes.Count -eq 0) {
    Write-Warning "No top-level frames found in the document."
    exit 0
}

Write-Host "Found $($nodes.Count) logo frames/components:" -ForegroundColor Green
$nodes | ForEach-Object { Write-Host " - $($_.Name) (ID: $($_.Id))" }

$nodeIds = ($nodes | ForEach-Object { $_.Id }) -join ","

# Download SVGs
Write-Host "`n[1/2] Fetching SVG image render links..." -ForegroundColor Cyan
$svgUrls = Invoke-RestMethod -Uri "https://api.figma.com/v1/images/$FileKey?ids=$nodeIds&format=svg" -Headers $Headers -Method Get

# Download 4x PNGs (High Quality)
Write-Host "[2/2] Fetching 4x High-Resolution PNG image render links..." -ForegroundColor Cyan
$pngUrls = Invoke-RestMethod -Uri "https://api.figma.com/v1/images/$FileKey?ids=$nodeIds&format=png&scale=4" -Headers $Headers -Method Get

foreach ($node in $nodes) {
    $safeName = $node.Name -replace '[\\/:*?"<>|]', '_'
    
    # SVG Download
    $sUrl = $svgUrls.images.$($node.Id)
    if ($sUrl) {
        $svgPath = Join-Path $OutputDir "$safeName.svg"
        Write-Host "Downloading $safeName.svg ..." -ForegroundColor Yellow
        Invoke-WebRequest -Uri $sUrl -OutFile $svgPath
    }

    # 4x PNG Download
    $pUrl = $pngUrls.images.$($node.Id)
    if ($pUrl) {
        $pngPath = Join-Path $OutputDir "$safeName@4x.png"
        Write-Host "Downloading $safeName@4x.png ..." -ForegroundColor Yellow
        Invoke-WebRequest -Uri $pUrl -OutFile $pngPath
    }
}

Write-Host "`nAll high quality logos successfully downloaded into: $OutputDir" -ForegroundColor Green
