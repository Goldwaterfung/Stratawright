#!/bin/bash

# Stratawright Dependency Installation Script
# Supports macOS, Ubuntu/Debian, and Fedora/RHEL

set -e

OS="$(uname -s)"
echo "Detected OS: $OS"

case "$OS" in
    Darwin*)
        echo "Installing dependencies for macOS..."

        # Check if Homebrew is installed (only for tools)
        if ! command -v brew &> /dev/null; then
            echo "Homebrew not found. Please install from https://brew.sh/ to install build tools."
            exit 1
        fi

        # Install build tools (Qt comes from unified aqtinstall script below,
        # NOT Homebrew — keeps mac/Windows/CI on the same pinned Qt 6.8.0)
        echo "Installing build tools via Homebrew..."
        brew install cmake pkg-config git

        # Install pinned Qt 6.8.0 via aqtinstall (same as Windows + CI)
        echo "Installing Qt via aqtinstall..."
        bash "$(dirname "$0")/install_qt.sh"

        # Setup vcpkg and third-party SDKs (vcpkg.json intentionally has NO Qt —
        # vcpkg Qt is too large/slow for CI)
        bash ./scripts/setup_vcpkg.sh
        bash ./scripts/setup_third_party.sh
        ;;

    Linux*)
        echo "Linux platform is not supported in Stratawright."
        exit 1
        ;;

    MINGW*|CYGWIN*|MSYS*)
        echo "Windows detected via Git Bash/MSYS."
        echo "Note: Qt 6 should be installed separately via Qt Online Installer or aqtinstall."
        bash ./scripts/setup_vcpkg.sh
        bash ./scripts/setup_third_party.sh
        ;;

    *)
        echo "Unsupported operating system: $OS"
        exit 1
        ;;
esac

echo ""
echo "To verify installation, run:"
echo "  cmake --version"
echo "  g++ --version   (or clang++ --version)"
