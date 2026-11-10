@echo off
REM Launches the built app: release first, debug as fallback.
setlocal
set "EXE=%~dp0build\release\bin\giga-strata.exe"
if not exist "%EXE%" set "EXE=%~dp0build\debug\bin\giga-strata.exe"
if not exist "%EXE%" (
    echo [START] giga-strata.exe not built. Run scripts\build.bat release
    exit /b 1
)
start "" "%EXE%"
