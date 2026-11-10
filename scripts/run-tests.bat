@echo off
REM Runs unit + golden tests through ctest. Builds first so tests never run stale binaries.
REM Usage: scripts\run-tests.bat [debug|release]    (default: debug)
REM Log:   logs\tests_<config>.log
setlocal
call "%~dp0_env.bat" || exit /b 1

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=debug"
set "BUILD_DIR=%ROOT%\build\%CONFIG%"
set "LOG=%LOGS%\tests_%CONFIG%.log"

call "%~dp0build.bat" %CONFIG% || exit /b 1

REM Test executables are not deployed; they find Qt DLLs through PATH.
set "PATH=%QT_DIR%\bin;%PATH%"
"%CTEST%" --test-dir "%BUILD_DIR%" --output-on-failure --timeout 60 > "%LOG%" 2>&1
if errorlevel 1 goto failed

findstr /c:"tests passed" "%LOG%"
echo [TESTS] SUCCESS - logs\tests_%CONFIG%.log
exit /b 0

:failed
echo [TESTS] FAILED - logs\tests_%CONFIG%.log
findstr /c:"tests passed" /c:"Failed" /c:"FAIL!" "%LOG%"
exit /b 1
