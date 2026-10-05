@echo off
setlocal enabledelayedexpansion

set "PROJECT_ROOT=%~dp0.."
set "VCPKG_DIR=%PROJECT_ROOT%\vcpkg"

echo Setting up vcpkg in %VCPKG_DIR%...

if not exist "%VCPKG_DIR%" (
    echo Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
    if errorlevel 1 (
        echo Failed to clone vcpkg.
        exit /b 1
    )
)

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo Bootstrapping vcpkg...
    cd /d "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat
    if errorlevel 1 (
        echo Failed to bootstrap vcpkg.
        exit /b 1
    )
    cd /d "%PROJECT_ROOT%"
)

echo vcpkg setup complete!
