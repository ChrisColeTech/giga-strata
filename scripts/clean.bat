@echo off
REM Deletes a build directory. Use when CMake cache or objects go stale.
REM Usage: scripts\clean.bat [debug|release|all]    (default: debug)
setlocal
call "%~dp0_env.bat" || exit /b 1

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=debug"
if /i "%CONFIG%"=="all" (
    if exist "%ROOT%\build" rmdir /s /q "%ROOT%\build"
    echo [CLEAN] removed build\
    exit /b 0
)
if /i not "%CONFIG%"=="debug" if /i not "%CONFIG%"=="release" (
    echo [CLEAN] FAILED - unknown config "%CONFIG%"; use debug, release or all
    exit /b 1
)
if exist "%ROOT%\build\%CONFIG%" rmdir /s /q "%ROOT%\build\%CONFIG%"
echo [CLEAN] removed build\%CONFIG%
exit /b 0
