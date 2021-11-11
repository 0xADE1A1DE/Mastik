/*
 * Copyright 2019 CSIRO
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
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <mastik/low.h>
#include <mastik/synctrace.h>
#include <mastik/l1.h>

#include <mastik/lx.h>
#include <mastik/l2.h>

#define ST_TRACEWIDTH L2_SETS

#define L2_LOW_THRESHOLD 90
#define LIMIT 400
#define SCALE 1000

typedef struct synctrace *synctrace_t;

struct synctrace
{
  // setup data
  int blockSize;
  uint8_t fixMask[ST_BLOCKBYTES];
  uint8_t fixData[ST_BLOCKBYTES];

  // exec data
  uint8_t input[ST_BLOCKBYTES];
  uint8_t output[ST_BLOCKBYTES];
  st_crypto_f crypto;
  void *cryptoData;

  // Prcoess data
  int limit;
  uint8_t *split;
  int map[L2_SETS];
  uint8_t clusterMask;
  st_clusters_t clusters;
};

void dummy_setup_cb(int nrecords, void *data)
{
  return;
}

// Synchronized Prime+Probe
int st_lxpp(lxpp_t lx, int nrecords, st_setup_cb setup, st_exec_cb exec, st_process_cb process, void *data)
{
  assert(lx != NULL);
  assert(exec != NULL);
  assert(nrecords >= 0);

  if (nrecords == 0)
    return 0;

  if (setup == NULL)
    setup = dummy_setup_cb;
  int len = lx_getmonitoredset(lx, NULL, 0);

  uint16_t *res = malloc(len * sizeof(uint16_t));

  for (int i = 0; i < nrecords; i++)
  {
    setup(i, data);
    lx_probe(lx, res);
    exec(i, data);
    lx_bprobe(lx, res);
    process(i, data, len, res);
  }

  free(res);
  return nrecords;
}

/*
// Synchronized Evict+Time
int st_l1et(l1pp_t l1, int nrecords, st_setup_cb setup, st_exec_cb exec, st_process_cb process, void *data);
// Synchronized Evict+Time
int l1_syncet(l1pp_t l1, int nrecords, uint16_t *results, l1_sync_cb setup, l1_sync_cb exec, void *data) {
  assert(l1 != NULL);
  assert(results != NULL);
  assert(exec != NULL);
  assert(nrecords >= 0);

  uint16_t dummyres[64];

  if (nrecords == 0)
    return 0;

  if (setup == NULL)
    setup = l1_dummy_cb;
  int len = l1->nsets;

  for (int i = 0; i < nrecords; i++, results++) {
    setup(l1, i, data);
    exec(l1, i, data);
    l1_probe(l1, dummyres);
    l1_probe(l1, dummyres);
    l1_probe(l1, dummyres);
    uint32_t start = rdtscp();
    exec(l1, i, data);
    uint32_t res = rdtscp() - start;
    *results = res > UINT16_MAX ? UINT16_MAX : res;
  }
  return nrecords;
}
*/

static void spp_setup(int recnum, void *vst)
{
  synctrace_t st = (synctrace_t)vst;
  for (int i = 0; i < st->blockSize; i++)
    st->input[i] = (rand() & 0xff & ~st->fixMask[i]) | (st->fixData[i] & st->fixMask[i]);
}

static void spp_process(int recnum, void *vst, int nres, uint16_t results[])
{
  synctrace_t st = (synctrace_t)vst;
  for (int byte = 0; byte < st->blockSize; byte++)
  {
    int inputbyte = st->split[byte] & st->clusterMask;
    st->clusters[byte].count[inputbyte]++;
    for (int i = 0; i < nres; i++)
    {
      uint64_t res = results[i] > LIMIT ? LIMIT : results[i];

      st->clusters[byte].avg[inputbyte][st->map[i]] += res;
      st->clusters[byte].var[inputbyte][st->map[i]] += res * res;
    }
  }
}

