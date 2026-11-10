@echo off
REM Publishes the bundled llama-server (built from the owner's fork) as a GitHub prerelease that
REM the release workflow downloads into every app zip, and pins it in scripts\server-release.txt.
REM Run after scripts\build.bat has packaged a current server, whenever the fork changes; then
REM commit server-release.txt. Needs the gh CLI, logged in with push rights.
REM Usage: scripts\publish-server.bat [debug|release]   (default: debug; the server is identical)
setlocal
call "%~dp0_env.bat" || exit /b 1

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=debug"
set "SERVER=%ROOT%\build\%CONFIG%\bin\server"
set "ASSET=llama-server-win64-cuda13.zip"
set "ZIP=%LOGS%\%ASSET%"

if not exist "%SERVER%\BUILD_INFO.txt" (
    echo [PUBLISH-SERVER] FAILED - no packaged server in "%SERVER%"; run scripts\build.bat %CONFIG% first
    exit /b 1
)
for /f "tokens=2" %%C in ('findstr /b /c:"commit:" "%SERVER%\BUILD_INFO.txt"') do set "COMMIT=%%C"
set "TAG=llama-server-%COMMIT:~0,12%"

if exist "%ZIP%" del "%ZIP%"
REM Windows 10+ tar writes zip with -a; the archive holds the files at its root. The full path
REM matters: from Git Bash, PATH finds GNU tar first, which reads "D:" as a remote host.
"%SystemRoot%\System32\tar.exe" -a -c -f "%ZIP%" -C "%SERVER%" . > "%LOGS%\publish_server.log" 2>&1
if errorlevel 1 goto failed

gh release view %TAG% >nul 2>&1
if not errorlevel 1 (
    echo [PUBLISH-SERVER] %TAG% already exists; re-uploading the asset
    gh release upload %TAG% "%ZIP%" --clobber >> "%LOGS%\publish_server.log" 2>&1
) else (
    gh release create %TAG% "%ZIP%" --prerelease --target main --title "Bundled llama-server %COMMIT:~0,12%" --notes "llama-server built from the giga-strata llama.cpp fork (commit %COMMIT%), with the CUDA 13 and MSVC runtime DLLs it needs. The app release workflow downloads this into every Giga Strata zip; download a Giga Strata release instead of this." >> "%LOGS%\publish_server.log" 2>&1
)
if errorlevel 1 goto failed

> "%~dp0server-release.txt" echo %TAG%
echo [PUBLISH-SERVER] SUCCESS - %TAG% - commit scripts\server-release.txt
exit /b 0

:failed
echo [PUBLISH-SERVER] FAILED - logs\publish_server.log
type "%LOGS%\publish_server.log"
exit /b 1
