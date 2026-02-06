# xv6-ntu-template

This repository serves as the template for the Operating Systems course assignments (MP0-MP4) at National Taiwan University. It extends the vanilla [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) with a modular infrastructure for assignment management, testing, and grading.

## Features

-   **Unified Management**: A single script `mp.sh` handles environment setup, building, running, and testing.
-   **Modular Configuration**: Assignment-specific settings are isolated in `mp.conf` and `conf/mp.mk`, keeping the core kernel code clean.
-   **Automated Grading**: Integrated Python-based grading system (`grade/`) with support for both shell-script tests and complex Python logic.
-   **Docker Support**: Consistent development environment via Docker.

## Usage

### 1. Setup

Initialize the environment (e.g., setting up git hooks):

```bash
./mp.sh setup
```

### 2. Run QEMU

Build and run xv6 in QEMU:

```bash
./mp.sh qemu
```

### 3. Run Tests

Execute the automated tests for the current assignment:

```bash
./mp.sh test
```

### 4. Clean

Clean build artifacts:

```bash
./mp.sh clean
```

## Configuration

### `mp.conf`

This file defines the current assignment ID and environment settings.

```bash
MP_ID="mp2"
DOCKER_IMAGE="ntuos/mp2"
```

### `conf/mp.mk`

This Makefile snippet allows you to inject assignment-specific build rules.

```makefile
# Example: Add a user program _myprog
UPROGS += $U/_myprog
```

## Directory Structure

-   `conf/`: Configuration files (e.g., `mp.mk`).
-   `grade/`: Grading system core (`run.py`, `gradelib.py`).
-   `tests/`: Assignment-specific test scripts and data.
-   `mp.sh`: Main management script.
-   `mp.conf`: Main configuration file.
