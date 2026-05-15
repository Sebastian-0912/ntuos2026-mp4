from gradelib import *

r = Runner(save("mp4_bigfile_private.out"))

@test(0, "Testing large files (private)")
def test_bigfile_private():
    r.run_qemu(shell_script([
        'bigfile_private'
    ]), timeout=1800)

@test(10, "Large files (private): 18000 blocks", parent=test_bigfile_private)
def test_bigfile_private1():
    r.match(r'^private testcase 1: ok$')
