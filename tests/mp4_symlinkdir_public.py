from gradelib import *

r = Runner(save("mp4_symlinkdir_public.out"))

@test(0, "Testing symbolic links to directories (public)")
def test_symlinkdir():
    r.run_qemu(shell_script([
        'symlinkdir'
    ]), timeout=30)

@test(5, "Symbolic links to directories: public testcase 1", parent=test_symlinkdir)
def test_symlinkdir_public1():
    r.match(r'^public testcase 1: ok$')

@test(5, "Symbolic links to directories: public testcase 2", parent=test_symlinkdir)
def test_symlinkdir_public2():
    r.match(r'^public testcase 2: ok$')
