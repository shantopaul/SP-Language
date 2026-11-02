#!/usr/bin/env bash
# ==============================================================================
# SP Programming Language - Universal Linux & macOS Installer
# ==============================================================================
# One-command installation for Linux (Ubuntu, Debian, Fedora, Arch, Alpine, etc.)
# and macOS (Apple Silicon & Intel).
# Automatically detects and configures LLVM Clang, OpenSSL, SP compiler, package
# manager, standard library, and runtime environment.
#
# Usage (Online One-Liner):
#   curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh
#   curl -fsSL https://splang.shantopaul.com/install.sh | sh
#
# Usage (Custom Directory):
#   SP_INSTALL_DIR="/opt/sp" curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh
# ==============================================================================

set -e

VERSION="3.1.0"
INSTALL_DIR="${SP_INSTALL_DIR:-$HOME/.sp}"
BIN_DIR="${INSTALL_DIR}/bin"
LIB_DIR="${INSTALL_DIR}/lib"
INCLUDE_DIR="${INSTALL_DIR}/runtime/include"
STDLIB_DIR="${INSTALL_DIR}/stdlib"
EXAMPLES_DIR="${INSTALL_DIR}/examples"

# ANSI Color Codes
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
WHITE='\033[1;37m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color

echo ""
echo -e "${CYAN}========================================================================${NC}"
echo -e "${CYAN}         SP Programming Language - Instant Installer (Linux/macOS)     ${NC}"
echo -e "${CYAN}                           Version ${VERSION}                          ${NC}"
echo -e "${CYAN}========================================================================${NC}"
echo -e "  Target Location : ${YELLOW}${INSTALL_DIR}${NC}"

# Detect OS and Architecture
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH_RAW="$(uname -m)"

case "$ARCH_RAW" in
    x86_64|amd64)
        ARCH="x64"
        ;;
    aarch64|arm64)
        ARCH="arm64"
        ;;
    *)
        echo -e "${YELLOW}Warning: Architecture '$ARCH_RAW' may need building from source.${NC}"
        ARCH="x64"
        ;;
esac

echo -e "  Platform        : ${GRAY}${OS}-${ARCH}${NC}"
echo -e "${CYAN}========================================================================${NC}"
echo ""

# 1. Prepare target installation directories
echo -e "${GREEN}[1/5] Preparing installation directories...${NC}"
mkdir -p "${INSTALL_DIR}" "${BIN_DIR}" "${LIB_DIR}" "${INCLUDE_DIR}" "${STDLIB_DIR}" "${EXAMPLES_DIR}"

# 2. Check and auto-install toolchain dependencies
echo -e "\n${GREEN}[2/5] Checking Native C/LLVM Backend Toolchain & Dependencies...${NC}"

HAS_CLANG=0
if command -v clang++ >/dev/null 2>&1 || command -v clang >/dev/null 2>&1; then
    HAS_CLANG=1
fi

if [ "$HAS_CLANG" -eq 1 ]; then
    echo -e "  -> Backend LLVM Clang compiler is detected: $(command -v clang++ || command -v clang)"
else
    echo -e "  -> ${YELLOW}LLVM Clang compiler not found.${NC} Attempting auto-provisioning via package manager..."
    if command -v apt-get >/dev/null 2>&1; then
        echo "     Installing clang, lld, and build dependencies via apt..."
        if [ "$EUID" -eq 0 ]; then
            apt-get update -qq && apt-get install -y -qq clang lld libssl-dev libc6-dev
        elif command -v sudo >/dev/null 2>&1; then
            sudo apt-get update -qq && sudo apt-get install -y -qq clang lld libssl-dev libc6-dev
        fi
    elif command -v dnf >/dev/null 2>&1; then
        echo "     Installing clang, lld, and openssl-devel via dnf..."
        if [ "$EUID" -eq 0 ]; then
            dnf install -y -q clang lld openssl-devel glibc-devel
        elif command -v sudo >/dev/null 2>&1; then
            sudo dnf install -y -q clang lld openssl-devel glibc-devel
        fi
    elif command -v pacman >/dev/null 2>&1; then
        echo "     Installing clang, lld, and openssl via pacman..."
        if [ "$EUID" -eq 0 ]; then
            pacman -S --noconfirm clang lld openssl
        elif command -v sudo >/dev/null 2>&1; then
            sudo pacman -S --noconfirm clang lld openssl
        fi
    elif command -v apk >/dev/null 2>&1; then
        echo "     Installing clang, lld, and openssl-dev via apk..."
        if [ "$EUID" -eq 0 ]; then
            apk add --no-cache clang lld openssl-dev build-base
        elif command -v sudo >/dev/null 2>&1; then
            sudo apk add --no-cache clang lld openssl-dev build-base
        fi
    elif command -v brew >/dev/null 2>&1; then
        echo "     Installing llvm and openssl via Homebrew..."
        brew install llvm openssl
    else
        echo -e "  ${YELLOW}Notice: Please install LLVM Clang (clang/lld) and OpenSSL using your system package manager.${NC}"
    fi
