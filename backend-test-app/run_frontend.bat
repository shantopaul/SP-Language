@echo off
setlocal
echo =========================================================================
echo               Starting Next.js 14 Frontend Application                   
echo =========================================================================

cd /d "%~dp0fontend-next"

echo [+] Launching Next.js development server...
echo     Dashboard available at http://localhost:3000
echo =========================================================================
npm run dev
