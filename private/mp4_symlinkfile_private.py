from gradelib import *

r = Runner(save("mp4_symlinkfile_private.out"))

@test(0, "Testing symbolic links to files (private)")
def test_symlinkfile_private():
    r.run_qemu(shell_script([
        'symlinkfile_private'
    ]), timeout=30)

@test(5, "Symbolic links (private): nested directories", parent=test_symlinkfile_private)
def test_symlinkfile_private1():
    r.match(r'^private testcase 1: ok$')

@test(5, "Symbolic links (private): deferred target creation", parent=test_symlinkfile_private)
def test_symlinkfile_private2():
    r.match(r'^private testcase 2: ok$')
