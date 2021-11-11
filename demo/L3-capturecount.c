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
#include <mastik/l3.h>

#define SAMPLES 1000


int main(int ac, char **av) {
  delayloop(3000000000U);

  l3pp_t l3 = l3_prepare(NULL, NULL);

  int nsets = l3_getSets(l3);
  int nmonitored = nsets/64;

  for (int i = 17; i < nsets; i += 64)
    l3_monitor(l3, i);


  uint16_t *res = calloc(SAMPLES * nmonitored, sizeof(uint16_t));
  for (int i = 0; i < SAMPLES * nmonitored; i+= 4096/sizeof(uint16_t))
    res[i] = 1;
  
  l3_repeatedprobecount(l3, SAMPLES, res, 0);

  for (int i = 0; i < SAMPLES; i++) {
    for (int j = 0; j < nmonitored; j++) {
      printf("%4d ", res[i*nmonitored + j]);
    }
    putchar('\n');
  }

  free(res);
  l3_release(l3);
}
