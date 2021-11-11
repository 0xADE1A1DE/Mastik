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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <mastik/util.h>
#include <mastik/l2.h>

#define MAX_SAMPLES 100000

void usage(const char *prog) {
  fprintf(stderr, "Usage: %s <samples>\n", prog);
  exit(1);
}

int main(int ac, char **av) {
  int samples = 0;

  if (av[1] == NULL)
    usage(av[0]);
  samples = atoi(av[1]);
  if (samples < 0)
    usage(av[0]);
  if (samples > MAX_SAMPLES)
    samples = MAX_SAMPLES;
    
  l2info_t l2i = (l2info_t)malloc(sizeof(struct l2info));
  l2i->associativity = 8;
  l2i->sets = 1024;
  
  l2pp_t l2 = l2_prepare(l2i, NULL);

  int nsets = l2_getmonitoredset(l2, NULL, 0);

  int *map = calloc(nsets, sizeof(int));
  l2_getmonitoredset(l2, map, nsets);

  int rmap[l2i->sets];
  for (int i = 0; i < l2i->sets; i++)
    rmap[i] = -1;
  for (int i = 0; i < nsets; i++) 
    rmap[map[i]] = i;  

  uint16_t *res = calloc(samples * nsets, sizeof(uint16_t));
  for (int i = 0; i < samples * nsets; i+= 4096/sizeof(uint16_t))
    res[i] = 1;
  
  delayloop(3000000000U);
  l2_repeatedprobe(l2, samples, res, 0);



  for (int i = 0; i < samples; i++) {
    for (int j = 0; j < l2i->sets; j++) {
      if (rmap[j] == -1)
	printf("  0 ");
      else
	printf("%3d ", res[i*nsets + rmap[j]]);
    }
    putchar('\n');
  }

  free(map);
  free(res);
  l2_release(l2);
}
