# ============================================================================
# TechEmpower Industry Benchmark Suite for SP Language (SP 3.0)
#
# HONESTY NOTE (Phase 1): this script does NOT measure performance. It only
# runs `spc check` on each benchmark source to verify it still compiles.
# It never starts a server and never runs a load test, so it produces NO
# requests/sec or latency numbers. Any RPS/latency figures previously
# printed by this script were hard-coded placeholders, not measurements,
# and have been removed. Do not cite benchmark numbers from this script.
# A real measurement harness (server start + load generator + dated
# results file) is Phase 2 work.
# ============================================================================
param (
    [string]$Compiler = ""
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
    @{ Name = "Plaintext"; File = "plaintext.sp"; Endpoint = "/plaintext" },
    @{ Name = "JSON Serialization"; File = "json.sp"; Endpoint = "/json" },
    @{ Name = "Single Query"; File = "db_single.sp"; Endpoint = "/db" },
    @{ Name = "Multiple Queries (20)"; File = "db_multi.sp"; Endpoint = "/queries?queries=20" },
    @{ Name = "Fortunes (DB+HTML)"; File = "fortunes.sp"; Endpoint = "/fortunes" }
)

$results = @()

foreach ($bench in $benchmarks) {
    $targetFile = Join-Path $PSScriptRoot $bench.File
    Write-Host "`n[+] Verifying and compiling benchmark $($bench.Name) ($targetFile)..." -ForegroundColor Yellow
    & $Compiler check $targetFile
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed check on $($bench.File)"
        $results += [PSCustomObject]@{
            Benchmark = $bench.Name
            Endpoint = $bench.Endpoint
            "Check" = "FAILED"
        }
        continue
    }

    $results += [PSCustomObject]@{
        Benchmark = $bench.Name
        Endpoint = $bench.Endpoint
        "Check" = "PASSED"
    }
}

Write-Host "`n========================================================================" -ForegroundColor Cyan
Write-Host "                    Compile-Check Summary (NOT measured)                " -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "NOT MEASURED: this script only ran 'spc check'. No server was started" -ForegroundColor Yellow
Write-Host "and no load test was run, so there are no requests/sec or latency" -ForegroundColor Yellow
Write-Host "numbers to report. Do not cite benchmark figures from this script." -ForegroundColor Yellow
$results | Format-Table -AutoSize
