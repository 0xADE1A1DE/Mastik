/*
 * Copyright 2016 CSIRO
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

#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mastik/symbol.h>
#include <mastik/ff.h>
#include <mastik/util.h>

#define SAMPLES 100000
#define SLOT	2000
#define THRESHOLD 100

#define BINARY "gpg-1.4.13"

char *monitor[] = {
  "mpih-mul.c:85",
  "mpih-mul.c:271",
  "mpih-div.c:356"
};
int nmonitor = sizeof(monitor)/sizeof(monitor[0]);

int main(int ac, char **av) {
#ifdef HAVE_DEBUG_SYMBOLS
  delayloop(2000000000);
  ff_t ff = ff_prepare();
  for (int i = 0; i < nmonitor; i++) {
    uint64_t offset = sym_getsymboloffset(BINARY, monitor[i]);
    if (offset == ~0ULL) {
      fprintf(stderr, "Cannot find %s in %s\n", monitor[i], BINARY);
      exit(1);
    } 
    ff_monitor(ff, map_offset(BINARY, offset));
  }

  uint16_t *res = malloc(SAMPLES * nmonitor * sizeof(uint16_t));
  for (int i = 0; i < SAMPLES * nmonitor ; i+= 4096/sizeof(uint16_t))
    res[i] = 1;
  ff_probe(ff, res);

  int l;
  do {
    l = ff_trace(ff, SAMPLES, res, SLOT, THRESHOLD, 500);
  } while (l < 10000);
  for (int i = 0; i < l; i++) {
    for (int j = 0; j < nmonitor; j++)
      printf("%d ", res[i * nmonitor + j]);
    putchar('\n');
  }

  free(res);
  ff_release(ff);
#else // HAVE_DEBUG_SYMBOLS
  fprintf(stderr, "%s: No dupport for debug symbols\n", av[0]);
  exit(1);
#endif
}
