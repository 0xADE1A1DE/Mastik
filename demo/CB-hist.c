/*
 * Copyright 2017 CSIRO
 *
 * This file is part of Mastik.
 *
 * Mastik is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mastik is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mastik.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mastik/cb.h>

#define COUNT 1000000
#define RANGE 512
#define OFFSETS 16

int *hist;
uint32_t *buf;

void measure(cb_t cb, int offset, int hoffset, int count, int range) {
  cb_monitor(cb, offset, 0);
  cb_repeatedprobe(cb, 10, buf);
  cb_repeatedprobe(cb, count, buf);
  for (int i = 0; i <  count; i++)  {
    int ind = buf[i];
    if (ind >= range)
      ind = 0;
    hist[hoffset * range + ind]++;
  }
}

void usage(char *p) {
  fprintf(stderr, "Usage: %s [-B|F] [-c count] [-n rounds] [-r range] [-b base offset] [-o offsets] [-s skip]\n", p);
  exit(1);
}

int main(int ac, char **av) {
  int mode = CBT_BANK_CONFLICT;
  int opt;
  int count = 1000000;
  int rounds = 1;
  int range = 1024;
  int base = 0;
  int offsets = 8;
  int skip = 4;

  while ((opt = getopt(ac, av, "BFc:n:r:b:o:s:")) != -1) {
    switch (opt) {
      case 'B': mode = CBT_BANK_CONFLICT; break;
      case 'F': mode = CBT_FALSE_DEPENDENCY; break;
      case 'c': count = atoi(optarg); break;
      case 'n': rounds = atoi(optarg); break;
      case 'r': range = atoi(optarg); break;
      case 'b': base = atoi(optarg); break;
      case 'o': offsets = atoi(optarg); break;
      case 's': skip = atoi(optarg); break;
      case'?':
      default: usage(av[0]);
    }
  }

  buf = malloc(sizeof(uint32_t) * count);
  memset(buf, '\0', sizeof(uint32_t) * count);
  hist = malloc(sizeof(int)*range*offsets);
  memset(hist, '\0', sizeof(int)*range*offsets);


  cb_t cb = cb_prepare(mode);
  for (int r = 0; r < rounds; r++)
    for (int i = 0; i < offsets; i++)
      measure(cb, base + i * skip, i, count, range);

  for (int i = 0; i < range; i++) {
    printf("%4d", i);
    for (int j = 0; j < offsets; j++)
      printf("  %8d" , hist[j * range + i]);
    putchar('\n');
  }
}




