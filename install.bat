@echo off
setlocal
echo ==========================================================
echo        SP Programming Language - Installation Setup       
echo ==========================================================
powershell -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
if %ERRORLEVEL% NEQ 0 (
    echo Installation failed with error code %ERRORLEVEL%.
    pause
    exit /b %ERRORLEVEL%
)
echo.
pause
