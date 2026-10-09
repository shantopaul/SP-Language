@echo off
setlocal
echo =========================================================================
echo       Starting SP Lang 3.0 Native Backend with Neon Cloud PostgreSQL      
echo =========================================================================

cd /d "%~dp0backend-splang"

echo [+] Building SP Lang native server binary...
"..\..\build\bin\spc.exe" build src/main.sp -o server.exe --std "..\..\stdlib"
if %errorlevel% neq 0 (
    echo [-] Error: Compilation failed!
    exit /b %errorlevel%
)

echo.
echo [+] Launching SP Lang server on http://localhost:8000 ...
echo     - REST API Root:   http://localhost:8000/api/items
echo     - Health Probe:    http://localhost:8000/api/health
echo     - Swagger UI Docs: http://localhost:8000/docs
echo     - Redoc View:      http://localhost:8000/redoc
echo     - Database:        Neon Cloud PostgreSQL (AWS US-East-2)
echo =========================================================================
.\server.exe
