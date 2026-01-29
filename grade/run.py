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
    
    for py_file in glob.glob(os.path.join(test_dir, "*.py")):
        if os.path.basename(py_file) == "setup.py":
            continue
        module_name = os.path.basename(py_file)[:-3]
        spec = importlib.util.spec_from_file_location(module_name, py_file)
        if spec and spec.loader:
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)

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
