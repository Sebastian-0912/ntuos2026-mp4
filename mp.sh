#!/bin/bash
# mp.sh - Unified management script for xv6-ntu-template

# Load configuration
if [ -f "mp.conf" ]; then
    source mp.conf
else
    echo "Error: mp.conf not found."
    exit 1
fi

# Constants
SCRIPT_DIR=$(realpath "$(dirname "$0")")
IMAGE_NAME="${DOCKER_IMAGE:-ntuos/mp2}" # Default fallback

# Docker command wrapper, fallback on podman
if command -v docker > /dev/null 2>&1; then
    DOCKER_CMD="docker"
elif command -v podman > /dev/null 2>&1; then
    DOCKER_CMD="podman"
else
    echo "Error: docker not found."
    exit 1
fi
DOCKER_CMD_OPTS=""
if $DOCKER_CMD ps >/dev/null 2>&1; then
    # Running without sudo
    if [[ "$DOCKER_CMD" == "podman" ]]; then
        DOCKER_CMD_OPTS="--userns=keep-id --security-opt label=disable"
    fi
else
    echo "Info: running container with sudo"
    if [[ "$DOCKER_CMD" == "podman" ]]; then
        DOCKER_CMD_OPTS="--security-opt label=disable"
    fi
    DOCKER_CMD="sudo $DOCKER_CMD"
fi

if [ -z "$GITHUB_ACTIONS" ]; then
    DOCKER_CMD_OPTS+=" -it"
fi

# Helper functions
maysudo() {
    if ! "$@" >/dev/null 2>&1; then
        sudo "$@" >/dev/null 2>&1 || { echo "Error: '$*' failed even with sudo." >&2; return 1; }
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
    local
    desired_user_group="$(id -u):$(id -g)"
    if [ "$current_user_group" != "$desired_user_group" ]; then
        maysudo chown -R "$desired_user_group" "$target" >/dev/null 2>&1
    fi
}

START_IMAGE="$DOCKER_CMD run $DOCKER_CMD_OPTS -v $(realpath "$SCRIPT_DIR"):/home/student/xv6 -w /home/student/xv6 -u 1000:1000 --rm $IMAGE_NAME"

# Main logic
case "$1" in
    "setup")
        echo "Setting up environment for $MP_ID..."
        mkdir -p .git/hooks
        # Link hooks if scripts directory exists
        if [ -d "scripts" ]; then
            for hook in pre-commit pre-push; do
                if [ -f "scripts/$hook" ]; then
                    ln -sf "../../scripts/$hook" ".git/hooks/$hook"
                fi
            done
        fi
        ;;
    "qemu")
        echo "Starting qemu in $IMAGE_NAME..."
        $START_IMAGE make qemu
        chown_if_need "."
        ;;
    "test"|"grade")
        echo "Running tests for $MP_ID..."
        # Pass arguments to run.py
        shift
        $START_IMAGE python3 grade/run.py "$@"
        chown_if_need "."
        ;;
    "clean")
        $START_IMAGE make clean
        chown_if_need "."
        ;;
    *)
        echo "Usage: $0 {setup|qemu|test|grade|clean}"
        exit 1
        ;;
esac
