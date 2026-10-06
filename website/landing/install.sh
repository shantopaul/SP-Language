#!/usr/bin/env bash
# SP Programming Language Installer for Linux & macOS
# Usage: curl -fsSL https://splang.shantopaul.com/install.sh | sh

set -e

echo "=========================================================="
echo "    SP Programming Language Installer (v1.0.0)"
echo "=========================================================="

INSTALL_DIR="$HOME/.sp"
BIN_DIR="$INSTALL_DIR/bin"

echo "[1/3] Preparing directory: $INSTALL_DIR"
mkdir -p "$BIN_DIR"

OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"

if [ "$ARCH" = "x86_64" ]; then
    ARCH="x86_64"
elif [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
    ARCH="arm64"
fi

TAR_NAME="sp-1.0.0-${OS}-${ARCH}.tar.gz"
DOWNLOAD_URL="https://github.com/shantopaul/SP-Language/releases/latest/download/${TAR_NAME}"

echo "[2/3] Downloading toolchain from: $DOWNLOAD_URL"
echo "[3/3] Setting up PATH environment in shell configuration..."

SHELL_RC="$HOME/.bashrc"
if [ -f "$HOME/.zshrc" ]; then
    SHELL_RC="$HOME/.zshrc"
fi

if ! grep -q "$BIN_DIR" "$SHELL_RC" 2>/dev/null; then
    echo "export PATH=\"$BIN_DIR:\$PATH\"" >> "$SHELL_RC"
    echo "Added $BIN_DIR to $SHELL_RC"
fi

echo ""
echo "Installation complete! Restart your shell or run:"
echo "  export PATH=\"$BIN_DIR:\$PATH\""
echo "  spc --version"
echo "  spm --version"
echo "Visit: https://docs.splang.shantopaul.com"
echo ""
