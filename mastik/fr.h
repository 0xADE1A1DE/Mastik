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

#ifndef __FR_H__
#define __FR_H__ 1

typedef struct fr *fr_t;



fr_t fr_prepare();
void fr_release(fr_t fr);

int fr_monitor(fr_t fr, void *adrs);
int fr_unmonitor(fr_t fr, void *adrs);
int fr_getmonitoredset(fr_t fr, void **adrss, int nlines);
void fr_randomise(fr_t fr);

int fr_evict(fr_t fr, void *adrs);
int fr_unevict(fr_t fr, void *adrs);
int fr_getevictedset(fr_t fr, void **adrss, int nlines);

int fr_probethreshold(void);


void fr_probe(fr_t fr, uint16_t *results);


/*
 * Performs repeated calls to fr_probe. Prameters are:
 * fr          - The fr descriptor
 * max_records - The number of records to collect
 * results     - An array for results of size fr_len(fr) * max_records
 * slot        - The length of a time-slot in cycles.  Use 0 to have flexible slot size
 * threshold   - Activity threshold. If >0, capture starts when one of the results is above the threshold
 * max_idle    - Stop capture when read more than max_idle idle records.
 *
 * Capture record starts when a probe is above the threshold. Hence threshold=0 means that capture
 * starts immediately.
 * Capture stops when either max_idle records have no value above the threshold or when max_records
 * are captured. max_idle is ignored if =0.
 *
 * Output is the reload times for each of the monitored addresses in each time slot.
 * Missed slots are indicted using time 0.
 * The return value is the number of records captured.
 */
int fr_trace(fr_t fr, int max_records, uint16_t *results, int slot, int threshold, int max_idle);

int fr_repeatedprobe(fr_t fr, int max_records, uint16_t *results, int slot);



#endif // __FR_H__

