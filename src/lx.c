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

#include <mastik/l1.h>
#include <mastik/l2.h>
#include <mastik/l3.h>
#include <mastik/low.h>
#include <mastik/impl.h>
#include <mastik/mm.h>

#include "vlist.h"
#include "mm-impl.h"
#include "timestats.h"
#include "tsx.h"

int probetime(void *pp) {
  if (pp == NULL)
    return 0;
  int rv = 0;
  void *p = (void *)pp;
  uint32_t s = rdtscp();
  do {
    p = LNEXT(p);
  } while (p != (void *) pp);
  return rdtscp()-s;
}

int bprobetime(void *pp) {
  if (pp == NULL)
    return 0;
  return probetime(NEXTPTR(pp));
}

int probecount(void *pp) {
  if (pp == NULL)
    return 0;
  int rv = 0;
  void *p = (void *)pp;
  do {
    uint32_t s = rdtscp();
    p = LNEXT(p);
    s = rdtscp() - s;
    if (s > L3_THRESHOLD)
      rv++;
  } while (p != (void *) pp);
  return rv;
}

int bprobecount(void *pp) {
  if (pp == NULL)
    return 0;
  return probecount(NEXTPTR(pp));
}

void lx_probe(lxpp_t lx, uint16_t *results) {
  for (int i = 0; i < lx->nmonitored; i++) {
    int t = probetime(lx->monitoredhead[i]);
    results[i] = t > UINT16_MAX ? UINT16_MAX : t;
  }
}

void lx_bprobe(lxpp_t lx, uint16_t *results) {
  for (int i = 0; i < lx->nmonitored; i++) {
    int t = bprobetime(lx->monitoredhead[i]);
    results[i] = t > UINT16_MAX ? UINT16_MAX : t;
  }
}

void lx_probecount(lxpp_t lx, uint16_t *results) {
  for (int i = 0; i < lx->nmonitored; i++)
    results[i] = probecount(lx->monitoredhead[i]);
}

void lx_bprobecount(lxpp_t lx, uint16_t *results) {
  for (int i = 0; i < lx->nmonitored; i++)
    results[i] = bprobecount(lx->monitoredhead[i]);
}

int lx_repeatedprobe(lxpp_t lx, int nrecords, uint16_t *results, int slot) {
  assert(lx != NULL);
  assert(results != NULL);

  if (nrecords == 0)
    return 0;

  int len = lx->nmonitored;

  int even = 1;
  int missed = 0;
  uint64_t prev_time = rdtscp64();
  for (int i = 0; i < nrecords; i++, results+=len) {
    if (missed) {
      for (int j = 0; j < len; j++)
	results[j] = 0;
    } else {
      if (even)
	lx_probe(lx, results);
      else
	lx_bprobe(lx, results);
      even = !even;
    }
    if (slot > 0) {
      prev_time += slot;
      missed = slotwait(prev_time);
    }
  }
  return nrecords;
}

int lx_repeatedprobecount(lxpp_t lx, int nrecords, uint16_t *results, int slot) {
  assert(lx != NULL);
  assert(results != NULL);

  if (nrecords == 0)
    return 0;

  int len = lx->nmonitored;

  int even = 1;
  int missed = 0;
  uint64_t prev_time = rdtscp64();
  for (int i = 0; i < nrecords; i++, results+=len) {
    if (missed) {
      for (int j = 0; j < len; j++)
	results[j] = -1;
    } else {
      if (even)
	lx_probecount(lx, results);
      else
	lx_bprobecount(lx, results);
      even = !even;
    }
    if (slot > 0) {
      prev_time += slot;
      missed = slotwait(prev_time);
    }
  }
  return nrecords;
}

void lx_randomise(lxpp_t lx) {
  for (int i = 0; i < lx->nmonitored; i++) {
    int p = random() % (lx->nmonitored - i) + i;
    int t = lx->monitoredset[p];
    lx->monitoredset[p] = lx->monitoredset[i];
    lx->monitoredset[i] = t;
    
    void *vt = lx->monitoredhead[p];
    lx->monitoredhead[p] = lx->monitoredhead[i];
    lx->monitoredhead[i] = vt;
  }
}

