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

#include <mastik/mm.h>
#include <mastik/l3.h>
#include <mastik/low.h>
#include <mastik/impl.h>
#include <mastik/lx.h>

#include "vlist.h"
#include "mm-impl.h"
#include "timestats.h"
#include "tsx.h"

#define CHECKTIMES 16

/*
 * Intel documentation still mentiones one slice per core, but
 * experience shows that at least some Skylake models have two
 * smaller slices per core. 
 * When probing the cache, we can use the smaller size - this will
 * increase the probe time but for huge pages, where we use
 * the slice size, the probe is fast and the increase is not too
 * significant.
 * When using the PTE maps, we need to know the correct size and
 * the correct number of slices.  This means that, currently and
 * without user input, PTE is not guaranteed to work.
 * So, on a practical note, L3_GROUPSIZE_FOR_HUGEPAGES is the
 * smallest slice size we have seen; L3_SETS_PER_SLICE is the
 * default for the more common size.  If we learn how to probe
 * the slice size we can get rid of this mess.
 */

struct l3pp {
  void **monitoredhead;
  int nmonitored;
  
  int *monitoredset;
  
  uint32_t *monitoredbitmap;
    
  cachelevel_e level;
  size_t totalsets;
  
  int ngroups;
  int groupsize;
  vlist_t *groups;
  
  struct l3info l3info;
  
  mm_t mm; 
  uint8_t internalmm;
  
  // To reduce probe time we group sets in cases that we know that a group of consecutive cache lines will
  // always map to equivalent sets. In the absence of user input (yet to be implemented) the decision is:
  // Non linear mappings - 1 set per group (to be implemeneted)
  // Huge pages - L3_SETS_PER_SLICE sets per group (to be impolemented)
  // Otherwise - L3_SETS_PER_PAGE sets per group.
};

int loadL3cpuidInfo(l3info_t l3info) {
  for (int i = 0; ; i++) {
    l3info->cpuidInfo.regs.eax = CPUID_CACHEINFO;
    l3info->cpuidInfo.regs.ecx = i;
    cpuid(&l3info->cpuidInfo);
    if (l3info->cpuidInfo.cacheInfo.type == 0)
      return 0;
    if (l3info->cpuidInfo.cacheInfo.level == 3)
      return 1;
  }
}

void fillL3Info(l3info_t l3info) {
  loadL3cpuidInfo(l3info);
  if (l3info->associativity == 0)
    l3info->associativity = l3info->cpuidInfo.cacheInfo.associativity + 1;
  if (l3info->slices == 0) {
    if (l3info->setsperslice == 0)
      l3info->setsperslice = L3_SETS_PER_SLICE;
    l3info->slices = (l3info->cpuidInfo.cacheInfo.sets + 1)/ l3info->setsperslice;
  }
  if (l3info->setsperslice == 0)
    l3info->setsperslice = (l3info->cpuidInfo.cacheInfo.sets + 1)/l3info->slices;
  if (l3info->bufsize == 0) {
    l3info->bufsize = l3info->associativity * l3info->slices * l3info->setsperslice * L3_CACHELINE * 2;
    if (l3info->bufsize < 10 * 1024 * 1024)
      l3info->bufsize = 10 * 1024 * 1024;
  }
}

#if 0
void printL3Info() {
  struct l3info l3Info;
  loadL3cpuidInfo(&l3Info);
  printf("type            : %u\n", l3Info.cpuidInfo.cacheInfo.type);
  printf("level           : %u\n", l3Info.cpuidInfo.cacheInfo.level);
  printf("selfInitializing: %u\n", l3Info.cpuidInfo.cacheInfo.selfInitializing);
  printf("fullyAssociative: %u\n", l3Info.cpuidInfo.cacheInfo.fullyAssociative);
  printf("logIds          : %u\n", l3Info.cpuidInfo.cacheInfo.logIds + 1);
  printf("phyIds          : %u\n", l3Info.cpuidInfo.cacheInfo.phyIds + 1);
  printf("lineSize        : %u\n", l3Info.cpuidInfo.cacheInfo.lineSize + 1);
  printf("partitions      : %u\n", l3Info.cpuidInfo.cacheInfo.partitions + 1);
  printf("associativity   : %u\n", l3Info.cpuidInfo.cacheInfo.associativity + 1);
  printf("sets            : %u\n", l3Info.cpuidInfo.cacheInfo.sets + 1);
  printf("wbinvd          : %u\n", l3Info.cpuidInfo.cacheInfo.wbinvd);
  printf("inclusive       : %u\n", l3Info.cpuidInfo.cacheInfo.inclusive);
  printf("complexIndex    : %u\n", l3Info.cpuidInfo.cacheInfo.complexIndex);
  exit(0);
}
#endif

void prime(void *pp, int reps) {
  walk((void *)pp, reps);
}

#define str(x) #x
#define xstr(x) str(x)

