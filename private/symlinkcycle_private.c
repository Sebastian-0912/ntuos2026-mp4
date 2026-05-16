// Private cycle testcases (TA-held, not in student template).
// To wire in: add `$P/_symlinkcycle_private` to UPROGS and add a build
// rule for `private/`, then re-run mkfs.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define fail(msg) do { printf("FAILURE: " msg "\n"); failed = 1; goto done; } while(0)
static int failed = 0;

static void private1(void);
static void private2(void);
static void private3(void);
static void private4(void);
static void cleanup(void);

int
main(int argc, char *argv[])
{
  cleanup();
  private1();
  cleanup();
  private2();
  cleanup();
  private3();
  cleanup();
  private4();
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

// Two different links enter the same cycle:
//   a -> c, b -> c, c -> d, d -> c
// Both open(a) and open(b) must fail: each path follows a/b -> c -> d -> c,
// hitting c a second time -> cycle.
static void
private1(void)
{
  int fd;
  mkdir("/cyc");
  if(symlink("/cyc/c", "/cyc/a") < 0) fail("symlink a->c failed");
  if(symlink("/cyc/c", "/cyc/b") < 0) fail("symlink b->c failed");
  if(symlink("/cyc/d", "/cyc/c") < 0) fail("symlink c->d failed");
  if(symlink("/cyc/c", "/cyc/d") < 0) fail("symlink d->c failed");

  fd = open("/cyc/a", O_RDWR);
  if(fd >= 0){ close(fd); fail("open via a (shared cycle) should fail"); }
  fd = open("/cyc/b", O_RDWR);
  if(fd >= 0){ close(fd); fail("open via b (shared cycle) should fail"); }

  printf("private testcase 1: ok\n");
done:
  return;
}

// O_NOFOLLOW on self-loop a -> a:
//   open(a, O_RDWR)             must fail  (cycle when following)
//   open(a, O_RDONLY|O_NOFOLLOW) must succeed (returns fd to symlink itself)
// Also verify fstat sees the fd as T_SYMLINK.
static void
private2(void)
{
  int fd1 = -1, fd2 = -1;
  struct stat st;
  mkdir("/cyc");

  if(symlink("/cyc/a", "/cyc/a") < 0) fail("symlink a->a failed");

  fd1 = open("/cyc/a", O_RDWR);
  if(fd1 >= 0) fail("open(a) should fail on self-loop");

  fd2 = open("/cyc/a", O_RDONLY | O_NOFOLLOW);
  if(fd2 < 0) fail("open(a, O_NOFOLLOW) should succeed on self-loop");
  if(fstat(fd2, &st) != 0) fail("fstat on O_NOFOLLOW fd failed");
  if(st.type != T_SYMLINK) fail("O_NOFOLLOW fd is not T_SYMLINK");

  printf("private testcase 2: ok\n");
done:
  if(fd1 >= 0) close(fd1);
  if(fd2 >= 0) close(fd2);
}

// 3-cycle: a -> b -> c -> a
static void
private3(void)
{
  int fd;
  mkdir("/cyc");
  if(symlink("/cyc/b", "/cyc/a") < 0) fail("symlink a->b failed");
  if(symlink("/cyc/c", "/cyc/b") < 0) fail("symlink b->c failed");
  if(symlink("/cyc/a", "/cyc/c") < 0) fail("symlink c->a failed");
  fd = open("/cyc/a", O_RDWR);
  if(fd >= 0){ close(fd); fail("open 3-cycle should fail"); }
  printf("private testcase 3: ok\n");
done:
  return;
}

// 25-hop non-cyclic chain a -> b -> ... -> y -> z (z is a real file)
static void
private4(void)
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

  printf("private testcase 4: ok\n");
done:
  if(fd >= 0) close(fd);
  if(fd2 >= 0) close(fd2);
}
