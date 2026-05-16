from gradelib import *

r = Runner(save("mp4_symlinkfile_public.out"))

@test(0, "Testing symbolic links to files (public)")
def test_symlinkfile():
    r.run_qemu(shell_script([
        'symlinkfile'
    ]), timeout=30)

@test(5, "Symbolic links to files: public testcase 1", parent=test_symlinkfile)
def test_symlinkfile_public1():
    r.match(r'^public testcase 1: ok$')

@test(5, "Symbolic links to files: public testcase 2", parent=test_symlinkfile)
def test_symlinkfile_public2():
    r.match(r'^public testcase 2: ok$')
