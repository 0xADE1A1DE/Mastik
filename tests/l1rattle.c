#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
 char *p;
int main(int c, char **v) {
  p = mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  while (1) {
    for (int i = 0; i < 64000; i++)
      p[1880]++;
    for (int i = 0; i < 64000; i++)
      p[880]++;
  }
}
