@echo off
setlocal
rem Run from any directory, including a checkout whose path contains spaces.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0cpp\scripts\setup-windows.ps1" %*
set "mfg_result=%errorlevel%"
if not "%mfg_result%"=="0" (
  echo.
  echo Setup did not finish. Read the error above, then run this file again.
  pause
)
exit /b %mfg_result%
