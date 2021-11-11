/*
 * Copyright 2020 CSIRO
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
#ifndef __TRANSIENT_H__
#define __TRANSIENT_H__ 1

#include <stdlib.h>
#include <mastik/low.h>

#ifndef MASTIK_FRCC_THRESHOLD
#define MASTIK_FRCC_THRESHOLD 100
#endif

#ifndef MASTIK_FRCC_STEP
#define MASTIK_FRCC_STEP 4096
#endif

#ifndef MASTIK_FRCC_LEN
#define MASTIK_FRCC_LEN 256
#endif

#ifndef MASTIK_FRCC_SKIP
#define MASTIK_FRCC_SKIP 171
#endif

// Flush+Reload Covert Channel
typedef void *frcc_t;


static inline frcc_t frcc_new() {
  return calloc(MASTIK_FRCC_LEN * MASTIK_FRCC_STEP, 1);
}

static inline void frcc_delete(frcc_t frcc) {
  free(frcc);
}

static inline void frcc_send(frcc_t frcc, int ind) {
  memaccess((char *)frcc + ind * MASTIK_FRCC_STEP);
}

static inline void frcc_prepare(frcc_t frcc) {
  for (int i = 0; i < MASTIK_FRCC_LEN; i++) {
    clflush((char *)frcc + i*MASTIK_FRCC_STEP);
  }
}

static inline void frcc_recv(frcc_t frcc, int *data) {
  for (int i = 0; i < MASTIK_FRCC_LEN; i++) {
    int ind = (i * MASTIK_FRCC_SKIP)%MASTIK_FRCC_LEN;
    if (memaccesstime(frcc + ind * MASTIK_FRCC_STEP) < MASTIK_FRCC_THRESHOLD)
      data[ind]++;
  }
}

static inline int frcc_recv1(frcc_t frcc) {
  int min = memaccesstime(frcc);
  int minind = 0;
  for (int i = 1; i < MASTIK_FRCC_LEN; i++) {
    int ind = (i * MASTIK_FRCC_SKIP)%MASTIK_FRCC_LEN;
    int atime = memaccesstime(frcc + ind * MASTIK_FRCC_STEP);
    if (atime < min) {
      min = atime;
      minind = ind;
    }
  }
  if (min < MASTIK_FRCC_THRESHOLD)
    return minind;
  return -1;
}

#endif // __TRANSIENT_H__
