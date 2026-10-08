@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0RunRemoteDemo.ps1" %*
set "result=%errorlevel%"
if not "%result%"=="0" (
    echo.
    echo Remote demo could not be started.
    pause
)
exit /b %result%
