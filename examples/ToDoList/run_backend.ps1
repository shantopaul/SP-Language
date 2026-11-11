$ErrorActionPreference = "Stop"

$BackendDir = Join-Path $PSScriptRoot "backend"
Push-Location $BackendDir

try {
    if (-not (Test-Path ".env")) {
        Write-Host "No .env file found in backend/. Copying .env.example -> .env ..."
        Copy-Item ".env.example" ".env"
    }

    if (-not (Test-Path "build")) {
        New-Item -ItemType Directory -Path "build" | Out-Null
    }

    Write-Host "Checking SP Language backend..."
    spc check src/main.sp
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Building native SP backend binary..."
    spc build src/main.sp -o build/todolist-backend.exe
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Starting ToDoList SP backend..."
    & ".\build\todolist-backend.exe"
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
