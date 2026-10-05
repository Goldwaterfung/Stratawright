# Quick Start Guide

## 1. Agentic One-Prompt Setup (Recommended)

Open your AI agent (**Claude Code**, **Codex**, **Cursor**, **Hermes**, **Gemini CLI**, **OpenCode**, etc.) inside this project directory and tell it:

```text
Build and package Stratawright for me
```

*(Your agent will automatically execute `./scripts/install_dependencies.sh` and `./scripts/build.sh release --package` under the hood).*

---

## 2. Manual Command-Line Setup

### Install Dependencies

#### macOS:
Run the dependency installation script to install build tools, pre-built **Qt 6** via Homebrew, and bootstrap **vcpkg**:

```bash
./scripts/install_dependencies.sh
```

This script will:
1. Install `cmake`, `git`, `pkg-config`, and `qt@6` via Homebrew.
2. Bootstrap **vcpkg** in the project root.
3. Automatically download and build `rtaudio`, `rtmidi`, `libsndfile`, `spdlog`, etc.

#### Windows:
1. Install **Visual Studio 2022** or **Visual Studio 2026** (with *Desktop development with C++*) and **Git**.
2. Install pre-built **Qt 6** (using `aqtinstall` or the official Qt Online Installer):
   ```cmd
   pip install aqtinstall
   aqt install-qt windows desktop 6.8.0 win64_msvc2022_64 -m qtsvg --outputdir C:\Qt
   ```
3. Run the Windows dependency setup script (Command Prompt or PowerShell):
   ```cmd
   scripts\install_dependencies.bat
   ```
   *(Alternatively, run `scripts\setup_vcpkg.bat` and `powershell -ExecutionPolicy Bypass -File scripts\setup_third_party.ps1`)*

## 3. Build the Project

### Using build scripts (recommended):

#### macOS:
```bash
# Debug build
./scripts/build.sh debug

# Release build
./scripts/build.sh release

# Release build with tests
./scripts/build.sh release --test
```

#### Windows (Command Prompt / PowerShell):
```cmd
:: Debug build
scripts\build.bat debug

:: Release build
scripts\build.bat release

:: Release build with tests
scripts\build.bat release --test
```

### Manual build:

#### macOS:
```bash
mkdir -p build/debug && cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" ../../
cmake --build . --parallel
```

#### Windows (Command Prompt / PowerShell):
```cmd
mkdir build\debug
cd build\debug
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64" -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake ../../
cmake --build . --config Debug --parallel
```

## 4. Run the Application

```bash
cd build/debug
./bin/stratawright
```

## 5. Run Tests

```bash
cd build/debug
ctest --output-on-failure
```

Or run specific test:
```bash
./tests/unit/unit_tests
```
