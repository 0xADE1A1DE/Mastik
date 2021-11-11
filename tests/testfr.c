#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <mastik/fr.h>

char a[1024];


int main(int ac, char **v) {
  fr_t fr = fr_prepare();
  uint16_t results[1000*2];
  int fd = open(v[1], O_RDONLY);
  void *p = mmap(0, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
  if (p == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  close(fd);

  fr_monitor(fr, p);
  int count = fr_repeatedprobe(fr, 1000, results, 100000);
  printf("%d\n", count);
  for (int i = 0; i < count; i++) {
    printf("%3u ", results[i]);
    if (i % 20 == 19)
      putchar('\n');
  }
  putchar('\n');

  return 0;
}