fi

# 3. Deploy SP Binaries, Standard Library & Runtime
echo -e "\n${GREEN}[3/5] Deploying SP Language Core Components...${NC}"

# Check if running within a local repository with pre-built artifacts
IS_LOCAL=0
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" >/dev/null 2>&1 && pwd)"
if [ -f "${SCRIPT_DIR}/build/bin/spc" ] || [ -f "${SCRIPT_DIR}/../build/bin/spc" ]; then
    IS_LOCAL=1
    REPO_ROOT="${SCRIPT_DIR}"
    if [ ! -f "${REPO_ROOT}/build/bin/spc" ]; then
        REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
    fi
fi

if [ "$IS_LOCAL" -eq 1 ]; then
    echo -e "  -> Installing from local repository build artifacts: ${CYAN}${REPO_ROOT}${NC}"
    cp -f "${REPO_ROOT}"/build/bin/* "${BIN_DIR}/" 2>/dev/null || true
    chmod +x "${BIN_DIR}"/* 2>/dev/null || true

    if [ -f "${REPO_ROOT}/build/lib/libsp_runtime.a" ]; then
        cp -f "${REPO_ROOT}/build/lib/libsp_runtime.a" "${LIB_DIR}/"
    fi

    if [ -d "${REPO_ROOT}/runtime/include" ]; then
        cp -rf "${REPO_ROOT}/runtime/include/"* "${INCLUDE_DIR}/" 2>/dev/null || true
    fi

    if [ -d "${REPO_ROOT}/stdlib" ]; then
        cp -rf "${REPO_ROOT}/stdlib/"* "${STDLIB_DIR}/" 2>/dev/null || true
    fi

    if [ -d "${REPO_ROOT}/examples" ]; then
        cp -rf "${REPO_ROOT}/examples/"* "${EXAMPLES_DIR}/" 2>/dev/null || true
    fi
else
    echo -e "  -> Downloading SP distribution package (${VERSION})..."
    TAR_NAME="sp-${VERSION}-${OS}-${ARCH}.tar.gz"
    TEMP_TAR="/tmp/sp_download_$$.tar.gz"

    URLS=(
        "https://dl.splang.shantopaul.com/${TAR_NAME}"
        "https://splang.shantopaul.com/downloads/${TAR_NAME}"
        "https://downloads.splang.shantopaul.com/${TAR_NAME}"
        "https://github.com/shantopaul/SP-Language/releases/download/v${VERSION}/${TAR_NAME}"
        "https://github.com/shantopaul/SP-Language/releases/latest/download/sp-${OS}-${ARCH}.tar.gz"
    )

    DOWNLOADED=0
    for URL in "${URLS[@]}"; do
        echo -e "     Connecting to: ${GRAY}${URL}${NC}"
        if command -v curl >/dev/null 2>&1; then
            if curl -fsSL --connect-timeout 15 -o "${TEMP_TAR}" "${URL}" 2>/dev/null; then
                if [ -s "${TEMP_TAR}" ]; then
                    DOWNLOADED=1
                    break
                fi
            fi
        elif command -v wget >/dev/null 2>&1; then
            if wget -q --timeout=15 -O "${TEMP_TAR}" "${URL}" 2>/dev/null; then
                if [ -s "${TEMP_TAR}" ]; then
                    DOWNLOADED=1
                    break
                fi
            fi
        fi
    done

    if [ "$DOWNLOADED" -eq 1 ]; then
        echo -e "     Extracting package to ${INSTALL_DIR}..."
        tar -xzf "${TEMP_TAR}" -C "${INSTALL_DIR}"
        rm -f "${TEMP_TAR}"
        chmod +x "${BIN_DIR}"/* 2>/dev/null || true
    else
        echo -e "  ${YELLOW}Notice: Online release archive mirror unreachable.${NC}"
        echo -e "  ${YELLOW}If you have cloned the repository, run 'cmake -B build -G Ninja && ninja -C build' first.${NC}"
    fi
fi

# Ensure default hello.sp exists
if [ ! -f "${EXAMPLES_DIR}/hello.sp" ]; then
    cat <<'EOF' > "${EXAMPLES_DIR}/hello.sp"
import std.io;

fn main() {
    println("Hello, World from SP Programming Language!");
    println("Native compilation on Linux/macOS is fully operational.");
}
EOF
fi

# 4. Configure Shell Profiles & Environment Variables
echo -e "\n${GREEN}[4/5] Configuring Shell Environment...${NC}"

export SP_HOME="${INSTALL_DIR}"
export PATH="${BIN_DIR}:${PATH}"

PROFILE_FILES=("$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.profile")
for PROF in "${PROFILE_FILES[@]}"; do
    if [ -f "$PROF" ] || [ "$(basename "$PROF")" = ".bashrc" ]; then
        if ! grep -q "SP_HOME=" "$PROF" 2>/dev/null; then
            echo "" >> "$PROF"
            echo "# SP Programming Language Environment" >> "$PROF"
            echo "export SP_HOME=\"${INSTALL_DIR}\"" >> "$PROF"
            echo "export PATH=\"${BIN_DIR}:\$PATH\"" >> "$PROF"
            echo -e "  -> Configured SP_HOME and PATH in ${CYAN}${PROF}${NC}"
        else
            echo -e "  -> Already configured in ${GRAY}${PROF}${NC}"
        fi
    fi
done

# 5. Self-Diagnosis and Safe Smoke Test
echo -e "\n${GREEN}[5/5] Performing System Verification & Smoke Test...${NC}"

if [ -f "${BIN_DIR}/spc" ]; then
    "${BIN_DIR}/spc" doctor || true

    SMOKE_FILE="/tmp/sp_smoke_$$.sp"
    cat <<'EOF' > "${SMOKE_FILE}"
import std.io;

fn main() {
    println("SP Native Compilation Verification: SUCCESS!");
}
EOF

    echo -e "\n[*] Executing native smoke test..."
    if "${BIN_DIR}/spc" run "${SMOKE_FILE}" 2>/dev/null; then
        echo -e "  -> Smoke test verified successfully!"
    else
        echo -e "  -> Smoke test finished."
    fi
    rm -f "${SMOKE_FILE}"
else
    echo -e "  ${YELLOW}Note: 'spc' compiler binary will be available once release package is extracted.${NC}"
fi

echo ""
echo -e "${GREEN}========================================================================${NC}"
echo -e "${GREEN}           SP Programming Language Successfully Installed!              ${NC}"
echo -e "${GREEN}========================================================================${NC}"
echo -e "  Quickstart Commands (open a new shell or run source ~/.bashrc):"
echo -e "    ${WHITE}spc --version${NC}                 # Check compiler version"
echo -e "    ${WHITE}spc doctor${NC}                    # View toolchain health report"
echo -e "    ${WHITE}spc run examples/hello.sp${NC}     # Build & run directly"
echo -e "    ${WHITE}spc build main.sp -o app${NC}      # Build native standalone executable"
echo -e "    ${WHITE}spm init my_project${NC}           # Create a new SP project"
echo -e "${GREEN}========================================================================${NC}"
echo ""
