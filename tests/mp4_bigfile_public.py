from gradelib import *

r = Runner(save("out/mp4_bigfile_public.out"))

@test(0, "Testing large files (public)")
def test_bigfile():
    r.run_qemu(shell_script([
        'bigfile'
    ]), timeout=1800)

@test(6, "Large files: public testcase 1", parent=test_bigfile)
def test_bigfile_public1():
    r.match(r'^public testcase 1: ok$')

@test(6, "Large files: public testcase 2", parent=test_bigfile)
def test_bigfile_public2():
    r.match(r'^public testcase 2: ok$')
