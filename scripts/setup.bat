@echo off
REM One-time setup: installs aqtinstall into the venv, then Qt (MSVC 2022 x64) into C:\Qt.
REM Safe to re-run: skips whatever is already present.
REM Log: logs\setup.log
setlocal
call "%~dp0_env.bat" || exit /b 1

if exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo [SETUP] Qt %GIGA_STRATA_QT_VERSION% already installed at "%QT_DIR%"
    exit /b 0
)

echo [SETUP] Installing aqtinstall...
"%PY%" -m pip install --disable-pip-version-check -q aqtinstall==3.3.0 > "%LOGS%\setup.log" 2>&1
if errorlevel 1 goto failed

echo [SETUP] Downloading Qt %GIGA_STRATA_QT_VERSION% msvc2022_64 into %GIGA_STRATA_QT_ROOT% (about 1 GB)...
"%PY%" -m aqt install-qt windows desktop %GIGA_STRATA_QT_VERSION% win64_msvc2022_64 -O "%GIGA_STRATA_QT_ROOT%" >> "%LOGS%\setup.log" 2>&1
if errorlevel 1 goto failed
if not exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" goto failed

echo [SETUP] SUCCESS - Qt at %QT_DIR% - logs\setup.log
exit /b 0

:failed
echo [SETUP] FAILED - logs\setup.log
findstr /i "error" "%LOGS%\setup.log"
exit /b 1
