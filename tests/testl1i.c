#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <mastik/l1i.h>

uint16_t res[400][64];

volatile int a;

int main(int c, char **v) {
  int map[64];
  int rmap[64];
  srandom(time(NULL));
  l1ipp_t l1 = l1i_prepare();

  if (l1i_getmonitoredset(l1, rmap, 64) != 64) {
    fprintf(stderr, "Expected 64 sets\n");
    exit(1);
  }

  for (int i = 0; i < 1024*1024*1024; i++)
    a+=i;

  putchar('#');
  for (int i = 0; i < 64; i++) {
    printf("%2d ", rmap[i]);
    map[rmap[i]] = i;
  }
  putchar('\n');

  for (int i = 0; i < 400; i++) 
    res[i][0] = 1;
  for (int i = 0; i < 400; i++) 
    l1i_probe(l1, res[i]);
  for (int i = 0; i < 400; i++) {
    for (int j = 0; j < 64; j++)
      printf("%u ", res[i][map[j]]);
    putchar('\n');
  }

  return 0;
}
