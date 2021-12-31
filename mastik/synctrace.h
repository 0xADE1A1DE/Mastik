/*
 * Copyright 2021 The University of Adelaide
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

#ifndef __SYNCTRACE_H__
#define __SYNCTRACE_H__ 1

#include <mastik/l1.h>
#include <mastik/l2.h>

#include <stdint.h>

// Type of callback for setup 
typedef void (*st_setup_cb)(int recnum, void *data);
// Type of callback for exec 
typedef void (*st_exec_cb)(int recnum, void *data);
// Type of callback for results processing
typedef void (*st_process_cb)(int recnum, void *data, int nres, uint16_t results[]);

// Synchronized Prime+Probe
int st_lpp(lxpp_t lx, int nrecords, st_setup_cb setup, st_exec_cb exec, st_process_cb process, void *data);

// Synchronized Evict+Time
int st_l1et(l1pp_t l1, int nrecords, st_setup_cb setup, st_exec_cb exec, st_process_cb process, void *data);


// Crypto specific functions
//
#define ST_BLOCKBITS	512
#define ST_BLOCKBYTES	(ST_BLOCKBITS>>3)
#define ST_TRACEWIDTH L2_SETS

typedef void (*st_crypto_f)(uint8_t input[], uint8_t output[], void *cryptoData);

struct st_clusters {
  int count[256];
  int64_t avg[256][ST_TRACEWIDTH];
  int64_t var[256][ST_TRACEWIDTH];
};

typedef struct st_clusters *st_clusters_t;

st_clusters_t syncPrimeProbe(int nsamples, 
			      int blocksize, 
			      int splitinput,
			      uint8_t *fixMask, 
			      uint8_t *fixData, 
			      st_crypto_f crypto,
			      void *cryptoData,
			      uint8_t clusterMask,
						int cachelevel);

/*
st_clusters_t syncEvictTime(int nsamples, 
			      int blocksize, 
			      uint8_t *fixMask, 
			      uint8_t *fixData, 
			      st_crypto_f crypto,
			      void *cryptoData,
			      uint8_t clusterMask);
*/


#endif  // __SYNCTRACE_H__


