#!/bin/bash
# mp.sh - Unified management script for xv6-ntu-template

# ------------------------------------------------------------------------------
# 1. Configuration & Utilities
# ------------------------------------------------------------------------------

# Colors for friendly output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

hint() {
    echo -e "${BOLD}[HINT]${NC}  $1"
}

# Load configuration
if [ -f "mp.conf" ]; then
    source mp.conf
else
    # Fallback if mp.conf is missing (e.g. freshly cloned)
    warn "mp.conf not found. Using default defaults."
    MP_ID="mpX"
    DOCKER_IMAGE="ntuos/mp2"
fi

# Constants
SCRIPT_DIR=$(realpath "$(dirname "$0")")
IMAGE_NAME="${DOCKER_IMAGE:-ntuos/mp2}"

# ------------------------------------------------------------------------------
# 2. Environment Checks
# ------------------------------------------------------------------------------

check_os() {
    local os_name=$(uname -s)
    local kernel_release=$(uname -r)

    case "$os_name" in
        Linux*)
            if [[ "$kernel_release" == *"microsoft"* || "$kernel_release" == *"Microsoft"* || "$kernel_release" == *"WSL"* ]]; then
                 info "Environment: Windows (WSL2)"
            else
                 info "Environment: Linux"
            fi
            ;;
        Darwin*)
            info "Environment: macOS"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            # Detect legacy Windows shells (Git Bash, Cygwin, MinGW)
            error "Unsupported Shell Environment!"
            hint "You appear to be running in Windows CMD, PowerShell, or Git Bash."
            hint "Please install **WSL2 (Ubuntu)** and run this script from there."
            hint "Refer to doc/environment.md for setup instructions."
            exit 1
            ;;
        *)
            warn "Unknown OS ($os_name). Proceeding with caution..."
            ;;
    esac
}

check_docker() {
    # 2.1 Check if Docker binary exists
    if command -v docker > /dev/null 2>&1; then
        DOCKER_CMD="docker"
    elif command -v podman > /dev/null 2>&1; then
        DOCKER_CMD="podman"
        info "Using Podman as container engine."
    else
        error "Container engine not found."
        hint "Please install Docker Desktop (Windows/macOS) or Docker Engine (Linux)."
        exit 1
    fi

    # 2.2 Check if Docker Daemon is running
    # We use 'docker info' which requires daemon connection
    if ! $DOCKER_CMD info > /dev/null 2>&1; then
        # If standard check fails, try sudo (for Linux)
        if sudo $DOCKER_CMD info > /dev/null 2>&1; then
             warn "Docker daemon is running but requires sudo."
             DOCKER_CMD="sudo $DOCKER_CMD"
        else
             # Daemon is truly unreachable
             error "Docker Daemon is not running!"
             
             if [[ "$(uname -s)" == "Darwin" ]]; then
                 hint "On macOS, please make sure **Docker Desktop** is open and running."
             elif [[ "$(uname -r)" == *"microsoft"* || "$(uname -r)" == *"Microsoft"* ]]; then
                 hint "On WSL2, please make sure **Docker Desktop** is running in Windows."
                 hint "Ensure 'Settings > Resources > WSL Integration' is enabled."
             else
                 hint "On Linux, try starting it with: sudo systemctl start docker"
             fi
             exit 1
        fi
    fi

    # 2.3 Configure Daemon Options
    DOCKER_CMD_OPTS=""
    
    # Check for TTY (interactive mode)
    if [ -t 1 ]; then
        DOCKER_CMD_OPTS+=" -it"
    fi

    # Architecture check for Apple Silicon / ARM64
    if [[ "$(uname -m)" == "arm64" || "$(uname -m)" == "aarch64" ]]; then
        info "ARM64 architecture detected."
        # No warning needed now as we support multi-arch
    fi

    # Podman specific fix
    if [[ "$DOCKER_CMD" == *"podman"* ]]; then
        # Fix permission mapping for podman
        DOCKER_CMD_OPTS+=" --security-opt label=disable"
        # If not root, keep id
        if [[ "$DOCKER_CMD" != *"sudo"* ]]; then
             DOCKER_CMD_OPTS+=" --userns=keep-id"
        fi
    fi
}

check_environment() {
    check_os
    check_docker
}

# Run Checks
check_environment

# ------------------------------------------------------------------------------
# 3. Helper Functions
# ------------------------------------------------------------------------------

maysudo() {
    if ! "$@" >/dev/null 2>&1; then
        sudo "$@" >/dev/null 2>&1 || { error "Command '$*' failed even with sudo."; return 1; }
    fi
}

chown_if_need() {
    local target="$1"
    if [ ! -e "$target" ]; then return 1; fi
    
    local current_user_group
    if [[ "$OSTYPE" == "darwin"* ]]; then
        current_user_group=$(stat -f "%u:%g" "$target" 2>/dev/null)
    else
        current_user_group=$(stat -c "%u:%g" "$target" 2>/dev/null)
    fi
    
    local desired_user_group
    desired_user_group="$(id -u):$(id -g)"
    
    if [ "$current_user_group" != "$desired_user_group" ]; then
        # Only warn/run if we are not using podman (which handles mapping)
        # or if we are using docker
        if [[ "$DOCKER_CMD" != *"podman"* ]]; then
             maysudo chown -R "$desired_user_group" "$target" >/dev/null 2>&1
        fi
    fi
}

START_IMAGE="$DOCKER_CMD run $DOCKER_CMD_OPTS -v $(realpath "$SCRIPT_DIR"):/home/student/xv6 -w /home/student/xv6 -u 1000:1000 --rm $IMAGE_NAME"

# ------------------------------------------------------------------------------
# 4. Main Logic
# ------------------------------------------------------------------------------

case "$1" in
    "setup")
        info "Setting up environment for $MP_ID..."
        mkdir -p .git/hooks
        if [ -d "scripts" ]; then
            for hook in pre-commit pre-push; do
                if [ -f "scripts/$hook" ]; then
                    ln -sf "../../scripts/$hook" ".git/hooks/$hook"
                fi
            done
        fi
        info "Setup complete."
        ;;
    "qemu")
        info "Starting QEMU in $IMAGE_NAME..."
        $START_IMAGE make qemu
        chown_if_need "."
        ;;
    "test"|"grade")
        info "Running tests for $MP_ID..."
        shift
        $START_IMAGE python3 grade/run.py "$@"
        chown_if_need "."
        ;;
    "clean")
        info "Cleaning build artifacts..."
        $START_IMAGE make clean
        chown_if_need "."
        ;;
    *)
        echo "Usage: $0 {setup|qemu|test|grade|clean}"
        exit 1
        ;;
esac
