#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define fail(msg) do { printf("FAILURE: " msg "\n"); failed = 1; goto done; } while(0)
static int failed = 0;

static void public1(void);
static void public2(void);
static void cleanup(void);

int
main(int argc, char *argv[])
{
  cleanup();
  public1();
  cleanup();
  public2();
  cleanup();
  exit(failed);
}

static void
cleanup(void)
{
  unlink("/testsymlink/a");
  unlink("/testsymlink/b");
  unlink("/testsymlink/1");
  unlink("/testsymlink/2");
  unlink("/testsymlink/3");
  unlink("/testsymlink/4");
  unlink("/testsymlink");
}

static int
stat_slink(const char *pn, struct stat *st)
{
  int fd = open(pn, O_RDONLY | O_NOFOLLOW);
  if(fd < 0)
    return -1;
  if(fstat(fd, st) != 0){
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

static void
public1(void)
{
  int r, fd1 = -1, fd2 = -1;
  char buf[4] = {'a','b','c','d'};
  char c = 0;
  struct stat st;

  printf("public testcase 1:\n");

  mkdir("/testsymlink");

  fd1 = open("/testsymlink/a", O_CREATE | O_RDWR);
  if(fd1 < 0) fail("failed to open a");

  r = symlink("/testsymlink/a", "/testsymlink/b");
  if(r < 0) fail("symlink b -> a failed");

  if(stat_slink("/testsymlink/b", &st) != 0)
    fail("failed to stat b");
  if(st.type != T_SYMLINK)
    fail("b is not a symlink");

  if(write(fd1, buf, sizeof(buf)) != 4)
    fail("failed to write to a");

  fd2 = open("/testsymlink/b", O_RDWR);
  if(fd2 < 0) fail("failed to open b");
  read(fd2, &c, 1);
  if(c != 'a') fail("b and a are not the same");

  printf("public testcase 1: ok\n");
done:
  close(fd1);
  close(fd2);
}

static void
public2(void)
{
  int r, fd1 = -1, fd2 = -1;
  char c = 0, c2 = 0;

  printf("public testcase 2:\n");

  mkdir("/testsymlink");

  r = symlink("/testsymlink/2", "/testsymlink/1");
  if(r) fail("failed to link 1->2");
  r = symlink("/testsymlink/3", "/testsymlink/2");
  if(r) fail("failed to link 2->3");
  r = symlink("/testsymlink/4", "/testsymlink/3");
  if(r) fail("failed to link 3->4");

  fd1 = open("/testsymlink/4", O_CREATE | O_RDWR);
  if(fd1 < 0) fail("failed to create 4");
  fd2 = open("/testsymlink/1", O_RDWR);
  if(fd2 < 0) fail("failed to open 1");

  c = '#';
  if(write(fd2, &c, 1) != 1) fail("failed to write to 1");
  if(read(fd1, &c2, 1) != 1) fail("failed to read from 4");
  if(c != c2) fail("value mismatch between 1 and 4");

  printf("public testcase 2: ok\n");
done:
  close(fd1);
  close(fd2);
}
