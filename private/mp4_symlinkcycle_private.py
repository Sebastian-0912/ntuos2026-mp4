from gradelib import *

r = Runner(save("mp4_symlinkcycle_private.out"))

@test(0, "Testing symbolic link cycle detection (private)")
def test_symlinkcycle_private():
    r.run_qemu(shell_script([
        'symlinkcycle_private'
    ]), timeout=30)

@test(2, "Symlink cycle (private): shared cycle entry", parent=test_symlinkcycle_private)
def test_cycle_private1():
    r.match(r'^private testcase 1: ok$')

@test(2, "Symlink cycle (private): O_NOFOLLOW bypasses cycle", parent=test_symlinkcycle_private)
def test_cycle_private2():
    r.match(r'^private testcase 2: ok$')

@test(3, "Symlink cycle (private): 3-cycle", parent=test_symlinkcycle_private)
def test_cycle_private3():
    r.match(r'^private testcase 3: ok$')

@test(3, "Symlink (private): 25-hop non-cyclic chain", parent=test_symlinkcycle_private)
def test_cycle_private4():
    r.match(r'^private testcase 4: ok$')
