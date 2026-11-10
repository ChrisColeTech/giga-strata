@echo off
REM Shared environment for every build script. CALLed by them, never run directly.
REM Resolves every tool to an absolute path so nothing depends on the caller's PATH,
REM and never calls vcvars64.bat itself: vcvars in the same cmd session breaks batch
REM labels, so each step runs it in a child cmd (see docs/03 section 6).
REM
REM Exports: ROOT LOGS PY CMAKE CTEST NINJA VCVARS QT_DIR JOBS
REM Overrides (optional env vars): GIGA_STRATA_VENV, GIGA_STRATA_QT_VERSION, GIGA_STRATA_QT_ROOT, GIGA_STRATA_JOBS

for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "LOGS=%ROOT%\logs"
if not exist "%LOGS%" mkdir "%LOGS%"

if not defined GIGA_STRATA_VENV set "GIGA_STRATA_VENV=%ROOT%\.venv"
set "PY=%GIGA_STRATA_VENV%\Scripts\python.exe"
REM The venv was created elsewhere and moved, so pip's cmake.exe launcher in Scripts\ points
REM at a dead interpreter and exits 1 silently. Use the real binaries inside the wheel.
set "CMAKE=%GIGA_STRATA_VENV%\Lib\site-packages\cmake\data\bin\cmake.exe"
set "CTEST=%GIGA_STRATA_VENV%\Lib\site-packages\cmake\data\bin\ctest.exe"
set "NINJA=%GIGA_STRATA_VENV%\Scripts\ninja.exe"

if not defined GIGA_STRATA_QT_VERSION set "GIGA_STRATA_QT_VERSION=6.8.3"
if not defined GIGA_STRATA_QT_ROOT set "GIGA_STRATA_QT_ROOT=C:\Qt"
set "QT_DIR=%GIGA_STRATA_QT_ROOT%\%GIGA_STRATA_QT_VERSION%\msvc2022_64"

REM This rig segfaults compilers under heavy parallel load; 8 jobs is the safe default.
if not defined GIGA_STRATA_JOBS set "GIGA_STRATA_JOBS=8"
set "JOBS=%GIGA_STRATA_JOBS%"

REM vcvars64 starts vctip.exe (telemetry), which inherits the step's redirected log handle and
REM keeps it open, so the next step's "> log" fails with "being used by another process".
set "VSCMD_SKIP_SENDTELEMETRY=1"

if not exist "%PY%" (
    echo [ENV] FAILED - Python venv not found at "%PY%"
    exit /b 1
)
if not exist "%CMAKE%" (
    echo [ENV] FAILED - cmake not found at "%CMAKE%"
    echo        Fix: "%PY%" -m pip install cmake
    exit /b 1
)
if not exist "%NINJA%" (
    echo [ENV] FAILED - ninja not found at "%NINJA%"
    echo        Fix: "%PY%" -m pip install ninja
    exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ENV] FAILED - vswhere.exe not found; install Visual Studio 2022 or VS Build Tools 2022
    exit /b 1
)
set "VCVARS="
for /f "usebackq delims=" %%P in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%P\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
    echo [ENV] FAILED - no Visual Studio 2022 install with the C++ x64 toolset
    echo        Fix: add "Desktop development with C++" in the Visual Studio Installer
    exit /b 1
)
if not exist "%VCVARS%" (
    echo [ENV] FAILED - vcvars64.bat missing at "%VCVARS%"
    exit /b 1
)
exit /b 0
