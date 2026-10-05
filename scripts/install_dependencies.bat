@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Stratawright Windows Dependency Setup
echo ========================================
echo.

:: 1. Check for Git
where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Git is not found in PATH. Please install Git from https://git-scm.com/
    exit /b 1
)

:: 2. Check for CMake
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake is not found in PATH. Please install CMake from https://cmake.org/
    exit /b 1
)

:: 3. Bootstrap vcpkg
echo [1/3] Setting up vcpkg...
call "%~dp0setup_vcpkg.bat"
if errorlevel 1 (
    echo [ERROR] vcpkg setup failed.
    exit /b 1
)

:: 4. Clone third-party SDKs using PowerShell (built-in to Windows)
echo.
echo [2/3] Downloading third-party SDKs...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_third_party.ps1"
if errorlevel 1 (
    echo [ERROR] Third-party SDK setup failed.
    exit /b 1
)

:: 5. Check for pre-built Qt 6 installation
echo.
echo [3/3] Checking for Qt 6...
set "FOUND_QT="
if defined CMAKE_PREFIX_PATH (
    set "FOUND_QT=%CMAKE_PREFIX_PATH%"
) else if defined Qt6_DIR (
    set "FOUND_QT=%Qt6_DIR%"
) else (
    for /d %%D in ("C:\Qt\6.*") do (
        if exist "%%D\msvc2022_64" (
            set "FOUND_QT=%%D\msvc2022_64"
        )
    )
)

if defined FOUND_QT (
    echo [OK] Found Qt 6 at: %FOUND_QT%
) else (
    echo [NOTICE] Pre-built Qt 6 was not detected in C:\Qt\6.*\msvc2022_64.
    echo To install pre-built Qt 6 with a single command:
    echo   pip install aqtinstall
    echo   aqt install-qt windows desktop 6.8.0 win64_msvc2022_64 --outputdir C:\Qt
    echo Or download the official installer from https://www.qt.io/download
)

echo.
echo ========================================
echo Dependencies setup complete!
echo To build, run: scripts\build.bat debug
echo ========================================
