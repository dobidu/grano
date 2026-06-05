@echo off
:: Grano Windows acceptance test runner
:: Launches windows-test.ps1 with ExecutionPolicy bypass.
:: Usage:
::   windows-test.bat                              -- auto-download from CI (needs gh CLI)
::   windows-test.bat "D:\path\Grano.vst3"         -- VST3 only
::   windows-test.bat "D:\path\Grano.vst3" "D:\path\Grano.exe"  -- VST3 + Standalone

setlocal
set "SCRIPT=%~dp0windows-test.ps1"

if "%~1"=="" (
    powershell.exe -ExecutionPolicy Bypass -File "%SCRIPT%"
) else if "%~2"=="" (
    powershell.exe -ExecutionPolicy Bypass -File "%SCRIPT%" -Vst3Path "%~1"
) else (
    powershell.exe -ExecutionPolicy Bypass -File "%SCRIPT%" -Vst3Path "%~1" -StandalonePath "%~2"
)

pause
