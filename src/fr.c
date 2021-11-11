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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <mastik/low.h>
#include <mastik/fr.h>

#include "vlist.h"
#include "timestats.h"

struct fr { 
  vlist_t vl;
  vlist_t evict;
};



fr_t fr_prepare() {
  fr_t rv = malloc(sizeof(struct fr));
  rv->vl = vl_new();
  rv->evict = vl_new();
  return rv;
}

void fr_release(fr_t fr) {
  vl_free(fr->vl);
  fr->vl = NULL;
  vl_free(fr->evict);
  fr->evict = NULL;
  free(fr);
}


int fr_monitor(fr_t fr, void *adrs) {
  assert(fr != NULL);
  assert(adrs != NULL);
  if (vl_find(fr->vl, adrs) >= 0)
    return 0;
  vl_push(fr->vl, adrs);
  return 1;
}


int fr_unmonitor(fr_t fr, void *adrs) {
  assert(fr != NULL);
  assert(adrs != NULL);
  int i = vl_find(fr->vl, adrs);
  if (i < 0)
    return 0;
  vl_del(fr->vl, i);
  return 1;
}


int fr_getmonitoredset(fr_t fr, void **adrss, int nlines) {
  assert(fr != NULL);

  if (adrss != NULL) {
    int l = vl_len(fr->vl);
    if (l > nlines)
      l = nlines;
    for (int i = 0; i < l; i++)
      adrss[i] = vl_get(fr->vl, i);
  }
  return vl_len(fr->vl);
}

int fr_evict(fr_t fr, void *adrs) {
  assert(fr != NULL);
  assert(adrs != NULL);
  if (vl_find(fr->evict, adrs) >= 0)
    return 0;
  vl_push(fr->evict, adrs);
  return 1;
}


int fr_unevict(fr_t fr, void *adrs) {
  assert(fr != NULL);
  assert(adrs != NULL);
  int i = vl_find(fr->evict, adrs);
  if (i < 0)
    return 0;
  vl_del(fr->evict, i);
  return 1;
}


int fr_getevictedset(fr_t fr, void **adrss, int nlines) {
  assert(fr != NULL);

  if (adrss != NULL) {
    int l = vl_len(fr->evict);
    if (l > nlines)
      l = nlines;
    for (int i = 0; i < l; i++)
      adrss[i] = vl_get(fr->evict, i);
  }
  return vl_len(fr->evict);
}

void fr_randomise(fr_t fr) {
  assert(fr != NULL);
  assert(0);
}


int fr_probethreshold() {
  static char dummy;

  ts_t ts = ts_alloc();
  for (int i = 0; i < 100000; i++) {
    clflush(&dummy);
    ts_add(ts, memaccesstime(&dummy));
  }
  int res = ts_percentile(ts, 1);
  ts_free(ts);
  return res < 100 ? res - 10 : res * 9 / 10;
}






void fr_probe(fr_t fr, uint16_t *results) {
  assert(fr != NULL);
  assert(results != NULL);
  int l = vl_len(fr->vl);
  for (int i = 0; i < l; i++)  {
    void *adrs = vl_get(fr->vl, i);
    int res = memaccesstime(adrs);
    results[i] = res > UINT16_MAX ? UINT16_MAX : res;
    clflush(adrs);
  }
  l = vl_len(fr->evict);
  for (int i = 0; i < l; i++) 
    clflush(vl_get(fr->evict, i));
}

inline int is_active(uint16_t *results, int len, int threshold) {
  for (int i = 0; i < len; i++)
    if (results[i] < threshold)
      return 1;
  return threshold == 0;
}

int fr_trace(fr_t fr, int max_records, uint16_t *results, int slot, int threshold, int max_idle) {
  assert(fr != NULL);
  assert(results != NULL);

  if (max_records == 0)
    return 0;
  if (max_idle == 0)
    max_idle = max_records;

  int len = vl_len(fr->vl);

  // Wait to hit threshold
  uint64_t prev_time = rdtscp64();
  fr_probe(fr, results);
  do {
    if (slot > 0) {
      do {
	prev_time += slot;
      } while (slotwait(prev_time));
    }
    fr_probe(fr, results);
  } while (!is_active(results, len, threshold));

  int count = 1;
  int idle_count = 0;
  int missed = 0;

  while (idle_count < max_idle && count < max_records) {
    idle_count++;
    results += len;
    count++;
    if (missed) {
      for (int i = 0; i < len; i++)
	results[i] = 0;
    } else {
      fr_probe(fr, results);
      if (is_active(results, len, threshold))
	idle_count = 0;
    }
    prev_time += slot;
    missed = slotwait(prev_time);
  }
  return count;
}

int fr_repeatedprobe(fr_t fr, int max_records, uint16_t *results, int slot) {
  return fr_trace(fr, max_records, results, slot, 0, max_records);
}
  