int lx_getmonitoredset(lxpp_t lx, int *lines, int nlines) {
  if (lines == NULL || nlines == 0)
    return lx->nmonitored;
  if (nlines > lx->nmonitored)
    nlines = lx->nmonitored;
  if (lines)
    bcopy(lx->monitoredset, lines, nlines * sizeof(int));
  return lx->nmonitored;
}

static int return_linked_memory(lxpp_t lx, void* head) {
  void* p = head;
  do {
    mm_returnline(lx->mm, head);
    head = *((void **) head);
  } while (head != p);
}

int lx_unmonitor(lxpp_t lx, int line) {
  if (line < 0 || line >= lx->totalsets) 
    return 0;
  if (!IS_MONITORED(lx->monitoredbitmap, line))
    return 0;
  UNSET_MONITORED(lx->monitoredbitmap, line);
  for (int i = 0; i < lx->nmonitored; i++)
    if (lx->monitoredset[i] == line) {
      --lx->nmonitored;
      lx->monitoredset[i] = lx->monitoredset[lx->nmonitored];
      
      return_linked_memory(lx, lx->monitoredhead[i]);
      
      lx->monitoredhead[i] = lx->monitoredhead[lx->nmonitored];
      
      break;
    }
  return 1;
}

void lx_monitorall(lxpp_t lx) {
  for (int i = 0; i < lx->totalsets; i++) 
    lx_monitor(lx, i);
}

void lx_unmonitorall(lxpp_t lx) {
  for (int i = 0; i < lx->totalsets / 32; i++)
    lx->monitoredbitmap[i] = 0;
  for (int i = 0; i < lx->nmonitored; i++) 
    return_linked_memory(lx, lx->monitoredhead[i]);
  lx->nmonitored = 0;
}
                                       
int lx_getlxinfo(lxpp_t lx, lxinfo_t lxinfo) {
  if (!lxinfo || !lx)
    return -1;
  bcopy(&lx->lxinfo, lxinfo, sizeof(struct lxinfo));
}

void lx_dummy_cb(lxpp_t l1, int recnum, void *data) {
  return;
}

// Synchronized Prime+Probe
int lx_syncpp(lxpp_t lx, int nrecords, uint16_t *results, lx_sync_cb setup, lx_sync_cb exec, void *data) {
  assert(lx != NULL);
  assert(results != NULL);
  assert(exec != NULL);
  assert(nrecords >= 0);

  if (nrecords == 0)
    return 0;

  if (setup == NULL)
    setup = lx_dummy_cb;
  int len = lx->nmonitored;

  for (int i = 0; i < nrecords; i++, results += len) {
    setup(lx, i, data);

    lx_probe(lx, results);
  
    exec(lx, i, data);

    lx_bprobe(lx, results);
  }
  return nrecords;
}

int lx_monitor(lxpp_t lx, int line) {
  if (line < 0 || line >= lx->totalsets)
    return 0;
  if (IS_MONITORED(lx->monitoredbitmap, line))
    return 0;
  
  int associativity = lx->lxinfo.associativity;
    
  vlist_t vl = vl_new();
  _mm_requestlines(lx->mm, lx->level, line, associativity, vl);
  
  int len = vl_len(vl);
  for (int way = 0; way < len; way++) {
    void* mem = vl_get(vl, way);
    void* nmem = vl_get(vl, (way + 1) % len);
    void* pmem = vl_get(vl, (way - 1 + len) % len);
  
    LNEXT(mem) = nmem;
    LNEXT(mem + sizeof(void*)) = (pmem + sizeof(void *));
  }
  lx->monitoredset[lx->nmonitored] = line;
  lx->monitoredhead[lx->nmonitored++] = vl_get(vl, 0);
  SET_MONITORED(lx->monitoredbitmap, line);
  vl_free(vl);
  return 1;
}

void lx_release(lxpp_t lx) {
  free(lx->monitoredbitmap);
  free(lx->monitoredset);
  free(lx->monitoredhead);
  if (lx->internalmm)
    mm_release(lx->mm);
  bzero(lx, sizeof(struct lxpp));
  free(lx);
}
