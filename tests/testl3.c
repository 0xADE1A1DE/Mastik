#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <strings.h>
#include <assert.h>

#include <mastik/l3.h>

uint16_t res[400][64];

void progressNotification(int a1, int a2, void *data) {
  int *count = (int *)data;
  printf("# %d: %d/%d\n", *count, a1, a2);
  (*count)++;
}

volatile int a = 0;

int main(int c, char **v) {
  for (int i = 0; i < 1024*1024*1024; i++)
    a+=i;


  int map[64];
  srandom(time(NULL));
  struct l3info l3info;
  bzero(&l3info, sizeof(l3info));
  int setcount = 0;
  l3info.progressNotification = progressNotification;
  l3info.progressNotificationData = &setcount;
  //l3info.flags = L3FLAG_NOHUGEPAGES|L3FLAG_USEPTE;
  l3pp_t l3 = l3_prepare(&l3info, NULL);

  int nsets = l3_getSets(l3);
  printf("# Found %d sets of %d ways.  Total size %d KB\n", nsets, l3_getAssociativity(l3), (l3_getAssociativity(l3) * nsets) /16);

  if (nsets <= 0 || l3_getAssociativity(l3) <= 0)
  {
    exit(1);
  }

  for (int i = 0; i < nsets; i+= 64) 
    l3_monitor(l3, i);

  printf("# Setup done\n");
  int n = 8192/64;
  uint16_t res[8192/64*400];

  for (int i = 0; i < 400;) {
    l3_probecount(l3, res + n * i++);
    l3_bprobecount(l3, res + n * i++);
  }

  printf("# Probing done\n");
  for (int i = 0; i < 400; i++) {
    for (int j = 0; j < n; j++)
      printf("%u ", res[n * i + j]);
    putchar('\n');
  }

  return 0;
}
