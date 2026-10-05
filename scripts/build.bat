@echo off
setlocal enabledelayedexpansion

set "PROJECT_ROOT=%~dp0.."
set "BUILD_TYPE=Debug"
set "BUILD_TESTS=OFF"
set "RUN_TESTS=OFF"
set "ENABLE_SIMD=ON"
set "CLEAN_BUILD=0"

:parse_args
if "%~1"=="" goto :done_args
if /i "%~1"=="debug" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)
if /i "%~1"=="release" (
    set "BUILD_TYPE=Release"
    shift
    goto :parse_args
)
if /i "%~1"=="profile" (
    set "BUILD_TYPE=RelWithDebInfo"
    shift
    goto :parse_args
)
if /i "%~1"=="--test" (
    set "BUILD_TESTS=ON"
    set "RUN_TESTS=ON"
    shift
    goto :parse_args
)
if /i "%~1"=="--no-tests" (
    set "BUILD_TESTS=OFF"
    shift
    goto :parse_args
)
if /i "%~1"=="--no-simd" (
    set "ENABLE_SIMD=OFF"
    shift
    goto :parse_args
)
if /i "%~1"=="--clean" (
    set "CLEAN_BUILD=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    goto :print_help
)
echo Unknown option: %~1
goto :print_help

:print_help
echo Stratawright Windows Build Script
echo.
echo Usage: scripts\build.bat [CONFIGURATION] [OPTIONS]
echo.
echo Configurations:
echo   debug        Debug build (default)
echo   release      Release build (optimized)
echo   profile      Profile build (RelWithDebInfo)
echo.
echo Options:
echo   --test       Build and run unit tests
echo   --no-tests   Skip building tests
echo   --no-simd    Disable SIMD optimizations
echo   --clean      Clean build directory first
echo   --help       Show this help message
exit /b 0

:done_args

:: Determine build folder name
if /i "%BUILD_TYPE%"=="Debug" set "BUILD_SUBDIR=debug"
if /i "%BUILD_TYPE%"=="Release" set "BUILD_SUBDIR=release"
if /i "%BUILD_TYPE%"=="RelWithDebInfo" set "BUILD_SUBDIR=profile"
set "BUILD_DIR=%PROJECT_ROOT%\build\%BUILD_SUBDIR%"

echo ========================================
echo Building Stratawright (Windows)
echo Configuration: %BUILD_TYPE%
echo Build directory: %BUILD_DIR%
echo Tests: %BUILD_TESTS%
echo SIMD: %ENABLE_SIMD%
echo ========================================
echo.

:: Clean if requested
if "%CLEAN_BUILD%"=="1" (
    echo Cleaning %BUILD_DIR%...
    if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
)

:: Locate Qt 6 if not already in CMAKE_PREFIX_PATH
if not defined CMAKE_PREFIX_PATH (
    if defined Qt6_DIR (
        set "CMAKE_PREFIX_PATH=%Qt6_DIR%"
    ) else (
        for /d %%D in ("C:\Qt\6.*") do (
            if exist "%%D\msvc2022_64" (
                set "CMAKE_PREFIX_PATH=%%D\msvc2022_64"
            )
        )
    )
)

if defined CMAKE_PREFIX_PATH (
    echo Using Qt6 from: %CMAKE_PREFIX_PATH%
) else (
    echo Warning: Qt 6 path not detected in C:\Qt. If CMake fails to find Qt6, pass -DCMAKE_PREFIX_PATH=...
)

:: Toolchain path
set "VCPKG_TOOLCHAIN=%PROJECT_ROOT%\vcpkg\scripts\buildsystems\vcpkg.cmake"
if not exist "%VCPKG_TOOLCHAIN%" (
    echo [ERROR] vcpkg toolchain not found at %VCPKG_TOOLCHAIN%
    echo Please run scripts\install_dependencies.bat first.
    exit /b 1
)

:: Configure with CMake
echo.
echo Configuring with CMake...
cmake -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE="%BUILD_TYPE%" ^
    -DBUILD_TESTS="%BUILD_TESTS%" ^
    -DENABLE_SIMD="%ENABLE_SIMD%" ^
    -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

:: Build
echo.
echo Building...
cmake --build "%BUILD_DIR%" --config "%BUILD_TYPE%" --parallel
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

:: Run tests if requested
if "%RUN_TESTS%"=="ON" (
    echo.
    echo Running tests...
    ctest --test-dir "%BUILD_DIR%" -C "%BUILD_TYPE%" --output-on-failure
)

echo.
echo ========================================
echo Build complete!
echo Executable: %BUILD_DIR%\bin\%BUILD_TYPE%\stratawright.exe
echo To run:
echo   %BUILD_DIR%\bin\%BUILD_TYPE%\stratawright.exe
echo ========================================
