from gradelib import *

r = Runner(save("mp4_symlinkcycle_public.out"))

@test(0, "Testing symbolic link cycle detection (public)")
def test_symlinkcycle():
    r.run_qemu(shell_script([
        'symlinkcycle'
    ]), timeout=30)

@test(4, "Symlink cycle: self-loop", parent=test_symlinkcycle)
def test_cycle_public1():
    r.match(r'^public testcase 1: ok$')

@test(4, "Symlink cycle: 2-cycle", parent=test_symlinkcycle)
def test_cycle_public2():
    r.match(r'^public testcase 2: ok$')

@test(4, "Symlink cycle: cycle in the center (a->b->c->d->e->c)", parent=test_symlinkcycle)
def test_cycle_public5():
    r.match(r'^public testcase 5: ok$')
