#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define fail(msg) do { printf("FAILURE: " msg "\n"); failed = 1; goto done; } while(0)
static int failed = 0;

static void public12(void);
static void cleanup(void);

int
main(int argc, char *argv[])
{
  cleanup();
  public12();
  cleanup();
  exit(failed);
}

static void
cleanup(void)
{
  unlink("/testsymlink2/p");
  unlink("/testsymlink3/q");
  unlink("/testsymlink2");
  unlink("/testsymlink3");
}

static void
public12(void)
{
  int r, fd1 = -1, fd2 = -1;
  char c = 0, c2 = 0;

  mkdir("/testsymlink2");
  mkdir("/testsymlink3");

  fd1 = open("/testsymlink2/p", O_CREATE | O_RDWR);
  if(fd1 < 0) fail("failed to open p");

  r = symlink("/testsymlink2", "/testsymlink3/q");
  if(r < 0) fail("symlink q -> /testsymlink2 failed");

  fd2 = open("/testsymlink3/q/p", O_RDWR);
  if(fd2 < 0) fail("failed to open /testsymlink3/q/p");

  printf("public testcase 1: ok\n");

  c = '#';
  if(write(fd1, &c, 1) != 1) fail("failed to write to /testsymlink2/p");
  if(read(fd2, &c2, 1) != 1) fail("failed to read from /testsymlink3/q/p");
  if(c != c2) fail("value mismatch via symlink");

  printf("public testcase 2: ok\n");
done:
  close(fd1);
  close(fd2);
}
