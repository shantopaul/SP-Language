$ErrorActionPreference = "Stop"

$FrontendDir = Join-Path $PSScriptRoot "frontend"
Push-Location $FrontendDir

try {
    if (-not (Test-Path ".env.local") -and (Test-Path ".env.example")) {
        Copy-Item ".env.example" ".env.local"
    }

    if (-not (Test-Path "node_modules")) {
        Write-Host "Installing Next.js frontend dependencies..."
        npm install
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    Write-Host "Starting ToDoList Next.js frontend on http://localhost:3000 ..."
    npm run dev
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
