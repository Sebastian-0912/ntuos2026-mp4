#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define fail(msg) do { printf("FAILURE: " msg "\n"); failed = 1; goto done; } while(0)
static int failed = 0;

static void public1(void);
static void public2(void);
static void public5(void);
static void cleanup(void);

int
main(int argc, char *argv[])
{
  cleanup();
  public1();
  cleanup();
  public2();
  cleanup();
  public5();
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

// Cycle in the center: a -> b -> c -> d -> e -> c
// (a, b are non-cyclic prefix; c -> d -> e -> c forms a 3-cycle in the middle)
// open(a) must fail: follow visits a, b, c, d, e, then hits c again => cycle
static void
public5(void)
{
  int fd;
  mkdir("/cyc");
  if(symlink("/cyc/b", "/cyc/a") < 0) fail("symlink a->b failed");
  if(symlink("/cyc/c", "/cyc/b") < 0) fail("symlink b->c failed");
  if(symlink("/cyc/d", "/cyc/c") < 0) fail("symlink c->d failed");
  if(symlink("/cyc/e", "/cyc/d") < 0) fail("symlink d->e failed");
  if(symlink("/cyc/c", "/cyc/e") < 0) fail("symlink e->c failed");
  fd = open("/cyc/a", O_RDWR);
  if(fd >= 0){ close(fd); fail("open center-cycle should fail"); }
  printf("public testcase 5: ok\n");
done:
  return;
}
