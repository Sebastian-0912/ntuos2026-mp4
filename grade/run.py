#!/usr/bin/env python3

import sys
import os
import glob
import importlib.util
from gradelib import *

# Configuration
TEST_DIR = os.environ.get("TEST_DIR", "tests")

def run_script_test(test_name, script_path, points=10, timeout=30):
    @test(points, test_name)
    def test_case():
        with open(script_path, "r") as f:
            script = [line.strip() for line in f.readlines()]
        
        output_file = f"out/{test_name}.out"
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        r = Runner(save(output_file))
        r.run_qemu(shell_script(script), timeout=timeout)
        
    return test_case

def load_python_tests(test_dir):
    # Add test_dir to sys.path so imports work if needed
    sys.path.append(os.path.abspath(test_dir))
    
    # 1. Gather all potential test files
    py_files = set(glob.glob(os.path.join(test_dir, "*.py")))
    
    # 2. Logic to load architecture-specific binary tests (.so)
    import platform
    arch = platform.machine()
    
    # Map machine architecture to our suffix convention
    # x86_64 -> .x86_64.so
    # aarch64/arm64 -> .aarch64.so
    arch_suffix = ""
    if arch == "x86_64":
        arch_suffix = ".x86_64.so"
    elif arch in ["aarch64", "arm64"]:
        arch_suffix = ".aarch64.so"
    
    # Find all base names of potential binary tests (e.g., test_mp0_private)
    # We look for ANY .so file to identify the base name, then select the correct arch.
    # Pattern: test_name.arch.so
    all_so = glob.glob(os.path.join(test_dir, "*.so"))
    binary_bases = set()
    for so in all_so:
        basename = os.path.basename(so)
        # Strip known suffixes to get the test name
        if basename.endswith(".x86_64.so"):
            binary_bases.add(basename[:-10])
        elif basename.endswith(".aarch64.so"):
            binary_bases.add(basename[:-11])
        elif basename.endswith(".so"): # Legacy fallback
            binary_bases.add(basename[:-3])

    final_test_files = []
    
    # Process binaries first
    for base in binary_bases:
        target_so = os.path.join(test_dir, base + arch_suffix)
        legacy_so = os.path.join(test_dir, base + ".so")
        
        final_target = None
        if arch_suffix and os.path.exists(target_so):
            final_target = target_so
        elif os.path.exists(legacy_so):
            final_target = legacy_so
            
        if final_target:
             final_test_files.append(final_target)
             # If we loaded a binary, DO NOT load the corresponding source .py
             py_source = os.path.join(test_dir, base + ".py")
             if py_source in py_files:
                 py_files.remove(py_source)
        else:
            print(f"[WARN] No suitable test binary found for {base} on {arch}")
            print(f"[HINT] Expected {base}{arch_suffix} or {base}.so")

    # Add remaining python files
    final_test_files.extend(list(py_files))
    
    # Sort for deterministic order
    test_files = sorted(final_test_files)

    for py_file in test_files:
        if os.path.basename(py_file) == "setup.py":
            continue
        
        # Determine module name (strip extension)
        filename = os.path.basename(py_file)
        if filename.endswith(".py"):
            module_name = filename[:-3]
        elif filename.endswith(".so"):
            # For .so, we need to handle the complex suffixes
             if filename.endswith(".x86_64.so"):
                module_name = filename[:-10]
             elif filename.endswith(".aarch64.so"):
                module_name = filename[:-11]
             else:
                module_name = filename[:-3]

        spec = importlib.util.spec_from_file_location(module_name, py_file)
        if spec and spec.loader:
            module = importlib.util.module_from_spec(spec)
            try:
                spec.loader.exec_module(module)
            except ImportError as e:
                print(f"\n[WARN] Failed to load test module {module_name}: {e}")

def load_script_tests(test_dir):
    for txt_file in glob.glob(os.path.join(test_dir, "*.txt")):
        test_name = os.path.basename(txt_file)[:-4]
        run_script_test(test_name, txt_file)

if __name__ == "__main__":
    # Ensure test directory exists
    if not os.path.isdir(TEST_DIR):
        print(f"Warning: Test directory '{TEST_DIR}' not found.")
        sys.exit(0)

    # Load tests
    load_script_tests(TEST_DIR)
    load_python_tests(TEST_DIR)

    # Run tests
    run_tests()
