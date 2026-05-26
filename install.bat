@echo off
setlocal

cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\install.ps1"

set "exitCode=%ERRORLEVEL%"
echo.
if not "%exitCode%"=="0" echo Install failed with exit code %exitCode%.
pause
exit /b %exitCode%
