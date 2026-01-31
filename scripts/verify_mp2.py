#!/usr/bin/env python3
import os
import shutil
import subprocess
import tempfile
import sys
from pathlib import Path

def error(msg):
    print(f"\033[91mError: {msg}\033[0m")
    sys.exit(1)

def info(msg):
    print(f"\033[92m[verify_mp2] {msg}\033[0m")

def main():
    # Ensure raw git use
    repo_root = Path.cwd().resolve()
    if not (repo_root / ".git").exists():
        error("Must run from root of xv6-ntu-mp repository")

    info(f"Repository Root: {repo_root}")
    
    # Create temp directory
    with tempfile.TemporaryDirectory(prefix="xv6_verify_") as tmp_dir:
        tmp_path = Path(tmp_dir)
        info(f"Created temp dir: {tmp_path}")
        
        # 1. Clone the current repository to temp dir
        info("Cloning repository to temp dir...")
        subprocess.run(["git", "clone", str(repo_root), tmp_path], check=True, stdout=subprocess.DEVNULL)
        
        # 2. Fetch/Checkout Answer Code
        info("Checking out Answer Code from ntuos2025/mp2-answer...")
        os.chdir(tmp_path)
        
        # Try to fetch the branch from the local repo (origin)'s remote refs
        # Since 'repo_root' has 'remotes/origin/ntuos2025/mp2-answer', we can fetch it by path.
        try:
            # Fetch directly from the repo_root path
            # Refspec: source:dest
            # Source: refs/remotes/origin/ntuos2025/mp2-answer (path in source repo)
            # Dest: mp2-answer (local branch in temp repo)
            info(f"Fetching answer branch from {repo_root}...")
            subprocess.run(
                ["git", "fetch", str(repo_root), "refs/remotes/origin/ntuos2025/mp2-answer:mp2-answer"],
                check=True, 
                stderr=subprocess.PIPE
            )
            info("  Fetch successful.")
        except subprocess.CalledProcessError as e:
            info(f"  Fetch failed: {e.stderr.decode()}")
            # Fallback: maybe it's a local branch in source?
            try:
                subprocess.run(
                    ["git", "fetch", str(repo_root), "ntuos2025/mp2-answer:mp2-answer"],
                    check=True
                )
            except subprocess.CalledProcessError:
                 error("Could not fetch mp2-answer branch from source repo.")
            
        # 3. Checkout Answer Files onto Template
        # Directories to verify: kernel, user, mkfs, test
        folders_to_checkout = ["kernel", "user", "mkfs", "test"]
        
        # Determine correct test folder name
        res = subprocess.run(["git", "ls-tree", "-d", "mp2-answer", "test"], capture_output=True, text=True)
        if "test" not in res.stdout:
             res2 = subprocess.run(["git", "ls-tree", "-d", "mp2-answer", "tests"], capture_output=True, text=True)
             if "tests" in res2.stdout:
                 folders_to_checkout[3] = "tests"
                 info("  Detected 'tests' folder in answer branch.")
             else:
                 info("  ! Warning: Neither 'test' nor 'tests' found in MP2 answer branch.")

        info(f"Overwriting with files from mp2-answer: {folders_to_checkout}")
        cmd = ["git", "checkout", "mp2-answer", "--"] + folders_to_checkout
        subprocess.run(cmd, check=True)
        
        # 4. Patch conf/mp.mk
        info("Patching conf/mp.mk...")
        mp_mk = tmp_path / "conf/mp.mk"
        with open(mp_mk, "a") as f:
            f.write("\n# MP2 Integration\n")
            f.write("OBJS += $K/slab.o $K/debug.o\n")
            f.write("UPROGS += $U/_mp2\n")
            f.write("UPROGS += $U/_debugswitch\n")
            f.write("UPROGS += $U/_prepare\n")
            f.write("UPROGS += $U/_checkstr\n")
            f.write("UPROGS += $U/_gah\n")
            f.write("UPROGS += $U/_oak\n")
            f.write("UPROGS += $U/_oap\n")
            f.write("UPROGS += $U/_tee\n")

        # 4.5 Patch mp.sh to use test/run_mp2.py
        run_py = tmp_path / "test/run_mp2.py"
        if not run_py.exists():
             run_py = tmp_path / "tests/run_mp2.py"
        
        if run_py.exists():
            info(f"Patching mp.sh to use {run_py.relative_to(tmp_path)}...")
            mpsh = tmp_path / "mp.sh"
            mpsh_content = mpsh.read_text()
            mpsh_content = mpsh_content.replace("python3 grade/run.py", f"python3 {run_py.relative_to(tmp_path)}")
            mpsh.write_text(mpsh_content)
        else:
            info("  ! run_mp2.py not found, using default mp.sh")

        # 5. Run Grading
        info("Starting build and grade (running ALL tests)...")
        subprocess.run(["make", "clean"], stdout=subprocess.DEVNULL)
        
        res = subprocess.run(["./mp.sh", "grade", "all"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        print(res.stdout)
        
        if res.returncode == 0:
            if "Score: 105" in res.stdout or "Score: 100/100" in res.stdout:
                info("SUCCESS: Grading script executed successfully with FULL MARKS.")
            else:
                info("Grading executed but score is not max. Check output.")
        else:
            error(f"Grading script failed with code {res.returncode}")

if __name__ == "__main__":
    main()