static void spp_exec(int recnum, void *vst)
{
  synctrace_t st = (synctrace_t)vst;
  (*st->crypto)(st->input, st->output, st->cryptoData);
}

static void normalise(int blockSize, st_clusters_t clusters,
                      int level)
{
  int nsets;
  if (level == 1)
    nsets = L1_SETS;
  else if (level == 2)
    nsets = L2_SETS;
  for (int byte = 0; byte < blockSize; byte++)
  {
    int total_count = 0;
    int64_t total_avg[nsets];
    memset(total_avg, 0, sizeof(total_avg));
    for (int cluster = 0; cluster < 256; cluster++)
    {
      if (clusters[byte].count[cluster])
      {
        int count = clusters[byte].count[cluster];
        total_count += count;
        for (int set = 0; set < nsets; set++)
        {
          int64_t avg = clusters[byte].avg[cluster][set];
          total_avg[set] += avg;
          avg = (avg * SCALE + count / 2) / count;
          clusters[byte].avg[cluster][set] = avg;
        }
      }
    }
    for (int set = 0; set < nsets; set++)
      total_avg[set] = (total_avg[set] * SCALE + total_count / 2) / total_count;

    for (int cluster = 0; cluster < 256; cluster++)
      if (clusters[byte].count[cluster])
        for (int set = 0; set < nsets; set++)
          clusters[byte].avg[cluster][set] -= total_avg[set];
  }
}

st_clusters_t syncPrimeProbe(int nsamples,
                             int blockSize,
                             int splitinput,
                             uint8_t *fixMask,
                             uint8_t *fixData,
                             st_crypto_f crypto,
                             void *cryptoData,
                             uint8_t clusterMask,
                             int cachelevel)
{
  synctrace_t st = (synctrace_t)malloc(sizeof(struct synctrace));
  memset(st, 0, sizeof(struct synctrace));
  st->blockSize = blockSize;
  if (fixMask && fixData)
  {
    memcpy(st->fixMask, fixMask, blockSize);
    memcpy(st->fixData, fixData, blockSize);
  }
  st->crypto = crypto;
  st->cryptoData = cryptoData;
  st_clusters_t clusters = calloc(blockSize, sizeof(struct st_clusters));
  st->clusters = clusters;
  st->clusterMask = clusterMask;
  st->split = splitinput ? st->input : st->output;
  lxpp_t lx;
  if (cachelevel == 1)
    lx = (lxpp_t)l1_prepare(NULL);
  else if (cachelevel == 2)
    lx = (lxpp_t)l2_prepare(NULL, NULL);

  lx_monitorall(lx);

  lx_getmonitoredset(lx, st->map, lx->nmonitored);
  st_lxpp(lx, nsamples, spp_setup, spp_exec, spp_process, st);

  free(st);
  normalise(blockSize, clusters, cachelevel);
  lx_release(lx);
  return clusters;
}

/*
st_clusters_t syncEvictTime(int nsamples, 
			   int blockSize, 
			   uint8_t *fixMask, 
			   uint8_t *fixData, 
			   st_crypto_f crypto,
			   void *cryptoData,
			   uint8_t clusterMask) {
  synctrace_t st = (synctrace_t)malloc(sizeof(struct synctrace));
  memset(st, 0, sizeof(struct synctrace));
  st->blockSize = blockSize;
  if (fixMask && fixData) {
    memcpy(st->fixMask, fixMask, blockSize);
    memcpy(st->fixData, fixData, blockSize);
  }
  st->crypto = crypto;
  st->cryptoData = cryptoData;
  st_clusters_t clusters = calloc(blockSize, sizeof(struct st_clusters));
  st->clusters = clusters;
  st->clusterMask = clusterMask;
  l1pp_t l1 = l1_prepare(NULL);
  l1_getmonitoredset(l1, st->map, L1_SETS);
  st_l1pp(l1, nsamples, spp_setup, spp_exec, spp_process, st);
  free(st);
  normalise(blockSize, clusters);
  return clusters;
}
*/
