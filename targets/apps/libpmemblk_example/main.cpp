#include <errno.h>
#include <fcntl.h>
#include <libpmemblk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* size of the pmemblk pool -- 1 GB */
#define POOL_SIZE ((size_t)(1 << 30))

/* size of each element in the pmem pool */
#define ELEMENT_SIZE 512

int main(int argc, char* argv[]) {
  const char path[] = "./blkexample.pool";
  PMEMblkpool* pbp;
  size_t nelements;
  char buf[ELEMENT_SIZE];

  /* create the pmemblk pool or open it if it already exists */
  pbp = pmemblk_create(path, ELEMENT_SIZE, POOL_SIZE, 0666);

  if (pbp == NULL)
    pbp = pmemblk_open(path, ELEMENT_SIZE);

  if (pbp == NULL) {
    perror(path);
    exit(1);
  }

  /* how many elements fit into the file? */
  nelements = pmemblk_nblock(pbp);
  printf("file holds %zu elements", nelements);

  /* store a block at index 5 */
  strcpy(buf, "hello, world");
  if (pmemblk_write(pbp, buf, 5) < 0) {
    perror("pmemblk_write");
    exit(1);
  }

  /* read the block at index 10 (reads as zeros initially) */
  if (pmemblk_read(pbp, buf, 10) < 0) {
    perror("pmemblk_read");
    exit(1);
  }

  /* zero out the block at index 5 */
  if (pmemblk_set_zero(pbp, 5) < 0) {
    perror("pmemblk_set_zero");
    exit(1);
  }

  /* ... */

  pmemblk_close(pbp);
}
