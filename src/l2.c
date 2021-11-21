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


#include "config.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <sys/mman.h>
#ifdef __APPLE__
#include <mach/vm_statistics.h>
#endif

#include <mastik/l2.h>
#include <mastik/low.h>
#include <mastik/mm.h>
#include <mastik/lx.h>
#include <mastik/impl.h>
#include <mastik/mm.h>

#include "vlist.h"
#include "mm-impl.h"
#include "timestats.h"
#include "tsx.h"



struct l2pp {
  void **monitoredhead;
  int nmonitored;
  
  int *monitoredset;
  
  uint32_t *monitoredbitmap;
  
  cachelevel_e level;
  size_t totalsets;
    
  int ngroups;
  int groupsize;
  vlist_t *groups;
  
  struct l2info l2info;
  
  mm_t mm;
  uint8_t internalmm;
};

int loadL2cpuidInfo(l2info_t l2info) {
  for (int i = 0; ; i++) {
    l2info->cpuidInfo.regs.eax = CPUID_CACHEINFO;
    l2info->cpuidInfo.regs.ecx = i;
    cpuid(&l2info->cpuidInfo);
    if (l2info->cpuidInfo.cacheInfo.type == 0)
      return 0;
    if (l2info->cpuidInfo.cacheInfo.level == 2)
      return 1;
  }
}

void fillL2Info(l2info_t l2info) {
  loadL2cpuidInfo(l2info);
  if (l2info->associativity == 0)
    l2info->associativity = l2info->cpuidInfo.cacheInfo.associativity + 1;
  if (l2info->sets == 0)
    l2info->sets = l2info->cpuidInfo.cacheInfo.sets + 1;
  if (l2info->bufsize == 0) {
    l2info->bufsize = l2info->associativity * l2info->sets * L2_CACHELINE * 2;
  }
}

l2pp_t l2_prepare(l2info_t l2info, mm_t mm) {
  // Setup
  l2pp_t l2 = (l2pp_t)malloc(sizeof(struct l2pp));
  bzero(l2, sizeof(struct l2pp));
  if (l2info != NULL)
    bcopy(l2info, &l2->l2info, sizeof(struct l2info));
  fillL2Info(&l2->l2info);
  l2->level = L2;
  
  l2->mm = mm;
  if (l2->mm == NULL) {
    l2->mm = mm_prepare(NULL, NULL, NULL);
    l2->internalmm = 1;
  }
  
  const int nsets = l2->l2info.sets;
  const int nways = l2->l2info.associativity;
  
  l2->groupsize = L2_SETS_PER_PAGE;
  
  l2->ngroups = l2->l2info.sets / l2->groupsize;

  l2->monitoredbitmap = (uint32_t *)calloc((nsets/32) + 1, sizeof(uint32_t));
  l2->monitoredset = malloc(nsets * sizeof(int)); 
  l2->monitoredhead = (void **)malloc(nsets * sizeof(void *));
  l2->nmonitored = 0;
  l2->totalsets = l2->l2info.sets;
  
  l2_monitorall(l2);
  
  return l2;
}
            
int l2_monitor(l2pp_t l2, int line) {
  lx_monitor((lxpp_t) l2, line);
}

void l2_randomise(l2pp_t l2) {
  lx_randomise((lxpp_t) l2);
}

void l2_monitorall(l2pp_t l2) {
  lx_monitorall((lxpp_t)l2);
}

int l2_unmonitor(l2pp_t l2, int line) {
  return lx_unmonitor((lxpp_t) l2, line);
}

void l2_unmonitorall(l2pp_t l2) {
  lx_unmonitorall((lxpp_t) l2);
}

void l2_probe(l2pp_t l2, uint16_t *results) {
  lx_probe((lxpp_t) l2, results);
}

void l2_bprobe(l2pp_t l2, uint16_t *results) {
  lx_bprobe((lxpp_t) l2, results);
}

int l2_getmonitoredset(l2pp_t l2, int *lines, int nlines) {
  return lx_getmonitoredset((lxpp_t) l2, lines, nlines);
}

void l2_release(l2pp_t l2) {
  lx_release((lxpp_t)l2);
}

int l2_repeatedprobe(l2pp_t l2, int nrecords, uint16_t *results, int slot) {
  return lx_repeatedprobe((lxpp_t) l2, nrecords, results, slot);
}

int l2_getl2info(l2pp_t l2, l2info_t l2info) {
  return lx_getlxinfo((lxpp_t)l2, (lxinfo_t)l2info);
}

int l2_syncpp(l2pp_t l2, int nrecords, uint16_t *results, lx_sync_cb setup, lx_sync_cb exec, void *data) {
  return lx_syncpp((lxpp_t)l2, nrecords, results, setup, exec, data);
}
