@echo off
REM Builds every target, then deploys the Qt runtime next to giga-strata.exe so the
REM bin folder runs on its own (NFR-1), then the bundled llama-server into bin\server
REM (docs/05 section 7). Configures first if the build dir is empty.
REM Usage: scripts\build.bat [debug|release] [target]   (default: debug, all targets)
REM Logs:  logs\build_<config>.log, logs\deploy_<config>.log, logs\server_<config>.log
REM Fork:  GIGA_STRATA_FORK_DIR overrides <root>\third_party\llama.cpp-upstream (worktrees have none)
setlocal
call "%~dp0_env.bat" || exit /b 1

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=debug"
set "TARGET=%~2"
set "BUILD_DIR=%ROOT%\build\%CONFIG%"
set "LOG=%LOGS%\build_%CONFIG%.log"
REM Compiler helpers (mspdbsrv and friends) inherit the log handle and outlive a build, so a
REM log they still hold cannot be rewritten: start from a fresh file, or a new name if locked.
del "%LOG%" >nul 2>&1
if exist "%LOG%" set "LOG=%LOGS%\build_%CONFIG%_%RANDOM%.log"

if not exist "%BUILD_DIR%\build.ninja" (
    call "%~dp0configure.bat" %CONFIG% || exit /b 1
)

REM A CMakeLists edit only reaches build.ninja when ninja regenerates it; do that before
REM asking it about targets, or new targets look undefined. Kept outside the if-block:
REM VCVARS may contain "(x86)", which closes a parenthesized block early.
cmd /c "call "%VCVARS%" >nul 2>&1 && "%NINJA%" -C "%BUILD_DIR%" build.ninja" > "%LOGS%\regenerate_%CONFIG%.log" 2>&1
if not "%TARGET%"=="" (
    "%NINJA%" -C "%BUILD_DIR%" -t query %TARGET% >nul 2>&1
    if errorlevel 1 (
        echo [BUILD] FAILED - target "%TARGET%" is not defined in build\%CONFIG%
        exit /b 1
    )
)

cmd /c "call "%VCVARS%" >nul 2>&1 && "%NINJA%" -C "%BUILD_DIR%" -j %JOBS% %TARGET%" > "%LOG%" 2>&1
if errorlevel 1 goto failed

REM A locked log means ninja never ran and cmd still returns 0; a real run always prints
REM "[n/m]" progress or "no work to do", so their absence is a failure.
findstr /r /c:"^\[[0-9]" /c:"no work to do" "%LOG%" >nul 2>&1
if errorlevel 1 (
    echo [BUILD] FAILED - ninja did not run; the build log may be locked by another process
    exit /b 1
)

findstr /r /c:": warning [A-Z][A-Z]*[0-9]" "%LOG%" >nul
if not errorlevel 1 (
    echo [BUILD] FAILED - warnings present ^(R32 requires warning-free^) - "%LOG%"
    findstr /r /c:": warning [A-Z][A-Z]*[0-9]" "%LOG%"
    exit /b 1
)
echo [BUILD] SUCCESS - %CONFIG% - "%LOG%"

set "EXE=%BUILD_DIR%\bin\giga-strata.exe"
if not exist "%EXE%" (
    echo [DEPLOY] SKIPPED - "%EXE%" not built yet
    exit /b 0
)
cmd /c "call "%VCVARS%" >nul 2>&1 && "%QT_DIR%\bin\windeployqt.exe" --%CONFIG% --qmldir "%ROOT%\src\ui\qml" --no-translations --no-system-d3d-compiler --no-opengl-sw "%EXE%"" > "%LOGS%\deploy_%CONFIG%.log" 2>&1
if errorlevel 1 (
    echo [DEPLOY] FAILED - logs\deploy_%CONFIG%.log
    exit /b 1
)
echo [DEPLOY] SUCCESS - build\%CONFIG%\bin is self-contained - logs\deploy_%CONFIG%.log
call :server
exit /b %errorlevel%

REM The fork's llama-server, its CUDA 13 runtime and the MSVC runtime it links, in bin\server.
REM The server runs from that folder, so it gets its own copy of the runtime: Windows does not
REM look in the parent bin folder for a child's DLLs.
:server
set "FORK=%GIGA_STRATA_FORK_DIR%"
if "%FORK%"=="" set "FORK=%ROOT%\third_party\llama.cpp-upstream"
set "SERVER_EXE=%FORK%\bin\bin\Release\llama-server.exe"
set "SERVER_DIR=%BUILD_DIR%\bin\server"
set "SERVER_LOG=%LOGS%\server_%CONFIG%.log"
set "CUDA_BIN=%CUDA_PATH_V13_0%\bin\x64"
if "%CUDA_PATH_V13_0%"=="" set "CUDA_BIN=%CUDA_PATH%\bin\x64"
if not exist "%CUDA_BIN%\cudart64_13.dll" set "CUDA_BIN=%ProgramFiles%\NVIDIA GPU Computing Toolkit\CUDA\v13.0\bin\x64"
> "%SERVER_LOG%" echo fork: "%FORK%"

