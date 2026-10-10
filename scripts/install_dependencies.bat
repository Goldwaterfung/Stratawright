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

:: 4. Install pinned Qt 6.8.0 via unified aqtinstall script (same as macOS + CI)
echo.
echo [3/3] Installing Qt 6 via aqtinstall...
call "%~dp0install_qt.bat"
if errorlevel 1 (
    echo [ERROR] Qt setup failed.
    exit /b 1
)

echo.
echo ========================================
echo Dependencies setup complete!
echo To build, run: scripts\build.bat debug
echo ========================================
