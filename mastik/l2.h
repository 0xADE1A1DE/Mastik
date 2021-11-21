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

#ifndef __L2_H__
#define __L2_H__ 1

#include <stdint.h>
#include <mastik/impl.h>

#include <mastik/mm.h>

typedef struct l2pp *l2pp_t;
typedef struct l2info *l2info_t;

#define L2_SETS 1024

#define L2_CACHELINE 64

// The number of cache sets in each page
#define L2_SETS_PER_PAGE 64

typedef void (*l2progressNotification_t)(int count, int est, void *data);

struct l2info {
  int associativity;
  int sets;
  int bufsize;
  int flags;
  void *progressNotificationData;
  
  // padding variables
  int slices;
  l2progressNotification_t progressNotification;
  union cpuid cpuidInfo;
};

l2pp_t l2_prepare(l2info_t l2info, mm_t mm);
int l2_monitor(l2pp_t l2, int line);
int l2_getmonitoredset(l2pp_t l2, int *lines, int nlines);
int l2_unmonitor(l2pp_t l2, int line);
void l2_monitorall(l2pp_t l2);
void l2_unmonitorall(l2pp_t l2);
void l2_probe(l2pp_t l2, uint16_t *results);
void l2_bprobe(l2pp_t l2, uint16_t *results);
int l2_getmonitoredset(l2pp_t l2, int *lines, int nlines);
void l2_release(l2pp_t l2);
int l2_repeatedprobe(l2pp_t l2, int nrecords, uint16_t *results, int slot);
int l2_getl2info(l2pp_t l2, l2info_t l2info);
int l2_syncpp(l2pp_t l2, int nrecords, uint16_t *results, lx_sync_cb setup, lx_sync_cb exec, void *data);
void l2_randomise(l2pp_t l2);

int loadL2cpuidInfo(l2info_t l2info);
void fillL2Info(l2info_t l2info);

#endif // __L2_H__
