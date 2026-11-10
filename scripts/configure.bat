@echo off
REM Generates build\debug or build\release with CMake + Ninja + MSVC + Qt.
REM Usage: scripts\configure.bat [debug|release]     (default: debug)
REM Log:   logs\configure_<config>.log
setlocal
call "%~dp0_env.bat" || exit /b 1

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=debug"
if /i "%CONFIG%"=="debug" (set "BUILD_TYPE=Debug") else if /i "%CONFIG%"=="release" (set "BUILD_TYPE=Release") else (
    echo [CONFIGURE] FAILED - unknown config "%CONFIG%"; use debug or release
    exit /b 1
)
set "BUILD_DIR=%ROOT%\build\%CONFIG%"
set "LOG=%LOGS%\configure_%CONFIG%.log"

if not exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo [CONFIGURE] FAILED - Qt not installed at "%QT_DIR%"; run scripts\setup.bat first
    exit /b 1
)
if not exist "%ROOT%\CMakeLists.txt" (
    echo [CONFIGURE] FAILED - no CMakeLists.txt at "%ROOT%"
    exit /b 1
)

cmd /c "call "%VCVARS%" >nul 2>&1 && "%CMAKE%" -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" > "%LOG%" 2>&1
if errorlevel 1 goto failed

echo [CONFIGURE] SUCCESS - %BUILD_TYPE% - logs\configure_%CONFIG%.log
exit /b 0

:failed
echo [CONFIGURE] FAILED - logs\configure_%CONFIG%.log
findstr /i "error" "%LOG%"
exit /b 1
