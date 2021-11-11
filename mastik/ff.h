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

#ifndef __FF_H__
#define __FF_H__ 1


/*
 * Performs the Flush+Flush [1] attack.
 * The attack exploits the timing variations in the clflush, which takes
 * a longer time to ensure the flushed memory line is evicted from
 * all cache levels if the line is cached.  
 *
 * The attack can only target memory addresses that are shared between
 * the spy and the victim.  Typically, such sharing results from mmaping
 * a binary the victim use.  For cross-VM attacks, the hypervisor
 * must support page deduplication.  The most common attack target is
 * shared code.
 *
 * The spatial resolution of the attack is one cache line (64 bytes).  The
 * attack is likely to be unaffected by cache optimisations such as
 * the Spatial Prefetcher that pair consecutive cache lines and any streamers
 * that try to identify access patterns to the cache. 
 *
 * False positives are relatively common.
 *
 * The attack is sensitive to migration between processor cores. clflush timing
 * depends on both the core the instruction executes on and the cache slice
 * the data is in. No single threshold is useful for distinguishin results on
 * all combinations of cores and slices.  It is recommended to control the spy
 * placement using taskset(1) or the supplied setaffinity. (See util.h.)
 *
 * [1] D. Gruss, C. Maurice, K. Wagner, and S. Mangard, "Flush+Flush: 
 *     A Fast and Stealthy Cahce Attack", DIMVA 2016 
 */


/* 
 * An opaque reference to an attack instance 
 */
typedef struct ff *ff_t;



/* 
 * Creates and initialises an attack instance
 */
ff_t ff_prepare();


/* 
 * Disposes of all resources acquired for an attack instance 
 */
void ff_release(ff_t ff);


/* 
 * Adds a memory address to the list of addresses monitored by
 * an attack instance
 */
int ff_monitor(ff_t ff, void *adrs);


/*
 * Removes a memory address from the list of addresses monitored
 * by an attack instance.
 */
int ff_unmonitor(ff_t ff, void *adrs);


/*
 * Retrieve the list of addresses monitored by an attack instance. Parameters:
 *   ff     - Attack instance
 *   adrss  - A buffer for storing the list. Set to NULL to retrieve the number
 *            of monitored addresses.
 *   nlines - Buffer size
 * Returns the number of addresses monitored.
 */
int ff_getmonitoredset(ff_t ff, void **adrss, int nlines);


/* 
 * Calculate the thresholds between flushing a cached and a non-cached line.
 * ff_trace (see below) usually keeps track of the thresholds, but
 * explicit calculation is required when the spy migrates between cores or 
 * to avoid delays when calling ff_trace.
 */ 
void ff_setthresholds(ff_t ff);


/*
 * Returns the threshold for a monitored address.
 * 0 - threshold not calculates
 * -1 - index out of bounds.
 */
int ff_getthreshold(ff_t ff, int index);

/*
 * Not currently implemented
 */
void ff_randomise(ff_t ff);





/*
 * Performs a single probe round on each of the monitored addresses.
 * results points to a buffer for storing the timing to clflush each
 * each of the monitored addresses.
 * Addresses are stored in the order returned by ff_getmonitoredset.
 * This is guaranteed to be the order addresses are added using ff_monitor
 * as long as ff_unmonitor is not used.
 */
void ff_probe(ff_t ff, uint16_t *results);


/*
 * Performs repeated calls to ff_probe. Prameters are:
 * ff          - The ff descriptor
 * max_records - The number of records to collect
 * results     - An array for results of size ff_len(ff) * max_records
 * slot        - The length of a time-slot in cycles.  Use 0 to have flexible slot size
 * threshold   - True if should calculate activity thresholds and use them.
 * max_idle    - Stop capture when read more than max_idle idle records.
 *
 * If threshold is set, capture record starts when a probe is active, i.e. 
 * above its threshold. Otherwise capture starts immediately.
 * Capture stops when either max_idle records with no active address or when max_records
 * are captured. max_idle is ignored if =0.
 *
 * Output is the reload times for each of the monitored addresses in each time slot.
 * Missed slots are indicted using time 0.
 * The return value is the number of records captured.
 */
int ff_trace(ff_t ff, int max_records, uint16_t *results, int slot, int threshold, int max_idle);


/*
 * Repeat ff_probe max_record times. Equivalent to:
 *     ff_trace(ff, max_records, results, slot, 0, max_records);
 */
int ff_repeatedprobe(ff_t ff, int max_records, uint16_t *results, int slot);


int ff_fastrepeatedprobe(ff_t ff, int max_records, uint16_t *results);

#endif // __FF_H__

