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
  char path[32];
  for(int i = 0; i < 30; i++){
    path[0] = 0;
    strcpy(path, "/cyc/");
    char numbuf[8];
    int n = i, k = 0;
    if(n == 0){ numbuf[k++] = '0'; }
    else { char tmp[8]; int t = 0; while(n){ tmp[t++] = '0' + n%10; n /= 10; } while(t--) numbuf[k++] = tmp[t]; }
    numbuf[k] = 0;
    strcpy(path + 5, numbuf);
    unlink(path);
  }
  unlink("/cyc/a");
  unlink("/cyc/b");
  unlink("/cyc/c");
  unlink("/cyc");
  unlink("/cdir/A");
  unlink("/cdir/B");
  unlink("/cdir");
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

// long chain that exceeds depth threshold (no cycle, but >= 25 hops)
static void
public4(void)
{
  int fd;
  char from[16], to[16];
  mkdir("/cyc");
  for(int i = 0; i < 25; i++){
    from[0] = '/'; from[1] = 'c'; from[2] = 'y'; from[3] = 'c'; from[4] = '/';
    to[0]   = '/'; to[1]   = 'c'; to[2]   = 'y'; to[3]   = 'c'; to[4]   = '/';
    int n = i;     int k = 5; char tmp[4]; int t = 0;
    if(n == 0){ tmp[t++] = '0'; } else { while(n){ tmp[t++] = '0' + n%10; n /= 10; } }
    while(t--) from[k++] = tmp[t]; from[k] = 0;
    n = i + 1;  k = 5; t = 0;
    if(n == 0){ tmp[t++] = '0'; } else { while(n){ tmp[t++] = '0' + n%10; n /= 10; } }
    while(t--) to[k++] = tmp[t]; to[k] = 0;
    if(symlink(to, from) < 0) fail("symlink chain failed");
  }
  fd = open("/cyc/0", O_RDWR);
  if(fd >= 0){ close(fd); fail("open long chain should fail by depth limit"); }
  printf("public testcase 4: ok\n");
done:
  return;
}
