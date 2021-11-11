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
#include <strings.h>
#include <assert.h>

#include "timestats.h"



struct ts {
  uint32_t data[TIME_MAX];
};


static ts_t lastfree = NULL;

ts_t ts_alloc() {
  ts_t rv = lastfree; 
  if (rv == NULL)
    rv = (ts_t)malloc(sizeof(struct ts));
  else
    lastfree = NULL;
  ts_clear(rv);
  return rv;
}

void ts_free(ts_t ts) {
  if (lastfree == NULL)
    lastfree = ts;
  else
    free(ts);
}

void ts_clear(ts_t ts) {
  bzero(ts, sizeof(struct ts));
}

void ts_add(ts_t ts, int tm) {
  if (tm < TIME_MAX &&  tm >= 0)
    ts->data[tm]++;
  else
    ts->data[0]++;
}

uint32_t ts_get(ts_t ts, int tm) {
  return tm < TIME_MAX && tm > 0 ? ts->data[tm] : 0;
}

uint32_t ts_outliers(ts_t ts) {
  return ts->data[0];
}


int ts_median(ts_t ts) {
  int c = 0;
  for (int i = 0; i < TIME_MAX; i++)
    c += ts->data[i];
  c = (c + 1) / 2;
  for (int i = 1; i < TIME_MAX; i++)
    if ((c -= ts->data[i]) < 0)
      return i;
  return 0;
}

int ts_max(ts_t ts) {
  for (int i = TIME_MAX; --i; )
    if (ts->data[i] != 0)
      return i;
  return 0;
}

int ts_percentile(ts_t ts, int percentile) {
  int c = 0;
  for (int i = 0; i < TIME_MAX; i++)
    c += ts->data[i];
  c = (c * percentile + 50) / 100;
  for (int i = 1; i < TIME_MAX; i++)
    if ((c -= ts->data[i]) < 0)
      return i;
  return ts_max(ts);
}
  

int ts_mean(ts_t ts, int scale) {
  uint64_t sum = 0;
  int count = 0;
  for (int i = 0; i < TIME_MAX; i++) {
    count += ts->data[i];
    sum += i* (uint64_t)ts->data[i];
  }
  return (int)((sum * scale)/count);
}
