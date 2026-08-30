#!/bin/bash

# Setup vcpkg for the project
# This script clones vcpkg if it doesn't exist and bootstraps it.

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_DIR="$PROJECT_ROOT/vcpkg"

echo "Setting up vcpkg in $VCPKG_DIR..."

if [ ! -d "$VCPKG_DIR" ]; then
    echo "Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi

if [ ! -f "$VCPKG_DIR/vcpkg" ] && [ ! -f "$VCPKG_DIR/vcpkg.exe" ]; then
    echo "Bootstrapping vcpkg..."
    cd "$VCPKG_DIR"
    if [ -f "./bootstrap-vcpkg.bat" ] && [[ "$(uname -s)" =~ MINGW|MSYS|CYGWIN ]]; then
        cmd.exe /c "bootstrap-vcpkg.bat"
    else
        ./bootstrap-vcpkg.sh
    fi
    cd "$PROJECT_ROOT"
fi

echo "✓ vcpkg setup complete!"
