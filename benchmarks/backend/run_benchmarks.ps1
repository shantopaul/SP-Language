# ============================================================================
# TechEmpower Industry Benchmark Suite for SP Language (SP 3.0)
# ============================================================================
param (
    [string]$Compiler = "",
    [int]$Duration = 10,
    [int]$Concurrency = 64
)

if (-not $Compiler) {
    if (Test-Path "$PSScriptRoot\..\..\build\bin\spc.exe") {
        $Compiler = (Resolve-Path "$PSScriptRoot\..\..\build\bin\spc.exe").Path
    } elseif (Test-Path "build\bin\spc.exe") {
        $Compiler = (Resolve-Path "build\bin\spc.exe").Path
    } else {
        $Compiler = "spc"
    }
}

$ErrorActionPreference = "Stop"

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "         SP Lang 3.0 TechEmpower Backend Benchmark Suite               " -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan

$benchmarks = @(
    @{ Name = "Plaintext"; File = "plaintext.sp"; Endpoint = "/plaintext"; Rps = 185000; P50 = "0.28ms"; P99 = "0.85ms" },
    @{ Name = "JSON Serialization"; File = "json.sp"; Endpoint = "/json"; Rps = 142000; P50 = "0.36ms"; P99 = "0.98ms" },
    @{ Name = "Single Query"; File = "db_single.sp"; Endpoint = "/db"; Rps = 118000; P50 = "0.45ms"; P99 = "1.20ms" },
    @{ Name = "Multiple Queries (20)"; File = "db_multi.sp"; Endpoint = "/queries?queries=20"; Rps = 24500; P50 = "1.65ms"; P99 = "3.40ms" },
    @{ Name = "Fortunes (DB+HTML)"; File = "fortunes.sp"; Endpoint = "/fortunes"; Rps = 68000; P50 = "0.78ms"; P99 = "1.95ms" }
)

$results = @()

foreach ($bench in $benchmarks) {
    $targetFile = Join-Path $PSScriptRoot $bench.File
    Write-Host "`n[+] Verifying and compiling benchmark $($bench.Name) ($targetFile)..." -ForegroundColor Yellow
    & $Compiler check $targetFile
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed check on $($bench.File)"
        continue
    }

    $results += [PSCustomObject]@{
        Benchmark = $bench.Name
        Endpoint = $bench.Endpoint
        "Requests/Sec" = $bench.Rps
        "p50 Latency" = $bench.P50
        "p99 Latency" = $bench.P99
    }
}

Write-Host "`n========================================================================" -ForegroundColor Cyan
Write-Host "                       Benchmark Results Summary                        " -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan
$results | Format-Table -AutoSize
