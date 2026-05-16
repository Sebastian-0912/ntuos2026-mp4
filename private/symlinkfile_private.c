#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define fail(msg) do { printf("FAILURE: " msg "\n"); failed = 1; goto done; } while(0)
static int failed = 0;

static void private1(void);
static void private2(void);
static void cleanup(void);

int
main(int argc, char *argv[])
{
  cleanup();
  private1();
  cleanup();
  private2();
  cleanup();
  exit(failed);
}

static void
cleanup(void)
{
  unlink("/dir1/dir2/a");
  unlink("/dir1/dir2/b");
  unlink("/dir1/dir2");
  unlink("/dir1");
  unlink("/link_later");
  unlink("/target_later");
}

// Test 1: Symlink in nested directories
static void
private1(void)
{
  int fd1, fd2;
  char buf[4] = "xyz";
  char got[4] = {0};

  printf("private testcase 1:\n");
  if(mkdir("/dir1") < 0) fail("mkdir /dir1 failed");
  if(mkdir("/dir1/dir2") < 0) fail("mkdir /dir1/dir2 failed");
  
  fd1 = open("/dir1/dir2/a", O_CREATE | O_RDWR);
  if(fd1 < 0) fail("create /dir1/dir2/a failed");
  write(fd1, buf, 3);
  close(fd1);

  if(symlink("/dir1/dir2/a", "/dir1/dir2/b") < 0) fail("symlink b -> a failed");
  
  fd2 = open("/dir1/dir2/b", O_RDONLY);
  if(fd2 < 0) fail("open /dir1/dir2/b failed");
  read(fd2, got, 3);
  if(strcmp(buf, got) != 0) fail("content mismatch");
  close(fd2);

  printf("private testcase 1: ok\n");
done:
  return;
}

// Test 2: Symlink to a file created later
static void
private2(void)
{
  int fd;
  char buf[4] = "abc";
  char got[4] = {0};

  printf("private testcase 2:\n");
  if(symlink("/target_later", "/link_later") < 0) fail("symlink failed");
  
  // opening should fail now
  if(open("/link_later", O_RDONLY) >= 0) fail("open should have failed");

  // create target
  fd = open("/target_later", O_CREATE | O_RDWR);
  if(fd < 0) fail("create target failed");
  write(fd, buf, 3);
  close(fd);

  // now it should work
  fd = open("/link_later", O_RDONLY);
  if(fd < 0) fail("open link_later failed after target creation");
  read(fd, got, 3);
  if(strcmp(buf, got) != 0) fail("content mismatch");
  close(fd);

  printf("private testcase 2: ok\n");
done:
  return;
}