l3pp_t l3_prepare(l3info_t l3info, mm_t mm) {
  // Setup
  l3pp_t l3 = (l3pp_t)malloc(sizeof(struct l3pp));
  bzero(l3, sizeof(struct l3pp));
  if (l3info != NULL)
    bcopy(l3info, &l3->l3info, sizeof(struct l3info));
  fillL3Info(&l3->l3info);
  l3->level = L3;

  // Check if linearmap and quadratic map are called together
  if ((l3->l3info.flags & (L3FLAG_LINEARMAP | L3FLAG_QUADRATICMAP)) == (L3FLAG_LINEARMAP | L3FLAG_QUADRATICMAP)) {
    free(l3);
    fprintf(stderr, "Error: Cannot call linear and quadratic map together\n");
    return NULL;
  }
  
  l3->mm = mm;
  if (l3->mm == NULL) {
    l3->mm = mm_prepare(NULL, NULL, (lxinfo_t)l3info);
    l3->internalmm = 1;
  }
  
  if (!mm_initialisel3(l3->mm)) 
    return NULL;
  
  l3->ngroups = l3->mm->l3ngroups;
  l3->groupsize = l3->mm->l3groupsize;
  
  // Allocate monitored set info
  l3->monitoredbitmap = (uint32_t *)calloc((l3->ngroups*l3->groupsize/32) + 1, sizeof(uint32_t));
  l3->monitoredset = (int *)malloc(l3->ngroups * l3->groupsize * sizeof(int));
  l3->monitoredhead = (void **)malloc(l3->ngroups * l3->groupsize * sizeof(void *));
  l3->nmonitored = 0;
  l3->totalsets = l3->ngroups * l3->groupsize;

  return l3;
}

void l3_release(l3pp_t l3) {
  lx_release((lxpp_t)l3);
}

int l3_monitor(l3pp_t l3, int line) {
  return lx_monitor((lxpp_t) l3, line);
}

int l3_unmonitor(l3pp_t l3, int line) {
  return lx_unmonitor((lxpp_t) l3, line);
}

void l3_unmonitorall(l3pp_t l3) {
  lx_unmonitorall((lxpp_t) l3);
}

int l3_getmonitoredset(l3pp_t l3, int *lines, int nlines) {
  return lx_getmonitoredset((lxpp_t) l3, lines, nlines);
}

void l3_randomise(l3pp_t l3) {
  lx_randomise((lxpp_t) l3);
}

void l3_probe(l3pp_t l3, uint16_t *results) {
  lx_probe((lxpp_t) l3, results);
}

void l3_bprobe(l3pp_t l3, uint16_t *results) {
  lx_bprobe((lxpp_t) l3, results);
}

void l3_probecount(l3pp_t l3, uint16_t *results) {
  lx_probecount((lxpp_t) l3, results);
}

void l3_bprobecount(l3pp_t l3, uint16_t *results) {
  lx_bprobecount((lxpp_t) l3, results);
}

// Returns the number of probed sets in the LLC
int l3_getSets(l3pp_t l3) {
  return l3->ngroups * l3->groupsize;
}

// Returns the number slices
int l3_getSlices(l3pp_t l3) {
  return l3->l3info.slices;
}

// Returns the LLC associativity
int l3_getAssociativity(l3pp_t l3) {
  return l3->l3info.associativity;
}

int l3_repeatedprobe(l3pp_t l3, int nrecords, uint16_t *results, int slot) {
  return lx_repeatedprobe((lxpp_t) l3, nrecords, results, slot);
}

int l3_repeatedprobecount(l3pp_t l3, int nrecords, uint16_t *results, int slot) {
  return lx_repeatedprobecount((lxpp_t) l3, nrecords, results, slot);
}

void l3_pa_prime(l3pp_t l3) {
  for (int i = 0; i < l3->nmonitored; i++) {
    int t = probetime(l3->monitoredhead[i]);
  }
}

/* A single round of TSX abort detection, decide existing time period of 
 * RTM region by modifing 'time_limit'.  It returns 0 if there is no TSX 
 * abort; It returns 1 if the abort is caused by internal buffer overflow or
 * memory address conflict; It returns -1 if abort is caused by other reasons,
 * e.g. context switch, some micro instructions 
 */
int l3_pabort(l3pp_t l3, uint32_t time_limit) {
  unsigned ret; 

  if ((ret = xbegin()) == XBEGIN_INIT){
    l3_pa_prime(l3); 
    uint32_t s = rdtscp();
    while((rdtscp() - s) < time_limit)
      ;
    xend();
  } else {
    if ((ret & XABORT_CAPACITY) || (ret & XABORT_CONFLICT) )
      return 1;
    else
      return -1; 
  }
  return 0;
}

/* Run TSX abort detection repeatedly. 'Sample': How many repeats will 
 * be performed; 'results': store results of l3_pabort()
 */
void l3_repeatedpabort(l3pp_t l3, int sample, int16_t *results, uint32_t time_limit) {
  int cont = 0;

  do{ 
    results[cont] = l3_pabort(l3, time_limit);
  } while(cont++ < sample);
}
