#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define fail(msg) do { printf("FAILURE: " msg "\n"); failed = 1; goto done; } while(0)
static int failed = 0;

static void public1(void);
static void public2(void);
static void public3(void);
static void public4(void);
static void cleanup(void);

int
main(int argc, char *argv[])
{
  cleanup();
  public1();
  cleanup();
  public2();
  cleanup();
  public3();
  cleanup();
  public4();
  cleanup();
  exit(failed);
}

static void
cleanup(void)
{
  char path[16];
  for(char c = 'a'; c <= 'z'; c++){
    path[0] = '/'; path[1] = 'c'; path[2] = 'y'; path[3] = 'c'; path[4] = '/';
    path[5] = c;   path[6] = 0;
    unlink(path);
  }
  unlink("/cyc");
}

// self-loop: /cyc/a -> /cyc/a
static void
public1(void)
{
  int fd;
  mkdir("/cyc");
  if(symlink("/cyc/a", "/cyc/a") < 0) fail("symlink self-loop failed");
  fd = open("/cyc/a", O_RDWR);
  if(fd >= 0){ close(fd); fail("open self-loop should fail"); }
  printf("public testcase 1: ok\n");
done:
  return;
}

// 2-cycle: a -> b -> a
static void
public2(void)
{
  int fd;
  mkdir("/cyc");
  if(symlink("/cyc/b", "/cyc/a") < 0) fail("symlink a->b failed");
  if(symlink("/cyc/a", "/cyc/b") < 0) fail("symlink b->a failed");
  fd = open("/cyc/a", O_RDWR);
  if(fd >= 0){ close(fd); fail("open 2-cycle should fail"); }
  printf("public testcase 2: ok\n");
done:
  return;
}

// 3-cycle: a -> b -> c -> a
static void
public3(void)
{
  int fd;
  mkdir("/cyc");
  if(symlink("/cyc/b", "/cyc/a") < 0) fail("symlink a->b failed");
  if(symlink("/cyc/c", "/cyc/b") < 0) fail("symlink b->c failed");
  if(symlink("/cyc/a", "/cyc/c") < 0) fail("symlink c->a failed");
  fd = open("/cyc/a", O_RDWR);
  if(fd >= 0){ close(fd); fail("open 3-cycle should fail"); }
  printf("public testcase 3: ok\n");
done:
  return;
}

// 25-hop non-cyclic chain a -> b -> ... -> y -> z (z is a real file)
// MUST succeed: not a cycle, follow chain to z, read should return z's content
static void
public4(void)
{
  int fd = -1, fd2 = -1;
  char want = '$', got = 0;
  char from[8] = "/cyc/?";
  char to[8]   = "/cyc/?";

  mkdir("/cyc");

  fd2 = open("/cyc/z", O_CREATE | O_RDWR);
  if(fd2 < 0) fail("failed to create /cyc/z");
  if(write(fd2, &want, 1) != 1) fail("failed to write to /cyc/z");
  close(fd2);
  fd2 = -1;

  for(char c = 'a'; c <= 'y'; c++){
    from[5] = c;
    to[5]   = c + 1;
    if(symlink(to, from) < 0) fail("symlink chain failed");
  }

  fd = open("/cyc/a", O_RDONLY);
  if(fd < 0) fail("open 25-hop non-cyclic chain should succeed (not a cycle)");
  if(read(fd, &got, 1) != 1) fail("read through chain failed");
  if(got != want) fail("content mismatch: chain did not reach /cyc/z");

  printf("public testcase 4: ok\n");
done:
  if(fd >= 0) close(fd);
  if(fd2 >= 0) close(fd2);
}
