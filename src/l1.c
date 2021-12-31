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
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <strings.h>

#include <mastik/low.h>
#include <mastik/l1.h>
#include <mastik/mm.h>
#include <mastik/impl.h>

#include "mm-impl.h"
#include "vlist.h"

struct l1pp {
  void **monitoredhead;
  int nsets;
  
  int* monitored;
  
  uint32_t *monitoredbitmap;
  
  cachelevel_e level;
  size_t totalsets;
  
  int ngroups;
  int groupsize;
  vlist_t *groups;
  
  struct l1info l1info;
  
  mm_t mm; 
  uint8_t internalmm;
};

int loadL1cpuidInfo(l1info_t l1info) {
  for (int i = 0; ; i++) {
    l1info->cpuidInfo.regs.eax = CPUID_CACHEINFO;
    l1info->cpuidInfo.regs.ecx = i;
    cpuid(&l1info->cpuidInfo);
    if (l1info->cpuidInfo.cacheInfo.type == 0)
      return 0;
    if (l1info->cpuidInfo.cacheInfo.level == 1)
      return 1;
  }
}

void fillL1Info(l1info_t l1info) {
  loadL1cpuidInfo(l1info);
  
  if (l1info->associativity == 0)
    l1info->associativity = l1info->cpuidInfo.cacheInfo.associativity + 1;
  if (l1info->sets == 0)
    l1info->sets = l1info->cpuidInfo.cacheInfo.sets + 1;
  if (l1info->bufsize == 0)
    l1info->bufsize = L1_STRIDE * l1info->associativity;
}

l1pp_t l1_prepare(mm_t mm) {
  l1pp_t l1 = (l1pp_t)malloc(sizeof(struct l1pp));
  bzero(l1, sizeof(struct l1pp));
  
  fillL1Info(&l1->l1info);
  
  l1->mm = mm;
  if (l1->mm == NULL) {
    l1->mm = mm_prepare(NULL, NULL, NULL);
    l1->internalmm = 1;
  }

  l1->monitored = (int*)malloc(L1_SETS * sizeof(int));
  l1->monitoredhead = (void **)malloc(L1_SETS * sizeof(void *));
  l1->monitoredbitmap = (uint32_t *)calloc((L1_SETS / 32) + 1, sizeof(uint32_t));
  l1->level = L1;
  l1->totalsets = L1_SETS;
  l1_monitorall(l1);
  return l1;
}

void l1_release(l1pp_t l1) {
  lx_release((lxpp_t)l1);
}


int l1_monitor(l1pp_t l1, int line) {
  return lx_monitor((lxpp_t) l1, line);
}

int l1_unmonitor(l1pp_t l1, int line) {
  lx_unmonitor((lxpp_t) l1, line);
}

void l1_monitorall(l1pp_t l1) {
  lx_monitorall((lxpp_t)l1);
}

void l1_unmonitorall(l1pp_t l1) {
  lx_unmonitorall((lxpp_t) l1);
}

int l1_getmonitoredset(l1pp_t l1, int *lines, int nlines) {
  return lx_getmonitoredset((lxpp_t) l1, lines, nlines); 
}

int l1_nsets(l1pp_t l1) {
  return l1->nsets;
}

void l1_randomise(l1pp_t l1) {
  lx_randomise((lxpp_t) l1);
}

void l1_probe(l1pp_t l1, uint16_t *results) {
  lx_probe((lxpp_t) l1, results);
}

void l1_bprobe(l1pp_t l1, uint16_t *results) {
  lx_bprobe((lxpp_t) l1, results);
}

int l1_repeatedprobe(l1pp_t l1, int nrecords, uint16_t *results, int slot) {
  return lx_repeatedprobe((lxpp_t) l1, nrecords, results, slot);
}

static void l1_dummy_cb(l1pp_t l1, int recnum, void *data) {
  return;
}

// Synchronized Prime+Probe
int l1_syncpp(l1pp_t l1, int nrecords, uint16_t *results, lx_sync_cb setup, lx_sync_cb exec, void *data) {
  lx_syncpp((lxpp_t)l1, nrecords, results, setup, exec, data);
}

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
