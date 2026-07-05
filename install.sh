#!/usr/bin/env bash
# install.sh — builds and installs the AGONY interpreter.
#
# Usage:
#   ./install.sh            # installs to ~/.local/bin (no sudo needed)
#   ./install.sh --system   # installs to /usr/local/bin (uses sudo if needed)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$SCRIPT_DIR/agony.cpp"
BIN_NAME="agony"

INSTALL_DIR="$HOME/.local/bin"
USE_SUDO=0

if [[ "${1:-}" == "--system" ]]; then
    INSTALL_DIR="/usr/local/bin"
fi

echo "== AGONY installer =="
echo "This will inflict a working compiler on your machine and use it against you."
echo

# ---------------------------------------------------------------------------
# 1. Find a C++ compiler.
# ---------------------------------------------------------------------------
CXX=""
for candidate in g++ clang++; do
    if command -v "$candidate" >/dev/null 2>&1; then
        CXX="$candidate"
        break
    fi
done

if [[ -z "$CXX" ]]; then
    echo "No C++ compiler (g++ or clang++) found on this machine."
    echo "Attempting to install one..."

    if command -v apt-get >/dev/null 2>&1; then
        echo "Detected apt (Debian/Ubuntu). Installing g++..."
        if [[ $EUID -ne 0 ]]; then
            sudo apt-get update && sudo apt-get install -y g++
        else
            apt-get update && apt-get install -y g++
        fi
        CXX="g++"
    elif command -v dnf >/dev/null 2>&1; then
        echo "Detected dnf (Fedora). Installing gcc-c++..."
        if [[ $EUID -ne 0 ]]; then
            sudo dnf install -y gcc-c++
        else
            dnf install -y gcc-c++
        fi
        CXX="g++"
    elif command -v pacman >/dev/null 2>&1; then
        echo "Detected pacman (Arch). Installing gcc..."
        if [[ $EUID -ne 0 ]]; then
            sudo pacman -Sy --noconfirm gcc
        else
            pacman -Sy --noconfirm gcc
        fi
        CXX="g++"
    elif command -v brew >/dev/null 2>&1; then
        echo "Detected Homebrew (macOS). Installing gcc..."
        brew install gcc
        CXX="g++"
    else
        echo "ERROR: could not detect a supported package manager (apt, dnf, pacman, brew)."
        echo "Please install a C++17-capable compiler (g++ or clang++) manually and re-run this script."
        exit 1
    fi
fi

echo "Using compiler: $CXX"

# ---------------------------------------------------------------------------
# 2. Compile.
# ---------------------------------------------------------------------------
if [[ ! -f "$SRC" ]]; then
    echo "ERROR: could not find agony.cpp next to this script ($SCRIPT_DIR)."
    exit 1
fi

BUILD_DIR="$(mktemp -d)"
echo "Compiling agony.cpp..."
"$CXX" -std=c++17 -O2 -Wall -o "$BUILD_DIR/$BIN_NAME" "$SRC"
echo "Build succeeded."

# ---------------------------------------------------------------------------
# 3. Install onto PATH.
# ---------------------------------------------------------------------------
mkdir -p "$INSTALL_DIR"
if [[ -w "$INSTALL_DIR" ]]; then
    cp "$BUILD_DIR/$BIN_NAME" "$INSTALL_DIR/$BIN_NAME"
else
    echo "Need elevated privileges to write to $INSTALL_DIR"
    sudo cp "$BUILD_DIR/$BIN_NAME" "$INSTALL_DIR/$BIN_NAME"
    USE_SUDO=1
fi
chmod +x "$INSTALL_DIR/$BIN_NAME" 2>/dev/null || sudo chmod +x "$INSTALL_DIR/$BIN_NAME"

rm -rf "$BUILD_DIR"

echo
echo "AGONY installed to: $INSTALL_DIR/$BIN_NAME"

# ---------------------------------------------------------------------------
# 4. PATH check.
# ---------------------------------------------------------------------------
if ! command -v "$BIN_NAME" >/dev/null 2>&1; then
    if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
        echo
        echo "NOTE: $INSTALL_DIR is not on your PATH yet."
        echo "Add this to your shell profile (~/.bashrc, ~/.zshrc, etc):"
        echo
        echo "    export PATH=\"$INSTALL_DIR:\$PATH\""
        echo
    fi
fi

echo "Try it now:"
echo "    agony $SCRIPT_DIR/hello.agony"
echo
echo "May your parity bits always be correct."
