#!/bin/bash
# mp.sh - Unified management script for xv6-ntu-template

# Constants
SCRIPT_DIR=$(realpath "$(dirname "$0")")

# Load configuration
if [ -f "$SCRIPT_DIR/conf/mp.conf" ]; then
    source "$SCRIPT_DIR/conf/mp.conf"
elif [ -f "mp_config" ]; then
    # Fallback for legacy support
    source mp_config
else
    echo "Warning: conf/mp.conf not found. Using defaults."
fi

# Configuration Defaults
CONTAINER_NAME="${MP_ID:-mp_container}"
IMAGE_NAME="${DOCKER_IMAGE:-ntuos/mp2}" # Default fallback
TEST_DIR_PATH="${TEST_DIR:-tests}"

# Docker command wrapper
DOCKER_CMD="docker"
if ! command -v docker >/dev/null 2>&1; then
    DOCKER_CMD="sudo docker"
fi

DOCKER_IT_FLAG="-it"
if [ -n "$GITHUB_ACTIONS" ]; then
    DOCKER_IT_FLAG=""
fi

# ... (Helper functions omitted for brevity, ensure they are preserved if not targeted) ...

# ... (Skipping to ensure_docker and START_IMAGE logic) ...

# NOTE: Since replacement chunks must be contiguous, I will target the block from DOCKER_CMD definition to START_IMAGE definition.
# Wait, Helper functions are in between. I should use two chunks or one large chunk if I can reproduce the content.
# I will use multi_replace.

if [ -n "$GITHUB_ACTIONS" ]; then
    DOCKER_IT_FLAG=""
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
    local desired_user_group="$(id -u):$(id -g)"
    if [ "$current_user_group" != "$desired_user_group" ]; then
        maysudo chown -R "$desired_user_group" "$target" >/dev/null 2>&1
    fi
}

ensure_docker() {
    if [ -n "$SIMULATION_MODE" ]; then
        echo "Simulation Mode: Executing locally..."
        eval "$@"
        return
    fi

    if [ -f /.dockerenv ]; then
        # Already inside Docker
        eval "$@"
    else
        # Not inside Docker, run the command in a new Docker container
        $DOCKER_CMD run $DOCKER_IT_FLAG -v "$(realpath "$SCRIPT_DIR"):/home/student/xv6" -w /home/student/xv6 -u 1000:1000 --rm "$IMAGE_NAME" "$@"
    fi
}

check_ta_commit() {
    # If TA_EMAILS is not defined, skip this check
    if [ -z "$TA_EMAILS" ]; then
        return 0
    fi

    # Bypass check if in Official Grading mode
    if [ -n "$OFFICIAL_GRADING" ]; then
        return 0
    fi

    # Get the committer email of the HEAD commit
    local committer_email
    committer_email=$(git log -1 --format='%ce')

    for ta_email in $TA_EMAILS; do
        if [ "$committer_email" == "$ta_email" ]; then
            echo "Skipping execution: Commit by TA ($committer_email)."
            exit 0
        fi
    done
}

sanitize() {
    echo "Starting sanitization..."
    if [ -z "$TRUSTED_REPO" ]; then
        echo "Error: TRUSTED_REPO not defined in conf/mp.conf"
        exit 1
    fi

    local temp_dir=$(mktemp -d)
    echo "Cloning trusted repo from $TRUSTED_REPO..."
    git clone --depth 1 "$TRUSTED_REPO" "$temp_dir"

    # Restore critical build files
    echo "Restoring Makefile and grade/ scripts..."
    cp "$temp_dir/Makefile" "$SCRIPT_DIR/Makefile"
    cp -r "$temp_dir/grade/"* "$SCRIPT_DIR/grade/"
    
    # Note: We do NOT overwrite mp.sh itself while it is running.
    
    rm -rf "$temp_dir"
    echo "Sanitization complete."
}

if [ -n "$SIMULATION_MODE" ]; then
    START_IMAGE=""
    echo "Simulation Mode: Docker bypassed."
else
    START_IMAGE="$DOCKER_CMD run $DOCKER_IT_FLAG -v $(realpath $SCRIPT_DIR):/home/student/xv6 -w /home/student/xv6 -u 1000:1000 --rm $IMAGE_NAME"
fi
# Main logic
case "$1" in
    "setup")
        echo "Setting up environment for $ASSIGNMENT..."
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
        check_ta_commit
        echo "Running tests for $ASSIGNMENT..."
        # Pass arguments to run.py
        shift
        $START_IMAGE python3 grade/run.py "$@"
        chown_if_need "."
        ;;
    "sanitize")
        sanitize
        ;;
    "clean")
        $START_IMAGE make clean
        chown_if_need "."
        ;;
    *)
        echo "Usage: $0 {setup|qemu|test|grade|sanitize|clean}"
        exit 1
        ;;
esac
