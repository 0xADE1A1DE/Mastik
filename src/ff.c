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
#include <strings.h>

#include <mastik/low.h>
#include <mastik/ff.h>
#include <mastik/util.h>

#include "vlist.h"
#include "timestats.h"

#define DEFAULT_THRESHOLD_CAPACITY 16

#define THRESHOLD_SAMPLES 1000

struct ff { 
  vlist_t vl;
  int modified;
  uint16_t *thresholds;
  int thresholdcap;
};


static inline void ensurecapacity(ff_t ff) {
  if (ff->thresholdcap >= vl_len(ff->vl))
    return;
  int newcap = ff->thresholdcap + vl_len(ff->vl);
  ff->thresholds = realloc(ff->thresholds, newcap * sizeof(uint16_t));
  bzero(ff->thresholds + ff->thresholdcap, vl_len(ff->vl) * sizeof(uint16_t));
}

ff_t ff_prepare() {
  ff_t rv = malloc(sizeof(struct ff));
  rv->vl = vl_new();
  rv->modified = 0;
  rv->thresholds = calloc(DEFAULT_THRESHOLD_CAPACITY, sizeof(uint16_t));
  rv->thresholdcap = DEFAULT_THRESHOLD_CAPACITY;
  return rv;
}

void ff_release(ff_t ff) {
  vl_free(ff->vl);
  ff->vl = NULL;
  free(ff->thresholds);
  ff->thresholds = NULL;
  free(ff);
}


int ff_monitor(ff_t ff, void *adrs) {
  assert(ff != NULL);
  assert(adrs != NULL);
  if (vl_find(ff->vl, adrs) >= 0)
    return 0;
  int ind = vl_push(ff->vl, adrs);
  if (!ff->modified) {
    ensurecapacity(ff);
    ff->thresholds[ind] = 0;
  }
  return 1;
}


int ff_unmonitor(ff_t ff, void *adrs) {
  assert(ff != NULL);
  assert(adrs != NULL);
  int i = vl_find(ff->vl, adrs);
  if (i < 0)
    return 0;
  vl_del(ff->vl, i);
  ff->modified = 1;
  return 1;
}


int ff_getmonitoredset(ff_t ff, void **adrss, int nlines) {
  assert(ff != NULL);

  if (adrss != NULL) {
    int l = vl_len(ff->vl);
    if (l > nlines)
      l = nlines;
    for (int i = 0; i < l; i++)
      adrss[i] = vl_get(ff->vl, i);
  }
  return vl_len(ff->vl);
}

void ff_randomise(ff_t ff) {
  assert(ff != NULL);
  assert(0);
}

static uint16_t inline probeaddr(void *addr) {
  uint32_t start = rdtscp();
  mfence();
  clflush(addr);
  mfence();
  uint32_t res = rdtscp() - start;
  return res > UINT16_MAX ? UINT16_MAX : res;
}

void ff_probe(ff_t fr, uint16_t *results) {
  assert(fr != NULL);
  assert(results != NULL);
  int l = vl_len(fr->vl);
  for (int i = 0; i < l; i++) 
    results[i] = probeaddr(vl_get(fr->vl, i));
}

static void setthresholds(ff_t ff, int force) {
  ensurecapacity(ff);
  int l = vl_len(ff->vl);
  if (ff->modified || force)
    bzero(ff->thresholds, l * sizeof(uint16_t));
  ts_t samples = NULL;
  for (int i = 0; i < l; i++) {
    if (ff->thresholds[i] != 0)
      continue;
    if (samples == NULL)
      samples = ts_alloc();
    ts_clear(samples);
    void *addr = vl_get(ff->vl, i);
    for (int j = 0; j < THRESHOLD_SAMPLES; j++) {
      ts_add(samples, probeaddr(addr));
      delayloop(10000);
    }
    ff->thresholds[i] = ts_percentile(samples, 99) + 6;
  }
  if (samples != NULL)
    ts_free(samples);
  ff->modified = 0;
}

void ff_setthresholds(ff_t ff) {
  setthresholds(ff, 1);
}

int ff_getthreshold(ff_t ff, int index) {
  ensurecapacity(ff);
  if (index < 0 || index > vl_len(ff->vl))
    return -1;
  return ff->thresholds[index];
}


inline int is_active(uint16_t *results, int len, uint16_t *thresholds) {
  if (thresholds == NULL)
    return 1;
  for (int i = 0; i < len; i++)
    if (results[i] < thresholds[i] * 2 && results[i] > thresholds[i])
      return 1;
  return 0;
}

int ff_fastrepeatedprobe(ff_t ff, int max_records, uint16_t *results) {
  uint32_t start = rdtscp();
  int l = vl_len(ff->vl);
  for (int i = 0; i < max_records; i++) {
    for (int j = 0; j < l; j++)  {
      mfence();
      clflush(vl_get(ff->vl, j));
      mfence();
      uint32_t end = rdtscp();
      *results++ =  (end - start) > UINT16_MAX ? UINT16_MAX : end-start;
      start = end;
    }
  }
  return max_records;
}

int ff_repeatedprobe(ff_t ff, int max_records, uint16_t *results, int slot) {
  return ff_trace(ff, max_records, results, slot, 0, max_records);
}


int ff_trace(ff_t ff, int max_records, uint16_t *results, int slot, int threshold, int max_idle) {
  assert(ff != NULL);
  assert(results != NULL);

  if (max_records == 0)
    return 0;
  if (max_idle == 0)
    max_idle = max_records;

  uint16_t *thresholds = NULL;
  if (threshold) {
    setthresholds(ff, 0);
    thresholds = ff->thresholds;
  }

  int len = vl_len(ff->vl);

  // Wait to hit threshold
  uint64_t prev_time = rdtscp64();
  ff_probe(ff, results);
  do {
    if (slot > 0) {
      do {
	prev_time += slot;
      } while (slotwait(prev_time));
    }
    ff_probe(ff, results);
  } while (!is_active(results, len, thresholds));

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
      ff_probe(ff, results);
      if (is_active(results, len, thresholds))
	idle_count = 0;
    }
    prev_time += slot;
    missed = slotwait(prev_time);
  }
  return count;
}

  
