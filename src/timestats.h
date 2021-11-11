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

#ifndef __TIMESTATS_H__
#define __TIMESTATS_H__ 1

#define TIME_MAX	1024

typedef struct ts *ts_t;

ts_t ts_alloc();
void ts_free(ts_t ts);

void ts_clear(ts_t ts);

// tm > 0
void ts_add(ts_t ts, int tm);

// tm > 0
uint32_t ts_get(ts_t ts, int tm);

uint32_t ts_outliers(ts_t ts);

int ts_median(ts_t ts);

int ts_max(ts_t ts);
int ts_percentile(ts_t ts, int percentile);

int ts_mean(ts_t ts, int scale);

#endif // __TIMESTATS_H__
