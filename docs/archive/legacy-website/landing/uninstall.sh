#!/usr/bin/env bash
# ==============================================================================
# SP Programming Language - Universal Unix/Linux/macOS Uninstaller
# ==============================================================================
# Usage:
#   curl -fsSL https://dl.splang.shantopaul.com/uninstall.sh | sh
# ==============================================================================

set -e

SP_DIR="${SP_HOME:-$HOME/.sp}"

echo ""
echo "========================================================================"
echo "         SP Programming Language - Complete System Uninstaller          "
echo "========================================================================"
echo "  Target Location : $SP_DIR"
echo "========================================================================"
echo ""

if [ -d "$SP_DIR" ]; then
    echo "[*] Removing SP installation directory: $SP_DIR..."
    rm -rf "$SP_DIR"
fi

# Clean PATH from shell rc files
clean_rc() {
    local rc="$1"
    if [ -f "$rc" ]; then
        echo "  -> Cleaning environment from $rc..."
        grep -v 'SP_HOME' "$rc" | grep -v '\.sp/bin' > "${rc}.tmp" || true
        mv "${rc}.tmp" "$rc"
    fi
}

clean_rc "$HOME/.bashrc"
clean_rc "$HOME/.zshrc"
clean_rc "$HOME/.profile"

echo ""
echo "========================================================================"
echo "       SP Programming Language Successfully Uninstalled!                "
echo "========================================================================"
echo "All compiler binaries, package managers, runtimes, and environment"
echo "variables have been cleanly removed."
echo "========================================================================"
echo ""
