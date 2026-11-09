# Script to package official SP Language 3.1.0 distribution archive
$ErrorActionPreference = "Stop"

$distDir = "build\dist\sp-3.1.0-windows-x64"
if (Test-Path $distDir) { Remove-Item $distDir -Recurse -Force }
New-Item -ItemType Directory -Path "$distDir\bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$distDir\stdlib" -Force | Out-Null
New-Item -ItemType Directory -Path "$distDir\lib" -Force | Out-Null
New-Item -ItemType Directory -Path "$distDir\include" -Force | Out-Null
New-Item -ItemType Directory -Path "$distDir\examples" -Force | Out-Null

Copy-Item "C:\SP-Language\bin\spc.exe" "$distDir\bin\spc.exe"
Copy-Item "C:\SP-Language\bin\spm.exe" "$distDir\bin\spm.exe"
Copy-Item "C:\SP-Language\bin\spls.exe" "$distDir\bin\spls.exe"
Copy-Item "C:\SP-Language\bin\spd.exe" "$distDir\bin\spd.exe"

Copy-Item "C:\SP-Language\stdlib\*" "$distDir\stdlib" -Recurse -Force
Copy-Item "C:\SP-Language\lib\*" "$distDir\lib" -Recurse -Force
Copy-Item "C:\SP-Language\include\*" "$distDir\include" -Recurse -Force

$sampleCode = @"
import std.io;

fn main() -> void {
    println("Welcome to SP Programming Language 3.1.0!");
    println("Native LLVM compilation verified.");
}
"@
Set-Content -Path "$distDir\examples\hello.sp" -Value $sampleCode

if (Test-Path "LICENSE") {
    Copy-Item "LICENSE" "$distDir\LICENSE"
}

$outZip = "build\dist\sp-3.1.0-windows-x64.zip"
if (Test-Path $outZip) { Remove-Item $outZip -Force }
Compress-Archive -Path "$distDir\*" -DestinationPath $outZip -CompressionLevel Optimal

$hash = (Get-FileHash -Path $outZip -Algorithm SHA256).Hash.ToLower()
$fileItem = Get-Item $outZip
$sizeBytes = $fileItem.Length
$sizeMB = [math]::Round($sizeBytes / 1048576, 2)

Write-Host "Created: $outZip"
Write-Host "Size: $sizeBytes bytes ($sizeMB MB)"
Write-Host "SHA-256: $hash"

# Output variables to a JSON file for website use
$meta = @{
    version = "3.1.0"
    zip = $outZip
    size = "$sizeMB MB"
    sha256 = $hash
} | ConvertTo-Json

Set-Content -Path "build\dist\metadata.json" -Value $meta