if not exist "%SERVER_EXE%" goto server_missing
where git >nul 2>&1 || goto server_no_git
set "DIRTY="
for /f "usebackq delims=" %%L in (`git -C "%FORK%" status --porcelain --untracked-files^=no`) do set "DIRTY=1"
if defined DIRTY goto server_dirty
set "COMMIT="
set "COMMIT_TIME="
set "BRANCH="
for /f "usebackq tokens=1,2 delims=/" %%A in (`git -C "%FORK%" log -1 --format^=%%H/%%ct`) do set "COMMIT=%%A" & set "COMMIT_TIME=%%B"
for /f "usebackq delims=" %%B in (`git -C "%FORK%" rev-parse --abbrev-ref HEAD`) do set "BRANCH=%%B"
if not defined COMMIT goto server_no_commit
set "EXE_TIME="
set "EXE_BUILT="
for /f "usebackq tokens=1,2" %%A in (`powershell -NoProfile -Command "$t=(Get-Item -LiteralPath $env:SERVER_EXE).LastWriteTimeUtc; '{0} {1}' -f [DateTimeOffset]::new($t).ToUnixTimeSeconds(), $t.ToString('yyyy-MM-ddTHH:mm:ssZ')"`) do set "EXE_TIME=%%A" & set "EXE_BUILT=%%B"
if not defined EXE_TIME goto server_no_time
>> "%SERVER_LOG%" echo commit %COMMIT% at %COMMIT_TIME%, exe built %EXE_BUILT% (%EXE_TIME%)
if %EXE_TIME% LSS %COMMIT_TIME% goto server_stale

for %%F in (cublas64_13.dll cublasLt64_13.dll cudart64_13.dll) do if not exist "%CUDA_BIN%\%%F" goto server_no_cuda
set "VS_ROOT=%VCVARS:\VC\Auxiliary\Build\vcvars64.bat=%"
set "CRT_DIR="
set "OPENMP_DIR="
for /d %%D in ("%VS_ROOT%\VC\Redist\MSVC\*") do if exist "%%D\x64" for /d %%R in ("%%D\x64\Microsoft.VC*.CRT") do set "CRT_DIR=%%R"
for /d %%D in ("%VS_ROOT%\VC\Redist\MSVC\*") do if exist "%%D\x64" for /d %%R in ("%%D\x64\Microsoft.VC*.OpenMP") do set "OPENMP_DIR=%%R"
if not defined CRT_DIR goto server_no_crt
if not defined OPENMP_DIR goto server_no_crt

if not exist "%SERVER_DIR%" mkdir "%SERVER_DIR%"
REM xcopy /d copies only what changed, so a test run does not re-copy 500 MB of CUDA DLLs.
xcopy "%SERVER_EXE%" "%SERVER_DIR%\" /d /y /q >> "%SERVER_LOG%" 2>&1 || goto server_copy
for %%F in (cublas64_13.dll cublasLt64_13.dll cudart64_13.dll) do xcopy "%CUDA_BIN%\%%F" "%SERVER_DIR%\" /d /y /q >> "%SERVER_LOG%" 2>&1 || goto server_copy
for %%F in (msvcp140.dll vcruntime140.dll vcruntime140_1.dll) do xcopy "%CRT_DIR%\%%F" "%SERVER_DIR%\" /d /y /q >> "%SERVER_LOG%" 2>&1 || goto server_copy
xcopy "%OPENMP_DIR%\vcomp140.dll" "%SERVER_DIR%\" /d /y /q >> "%SERVER_LOG%" 2>&1 || goto server_copy
> "%SERVER_DIR%\BUILD_INFO.txt" echo commit: %COMMIT%
>> "%SERVER_DIR%\BUILD_INFO.txt" echo branch: %BRANCH%
>> "%SERVER_DIR%\BUILD_INFO.txt" echo built: %EXE_BUILT%
echo [SERVER] SUCCESS - build\%CONFIG%\bin\server ^(%BRANCH% %COMMIT:~0,12%^) - logs\server_%CONFIG%.log
exit /b 0

:server_missing
REM Like DEPLOY before the exe exists: a checkout without the fork (a worktree, a fresh clone) still
REM builds and tests; the packaged folder then has no server, which this line says.
echo [SERVER] SKIPPED - "%SERVER_EXE%" not found; build the fork or set GIGA_STRATA_FORK_DIR
exit /b 0
:server_no_git
echo [SERVER] FAILED - git not found on PATH; it reads the fork's commit for BUILD_INFO.txt
exit /b 1
:server_dirty
echo [SERVER] FAILED - the fork at "%FORK%" has uncommitted changes; commit them and rebuild the server
exit /b 1
:server_no_commit
echo [SERVER] FAILED - "%FORK%" is not a git checkout
exit /b 1
:server_stale
echo [SERVER] FAILED - llama-server.exe ^(%EXE_BUILT%^) is older than the fork's HEAD commit; rebuild the server
exit /b 1
:server_no_cuda
echo [SERVER] FAILED - CUDA 13 runtime DLLs not found in "%CUDA_BIN%"; install the CUDA 13 toolkit
exit /b 1
:server_no_crt
echo [SERVER] FAILED - MSVC x64 redistributable folders not found under "%VS_ROOT%\VC\Redist\MSVC"
exit /b 1
:server_no_time
echo [SERVER] FAILED - could not read the modification time of "%SERVER_EXE%"
exit /b 1
:server_copy
echo [SERVER] FAILED - copying into "%SERVER_DIR%" failed - logs\server_%CONFIG%.log
exit /b 1

:failed
echo [BUILD] FAILED - "%LOG%"
findstr /i /c:"error" "%LOG%"
exit /b 1
