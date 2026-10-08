<#
.SYNOPSIS
    Automatically finds exported Figma logo files or zips in Downloads and extracts/moves them to Logo/
#>

$DownloadsDir = "C:\Users\Shanto Paul\Downloads"
$LogoDir = $PSScriptRoot

Write-Host "Scanning Downloads folder for exported Figma logos..." -ForegroundColor Cyan

# Check for zip archives
$zipFiles = Get-ChildItem -Path $DownloadsDir -Filter "*.zip" | Where-Object {
    $_.Name -match "SP-Lang" -or $_.Name -match "logo" -or $_.LastWriteTime -gt (Get-Date).AddHours(-2)
}

$extracted = $false
foreach ($zip in $zipFiles) {
    Write-Host "Found potential archive: $($zip.FullName)" -ForegroundColor Yellow
    try {
        Expand-Archive -Path $zip.FullName -DestinationPath $LogoDir -Force
        Write-Host "Extracted $($zip.Name) directly into $LogoDir!" -ForegroundColor Green
        $extracted = $true
    } catch {
        Write-Warning "Could not extract $($zip.Name): $_"
    }
}

# Check for loose svg or png files
$looseFiles = Get-ChildItem -Path $DownloadsDir | Where-Object {
    ($_.Extension -in @(".svg", ".png")) -and 
    ($_.Name -match "Primary logo" -or $_.Name -match "brandmark" -or $_.Name -match "Wordmark" -or $_.Name -match "App icon" -or $_.Name -match "SP Lang")
}

foreach ($file in $looseFiles) {
    $dest = Join-Path $LogoDir $file.Name
    Copy-Item -Path $file.FullName -Destination $dest -Force
    Write-Host "Copied $($file.Name) into $LogoDir" -ForegroundColor Green
    $extracted = $true
}

if (-not $extracted) {
    Write-Host "No new logo files found in Downloads yet." -ForegroundColor DarkYellow
    Write-Host "In Figma: select the logo layers -> Click 'Export' -> Export as SVG & PNG -> Run this script." -ForegroundColor Cyan
} else {
    Write-Host "`nAll downloaded logos are now in $LogoDir!" -ForegroundColor Green
}
