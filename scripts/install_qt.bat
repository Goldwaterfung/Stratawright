@echo off
setlocal enabledelayedexpansion
:: Unified Qt installer via aqtinstall (Windows).
:: Mirrors scripts/install_qt.sh — single pinned version: Qt 6.8.0.
:: Usage: scripts\install_qt.bat [--outputdir DIR]

set "QT_VERSION=6.8.0"
set "QT_ARCH=win64_msvc2022_64"
set "QT_MODULES=qtbase qtsvg qttools"
if defined QT_INSTALL_DIR (
    set "OUTPUTDIR=%QT_INSTALL_DIR%"
) else (
    set "OUTPUTDIR=C:\Qt"
)

if "%~1"=="--outputdir" (
    set "OUTPUTDIR=%~2"
)

set "QT_DIR=%OUTPUTDIR%\%QT_VERSION%\msvc2022_64"
if exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo Qt %QT_VERSION% already installed at %QT_DIR%, skipping.
    exit /b 0
)

echo Installing Qt %QT_VERSION% (%QT_ARCH%) to %OUTPUTDIR% ...
where aqt >nul 2>nul
if errorlevel 1 (
    echo Installing aqtinstall via pip...
    python -m pip install aqtinstall
    if errorlevel 1 exit /b 1
)

aqt install-qt windows desktop %QT_VERSION% %QT_ARCH% --outputdir "%OUTPUTDIR%" -m %QT_MODULES%
if errorlevel 1 exit /b 1

echo Qt installed at %QT_DIR%
echo Configure with: -DCMAKE_PREFIX_PATH="%QT_DIR%"
