from gradelib import *

r = Runner(save("mp4_symlinkdir_private.out"))

@test(0, "Testing symbolic links to directories (private)")
def test_symlinkdir_private():
    r.run_qemu(shell_script([
        'symlinkdir_private'
    ]), timeout=30)

@test(5, "Symbolic links to directories (private): nested links", parent=test_symlinkdir_private)
def test_symlinkdir_private1():
    r.match(r'^private testcase 1: ok$')

@test(5, "Symbolic links to directories (private): chained components", parent=test_symlinkdir_private)
def test_symlinkdir_private2():
    r.match(r'^private testcase 2: ok$')
