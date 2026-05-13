#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

#define fail(msg) do { printf("FAILURE: " msg "\n"); failed = 1; goto done; } while(0)
static int failed = 0;

static void
run(int target, const char *label)
{
  char buf[BSIZE];
  int fd, blocks;

  fd = open("big.file", O_CREATE | O_WRONLY);
  if(fd < 0)
    fail("cannot open big.file");

  blocks = 0;
  while(1){
    *(int*)buf = blocks;
    int cc = write(fd, buf, sizeof(buf));
    if(cc <= 0)
      break;
    blocks++;
    if(blocks % 100 == 0)
      printf(".");
    if(blocks == target)
      break;
  }
  printf("\nwrote %d blocks\n", blocks);
  if(blocks != target)
    fail("file is too small");

  printf("%s: ok\n", label);

done:
  close(fd);
  unlink("big.file");
}

int
main(void)
{
  run(270,  "public testcase 1");
  run(6666, "public testcase 2");
  exit(failed);
}
