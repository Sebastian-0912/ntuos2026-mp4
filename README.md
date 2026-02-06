# Machine Problem Assignment

## Goal
Complete the operating system assignment as specified in the documentation.

## Resources
*   **Specification:** Please refer to the `doc/` directory or the course website.
*   **Environment & Architecture:** [doc/environment.md](doc/environment.md)
*   **Original xv6 README:** [README](https://github.com/mit-pdos/xv6-riscv/blob/riscv/README)

## Quick Start
We provide a script `mp.sh` to help you build and run xv6 using Docker.

### 1. Build and Run xv6 (QEMU)
```bash
./mp.sh qemu
```

### 2. Run Tests
```bash
./mp.sh grade
```

### 3. Clean Build Artifacts
```bash
./mp.sh clean
```

---

## Original xv6-riscv Introduction
xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix Version 6 (v6). xv6 loosely follows the structure and style of v6, but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS
xv6 is inspired by John Lions's Commentary on UNIX 6th Edition. See also https://pdos.csail.mit.edu/6.828/.
