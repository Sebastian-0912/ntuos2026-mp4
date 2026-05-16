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
  unlink("/testdir/p");
  unlink("/testdir");
  unlink("/link_to_dir");
  unlink("/nested_link");
}

// Test 1: Nested directory symlinks
static void
private1(void)
{
  int fd;
  printf("private testcase 1:\n");
  
  if(mkdir("/testdir") < 0) fail("mkdir /testdir failed");
  if(symlink("/testdir", "/link_to_dir") < 0) fail("symlink failed");
  if(symlink("/link_to_dir", "/nested_link") < 0) fail("nested symlink failed");
  
  fd = open("/nested_link/p", O_CREATE | O_RDWR);
  if(fd < 0) fail("create file via nested directory link failed");
  close(fd);
  
  printf("private testcase 1: ok\n");
done:
  return;
}

// Test 2: Chained directory components
static void
private2(void)
{
  int fd;
  printf("private testcase 2:\n");
  
  if(mkdir("/dir_a") < 0) fail("mkdir /dir_a failed");
  if(mkdir("/dir_a/dir_b") < 0) fail("mkdir /dir_a/dir_b failed");
  if(symlink("/dir_a/dir_b", "/link_b") < 0) fail("symlink /link_b failed");
  
  fd = open("/link_b/file_x", O_CREATE | O_RDWR);
  if(fd < 0) fail("create file via link_b failed");
  close(fd);

  if(open("/dir_a/dir_b/file_x", O_RDONLY) < 0) fail("file not found in real dir");

  printf("private testcase 2: ok\n");
done:
  unlink("/link_b/file_x");
  unlink("/link_b");
  unlink("/dir_a/dir_b");
  unlink("/dir_a");
  return;
}
