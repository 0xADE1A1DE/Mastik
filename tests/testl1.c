#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>

#include <mastik/l1.h>

uint16_t res[400][64];

volatile int a;

int main(int c, char **v) {
  int rmap[64];
  int map[64];
  srandom(time(NULL));
  l1pp_t l1 = l1_prepare(NULL);
  if (l1_getmonitoredset(l1, rmap, 64) != 64) {
    fprintf(stderr, "Expected 64 monitored sets\n");
    exit(1);
  }
  for (int i = 0; i < 64; i++)
    map[rmap[i]] = i;
  for (int i = 0; i < 1024*1024*1024; i++)
    a+=i;
  for (int i = 0; i < 400; i++) 
    res[i][0] = 1;
  for (int i = 0; i < 200; i++)  {
    l1_probe(l1, res[i*2]);
    l1_bprobe(l1, res[i*2+1]);
  }
  for (int i = 0; i < 400; i++) {
    for (int j = 0; j < 64; j++)
      printf("%u ", res[i][map[j]]);
    putchar('\n');
  }

  return 0;
}
