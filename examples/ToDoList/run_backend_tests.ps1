$ErrorActionPreference = "Stop"

$BackendDir = Join-Path $PSScriptRoot "backend"
Push-Location $BackendDir

try {
    if (-not (Test-Path "build")) {
        New-Item -ItemType Directory -Path "build" | Out-Null
    }

    $suites = @(
        @{ Name = "auth_test"; Source = "tests/auth_test.sp"; Out = "build/auth_test.exe" },
        @{ Name = "category_test"; Source = "tests/category_test.sp"; Out = "build/category_test.exe" },
        @{ Name = "todo_test"; Source = "tests/todo_test.sp"; Out = "build/todo_test.exe" }
    )

    foreach ($suite in $suites) {
        Write-Host "`n=== Building $($suite.Name) ==="
        spc build $suite.Source -o $suite.Out
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

        Write-Host "=== Running $($suite.Name) ==="
        & ".\$($suite.Out)"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    Write-Host "`nAll ToDoList SP backend test suites passed!"
} finally {
    Pop-Location
}
