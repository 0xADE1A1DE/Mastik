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
#include <mastik/lx.h>
#include <mastik/mm.h>

#include "vlist.h"
#include "mm-impl.h"
#include "timestats.h"
#include "tsx.h"

static int ptemap(mm_t mm);
static int probemap(mm_t mm);
static int checkevict(vlist_t es, void *candidate);
static uintptr_t getphysaddr(void *p);

static void *allocate_buffer(mm_t mm)
{
  // Allocate cache buffer
  int bufsize;
  char *buffer = MAP_FAILED;
  bufsize = mm->l3info.bufsize;
#ifdef HUGEPAGES
  if ((mm->l3info.flags & L3FLAG_NOHUGEPAGES) == 0)
  {
    mm->pagesize = HUGEPAGESIZE;
    mm->pagetype = PAGETYPE_HUGE;
    mm->l3groupsize = L3_GROUPSIZE_FOR_HUGEPAGES;
    buffer = mmap(NULL, bufsize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | HUGEPAGES, -1, 0);
  }
#endif
  if (buffer == MAP_FAILED)
  {
    mm->pagetype = PAGETYPE_SMALL;
    mm->l3groupsize = L3_SETS_PER_PAGE;
    mm->pagesize = 4096;
    buffer = mmap(NULL, bufsize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  }
  if (buffer == MAP_FAILED)
  {
    perror("Error allocating buffer!\n");
    exit(-1);
  }

  bzero(buffer, bufsize);
  return buffer;
}

mm_t mm_prepare(lxinfo_t l1info, lxinfo_t l2info, lxinfo_t l3info)
{
  mm_t mm = calloc(1, sizeof(struct mm));

  if (l1info)
    bcopy(l1info, &mm->l1info, sizeof(struct lxinfo));
  if (l2info)
    bcopy(l2info, &mm->l2info, sizeof(struct lxinfo));
  if (l3info)
    bcopy(l3info, &mm->l3info, sizeof(struct lxinfo));

  fillL1Info((l1info_t)&mm->l1info);
  fillL2Info((l2info_t)&mm->l2info);
  fillL3Info((l3info_t)&mm->l3info);

  mm->memory = vl_new();
  vl_push(mm->memory, allocate_buffer(mm));

  return mm;
}

void mm_release(mm_t mm)
{
  int len = vl_len(mm->memory);
  size_t bufferSize = mm->l3info.bufsize;
  for (int i = 0; i < len; i++)
  {
    munmap(vl_get(mm->memory, i), bufferSize);
  }
  if (mm->l3buffer)
  {
    munmap(mm->l3buffer, bufferSize);
  }
  if (mm->l3groups)
  {
    for (int i = 0; i < mm->l3ngroups; i++)
      vl_free(mm->l3groups[i]);
    free(mm->l3groups);
  }
  vl_free(mm->memory);
  free(mm);
}

#define GET_GROUP_ID(buf) (*((uint64_t *)((uintptr_t)buf + 2 * sizeof(uint64_t))))
#define SET_GROUP_ID(buf, id) (*((uint64_t *)((uintptr_t)buf + 2 * sizeof(uint64_t))) = id)

#define CHECK_ALLOCATED_FLAG(buf, offset) (*((uint64_t *)((uintptr_t)buf + offset + 3 * sizeof(uint64_t))))
#define SET_ALLOCATED_FLAG(buf, offset) (*((uint64_t *)((uintptr_t)buf + offset + 3 * sizeof(uint64_t))) = 1ul)
#define UNSET_ALLOCATED_FLAG(buf) (*((uint64_t *)((uintptr_t)buf + 3 * sizeof(uint64_t))) = 0ul)

#define L2_STRIDE ((mm->l2info.sets * L2_CACHELINE))

int mm_initialisel3(mm_t mm)
{
  if (mm->l3groups == NULL)
  {
    mm->l3buffer = allocate_buffer(mm);
    // Create the cache map
    if (!ptemap(mm))
    {
      if (!probemap(mm))
      {
        free(mm->l3buffer);
        return 0;
      }
    }
  }
  return 1;
}

static void mm_l3findlines(mm_t mm, int set, int count, vlist_t list)
{
  assert(list != NULL);

  if (!mm_initialisel3(mm))
    return;

  int i = 0;
  while (count > 0)
  {
    int list_len = vl_len(mm->memory);
    for (; i < list_len; i++)
    {
      void *buf = vl_get(mm->memory, i);

      int groupOffset = set % mm->l3groupsize;

      for (uintptr_t offset = 0; offset < mm->l3info.bufsize; offset += mm->l3groupsize * LX_CACHELINE)
      {
        void *cand = buf + offset;

        if (GET_GROUP_ID(cand) == 0)
        {
          for (int group_id = 0; group_id < mm->l3ngroups; group_id++)
          {
            clflush(cand);
            vlist_t es = mm->l3groups[group_id];
            if (checkevict(es, cand))
            {
              SET_GROUP_ID(cand, group_id + 1);
              break;
            }
          }
        }
        if ((GET_GROUP_ID(cand) - 1) == (set / mm->l3groupsize))
        {
          if (!CHECK_ALLOCATED_FLAG(cand, groupOffset * L3_CACHELINE))
          {
            SET_ALLOCATED_FLAG(cand, groupOffset * L3_CACHELINE);
            vl_push(list, cand + groupOffset * L3_CACHELINE);
            if (--count == 0)
              return;
          }
        }
      }
    }
    vl_push(mm->memory, allocate_buffer(mm));
  }
  return;
}

static void mm_l1l2findlines(mm_t mm, cachelevel_e cachelevel,
                             int line, int count, vlist_t list)
{
  assert(list != NULL);

  uintptr_t stride;
  if (cachelevel == L1)
  {
    stride = L1_STRIDE;
  }
  else if (cachelevel == L2)
  {
    stride = L2_STRIDE;
  }

  int i = 0;
  while (count > 0)
  {
    int list_len = vl_len(mm->memory);
    for (; i < list_len; i++)
    {
      void *buf = vl_get(mm->memory, i);
      for (uintptr_t offset = line * LX_CACHELINE; offset < mm->l3info.bufsize; offset += stride)
      {
        if (!CHECK_ALLOCATED_FLAG(buf, offset))
        {
          SET_ALLOCATED_FLAG(buf, offset);
          vl_push(list, buf + offset);
          if (--count == 0)
            return;
        }
      }
    }
    vl_push(mm->memory, allocate_buffer(mm));
  }
  return;
}

static int parity(uint64_t v)
{
  v ^= v >> 1;
  v ^= v >> 2;
  v = (v & 0x1111111111111111UL) * 0x1111111111111111UL;
  return (v >> 60) & 1;
}

#define SLICE_MASK_0 0x1b5f575440UL
#define SLICE_MASK_1 0x2eb5faa880UL
#define SLICE_MASK_2 0x3cccc93100UL

static int addr2slice_linear(uintptr_t addr, int slices)
{
  int bit0 = parity(addr & SLICE_MASK_0);
  int bit1 = parity(addr & SLICE_MASK_1);
  int bit2 = parity(addr & SLICE_MASK_2);
  return ((bit2 << 2) | (bit1 << 1) | bit0) & (slices - 1);
}

static uintptr_t getphysaddr(void *p)
{
#ifdef __linux__
  static int fd = -1;

  if (fd < 0)
  {
    fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0)
      return 0;
  }
  uint64_t buf;
  memaccess(p);
  uintptr_t intp = (uintptr_t)p;
  int r = pread(fd, &buf, sizeof(buf), ((uintptr_t)p) / 4096 * sizeof(buf));

  return (buf & ((1ULL << 54) - 1)) << 12 | ((uintptr_t)p & 0x3ff);
#else
  return 0;
#endif
}

static int ptemap(mm_t mm)
{
  if ((mm->l3info.flags & L3FLAG_USEPTE) == 0)
    return 0;
  if (getphysaddr(mm->l3buffer) == 0)
    return 0;
  if (mm->l3info.slices & (mm->l3info.slices - 1)) // Cannot do non-linear for now
    return 0;
  // mm->l3info.sets is equal to sets per slice
  mm->l3ngroups = mm->l3info.sets * mm->l3info.slices / mm->l3groupsize;
  mm->l3groups = (vlist_t *)calloc(mm->l3ngroups, sizeof(vlist_t));

  for (int i = 0; i < mm->l3ngroups; i++)
    mm->l3groups[i] = vl_new();

  for (int i = 0; i < mm->l3info.bufsize; i += mm->l3groupsize * L3_CACHELINE)
  {
    uintptr_t phys = getphysaddr(mm->l3buffer + i);
    int slice = addr2slice_linear(phys, mm->l3info.slices);
    int cacheindex = ((phys / L3_CACHELINE) & (mm->l3info.sets - 1));
    vlist_t list = mm->l3groups[slice * (mm->l3info.sets / mm->l3groupsize) + cacheindex / mm->l3groupsize];
    vl_push(list, mm->l3buffer + i);
  }
  return 1;
}

#define CHECKTIMES 16

static volatile uint64_t c2m;

static int timedwalk(void *list, register void *candidate)
{
#ifdef DEBUG
  static int debug = 100;
  static int debugl = 1000;
#else
#define debug 0
#endif // DEBUG
  if (list == NULL)
    return 0;
  if (LNEXT(list) == NULL)
    return 0;
  void *start = list;
  ts_t ts = ts_alloc();

  void *c2 = &c2m;

  LNEXT(c2) = candidate;
  clflush(c2);

  memaccess(candidate);
  for (int i = 0; i < CHECKTIMES * (debug ? 20 : 1); i++)
  {
    walk(list, 20);
    void *p = LNEXT(c2);
    uint32_t time = memaccesstime(p);
    ts_add(ts, time);
  }
  int rv = ts_median(ts);
#ifdef DEBUG
  if (!--debugl)
  {
    debugl = 1000;
    debug++;
  }
  if (debug)
  {
    printf("--------------------\n");
    for (int i = 0; i < TIME_MAX; i++)
      if (ts_get(ts, i) != 0)
        printf("++ %4d: %4d\n", i, ts_get(ts, i));
    debug--;
  }
#endif // DEBUG
  ts_free(ts);
  return rv;
}

int timeevict(vlist_t es, void *candidate)
{
  if (vl_len(es) == 0)
    return 0;
  for (int i = 0; i < vl_len(es); i++)
    LNEXT(vl_get(es, i)) = vl_get(es, (i + 1) % vl_len(es));
  int timecur = timedwalk(vl_get(es, 0), candidate);

  return timecur;
}

static int checkevict(vlist_t es, void *candidate)
{
  int timecur = timeevict(es, candidate);

  return timecur > L3_THRESHOLD;
}

// Read lines from es\partition[removed_partition_index] to check if
// partition[removed_partition_index] is part of the eviction set. es is
// partitioned into nPartitions sublists with remainders added to last
// sublist.
static int checkevict_remove_partition(vlist_t es, int removed_partition_index,
                                       int subl_len, int nPartitions, void *candidate)
{
  if (vl_len(es) == 0)
    return 0;

  int start_removal_ind = subl_len * removed_partition_index;
  int end_removal_ind = start_removal_ind + subl_len - 1;
  // If the removed partition is last partition, extend to end of es:
  if (removed_partition_index == nPartitions - 1)
  {
    end_removal_ind = vl_len(es) - 1;
  }

  if (end_removal_ind - start_removal_ind + 1 == vl_len(es))
  {
    // The removed partition is the entire es
    return 0;
  }

  int current_index = (end_removal_ind + 1) % vl_len(es);
  int next_index = (current_index + 1) % vl_len(es);
  while (next_index != start_removal_ind)
  {
    LNEXT(vl_get(es, current_index)) = vl_get(es, next_index);
    current_index = next_index;
    next_index = (next_index + 1) % vl_len(es);
  }
  LNEXT(vl_get(es, current_index)) = vl_get(es, (end_removal_ind + 1) % vl_len(es));
  int timecur = timedwalk(vl_get(es, (end_removal_ind + 1) % vl_len(es)), candidate);

  return timecur > L3_THRESHOLD;
}

static void contract(vlist_t es, vlist_t candidates, void *current);

static void *expand(vlist_t es, vlist_t candidates)
{
  while (vl_len(candidates) > 0)
  {
    void *current = vl_poprand(candidates);
    int time = timeevict(es, current);

    if (time > L3_THRESHOLD)
      return current;

    vl_push(es, current);
  }
  return NULL;
}

static void contract(vlist_t es, vlist_t candidates, void *current)
{
  for (int i = 0; i < vl_len(es);)
  {
    void *cand = vl_get(es, i);
    vl_del(es, i);
    clflush(current);
    if (checkevict(es, current))
      vl_push(candidates, cand);
    else
    {
      vl_insert(es, i, cand);
      i++;
    }
  }
}

// Finds minimal eviction set by repeatedly partitioning es into (minEvictionSetSize + 1) sublists of
// equal length (remainder added to last sublist) and removing sublists not part of the eviction set
static void contract_partition(vlist_t es, vlist_t candidates, int minEvictionSetSize, void *current)
{
  int nPartitions = minEvictionSetSize + 1;

  // While negative elements remain in es
  while (nPartitions <= vl_len(es))
  {
    int sublist_len = vl_len(es) / nPartitions;
    int n_sublist_positives = nPartitions;

    // Find which sublists are not part of the eviction set
    for (int i = nPartitions - 1; i >= 0; i--)
    {
      clflush(current);
      if (checkevict_remove_partition(es, i, sublist_len, nPartitions, current))
      {
        n_sublist_positives--;
        // Calculate partition length (last partition can have more elements)
        int partition_len = i == nPartitions - 1 ? sublist_len + (vl_len(es) % nPartitions) : sublist_len;
        // Remove sublist[i] and push it into candidates
        for (int j = partition_len - 1; j >= 0; j--)
        {
          vl_push(candidates, vl_del(es, sublist_len * i + j));
        }
      }
    }
    // No sublists were removed. Error will be handled in map(l3pp_t, vlist_t)
    // function since its not an eviction set
    if (n_sublist_positives == nPartitions)
    {
      return;
    }
  }
  return;
}

static void collect(vlist_t es, vlist_t candidates, vlist_t set)
{
  for (int i = vl_len(candidates); i--;)
  {
    void *p = vl_del(candidates, i);
    if (checkevict(es, p))
      vl_push(set, p);
    else
      vl_push(candidates, p);
  }
}

static vlist_t map(mm_t mm, vlist_t lines)
{
#ifdef DEBUG
  printf("%d lines\n", vl_len(lines));
#endif // DEBUG
  vlist_t groups = vl_new();
  vlist_t es = vl_new();
  int nlines = vl_len(lines);
  int fail = 0;
  while (vl_len(lines))
  {
    assert(vl_len(es) == 0);
#ifdef DEBUG
    int d_l1 = vl_len(lines);
#endif // DEBUG
    if (fail > 5)
      break;
    void *c = vl_poprand(lines);
#ifdef DEBUG
    int d_l2 = vl_len(lines);
#endif // DEBUG
    vlist_t leftovers = vl_new();
    contract_partition(lines, leftovers, mm->l3info.associativity, c);
    es = lines;
    lines = leftovers;
#ifdef DEBUG
    int d_l3 = vl_len(es);
#endif // DEBUG
    if (vl_len(es) > mm->l3info.associativity || vl_len(es) < mm->l3info.associativity - 3)
    {
      vl_push(lines, c);
      while (vl_len(es))
        vl_push(lines, vl_del(es, 0));
#ifdef DEBUG
      printf("set %3d: lines: %4d contracted: %2d failed\n", vl_len(groups), d_l1, d_l3);
#endif // DEBUG
      fail++;
      continue;
    }
    fail = 0;
    vlist_t set = vl_new();
    vl_push(set, c);
    collect(es, lines, set);
    while (vl_len(es))
      vl_push(set, vl_del(es, 0));
#ifdef DEBUG
    printf("set %3d: lines: %4d contracted: %2d collected: %d\n", vl_len(groups), d_l1, d_l3, vl_len(set));
#endif // DEBUG
    vl_push(groups, set);
    if (mm->l3info.progressNotification)
      (*mm->l3info.progressNotification)(nlines - vl_len(lines), nlines, mm->l3info.progressNotificationData);
  }

  vl_free(es);
  return groups;
}

static vlist_t quadraticmap(mm_t mm, vlist_t lines)
{
#ifdef DEBUG
  printf("%d lines\n", vl_len(lines));
#endif // DEBUG
  vlist_t groups = vl_new();
  vlist_t es = vl_new();
  int nlines = vl_len(lines);
  int fail = 0;
  while (vl_len(lines))
  {
    assert(vl_len(es) == 0);
#ifdef DEBUG
    int d_l1 = vl_len(lines);
#endif // DEBUG
    if (fail > 5)
      break;
    void *c = expand(es, lines);
#ifdef DEBUG
    int d_l2 = vl_len(es);
#endif // DEBUG
    if (c == NULL)
    {
      while (vl_len(es))
        vl_push(lines, vl_del(es, 0));
#ifdef DEBUG
      printf("set %3d: lines: %4d expanded: %4d c=NULL\n", vl_len(groups), d_l1, d_l2);
#endif // DEBUG
      fail++;
      continue;
    }
    contract(es, lines, c);
    contract(es, lines, c);
    contract(es, lines, c);
#ifdef DEBUG
    int d_l3 = vl_len(es);
#endif // DEBUG
    if (vl_len(es) > mm->l3info.associativity || vl_len(es) < mm->l3info.associativity - 3)
    {
      while (vl_len(es))
        vl_push(lines, vl_del(es, 0));
#ifdef DEBUG
      printf("set %3d: lines: %4d expanded: %4d contracted: %2d failed\n", vl_len(groups), d_l1, d_l2, d_l3);
#endif // DEBUG
      fail++;
      continue;
    }
    fail = 0;
    vlist_t set = vl_new();
    vl_push(set, c);
    collect(es, lines, set);
    while (vl_len(es))
      vl_push(set, vl_del(es, 0));
#ifdef DEBUG
    printf("set %3d: lines: %4d expanded: %4d contracted: %2d collected: %d\n", vl_len(groups), d_l1, d_l2, d_l3, vl_len(set));
#endif // DEBUG
    vl_push(groups, set);
    if (mm->l3info.progressNotification)
      (*mm->l3info.progressNotification)(nlines - vl_len(lines), nlines, mm->l3info.progressNotificationData);
  }

  vl_free(es);
  return groups;
}

static int probemap(mm_t mm)
{
  if ((mm->l3info.flags & LXFLAG_NOPROBE) != 0)
    return 0;
  vlist_t pages = vl_new();
  for (int i = 0; i < mm->l3info.bufsize; i += mm->l3groupsize * LX_CACHELINE)
    vl_push(pages, mm->l3buffer + i);
  vlist_t groups = vl_new();

  if ((mm->l3info.flags & LXFLAG_QUADRATICMAP) != 0)
  {
    groups = quadraticmap(mm, pages);
  }
  else if ((mm->l3info.flags & LXFLAG_LINEARMAP) != 0)
  {
    groups = map(mm, pages);
  }
  // If quadratic or linear map aren't specified, default to fastest behavior based on small/huge pages
  else if ((mm->l3info.flags & LXFLAG_NOHUGEPAGES) != 0)
  {
    groups = map(mm, pages);
  }
  else
  {
    groups = quadraticmap(mm, pages);
  }
  // Store map results
  mm->l3ngroups = vl_len(groups);

  mm->l3groups = (vlist_t *)calloc(mm->l3ngroups, sizeof(vlist_t));
  for (int i = 0; i < vl_len(groups); i++)
    mm->l3groups[i] = vl_get(groups, i);
  vl_free(groups);
  vl_free(pages);
  return 1;
}

void _mm_requestlines(mm_t mm, cachelevel_e cachelevel, int line, int count, vlist_t list)
{
  switch (cachelevel)
  {
  case L1:
    return mm_l1l2findlines(mm, cachelevel, line, count, list);
  case L2:
    if (mm->pagetype == PAGETYPE_HUGE)
      return mm_l1l2findlines(mm, cachelevel, line, count, list);
    else
      perror("Operation not supported yet!\n");
    exit(-2);
  case L3:
    return mm_l3findlines(mm, line, count, list);
  }
}

void *mm_requestline(mm_t mm, cachelevel_e cachelevel, int line)
{
  vlist_t vl = vl_new();
  _mm_requestlines(mm, cachelevel, line, 1, vl);
  void *mem = vl_get(vl, 0);
  vl_free(vl);
  return mem;
}

void mm_requestlines(mm_t mm, cachelevel_e cachelevel, int line, void **lines, int count)
{
  vlist_t vl = vl_new();
  _mm_requestlines(mm, cachelevel, line, count, vl);
  for (int i = 0; i < count; i++)
  {
    lines[i] = vl_get(vl, i);
  }
  vl_free(vl);
  return;
}

void mm_returnline(mm_t mm, void *line)
{
  UNSET_ALLOCATED_FLAG(line);
}

void _mm_returnlines(mm_t mm, vlist_t lines)
{
  int len = vl_len(lines);
  for (int i = 0; i < len; i++)
    mm_returnline(mm, vl_get(lines, i));
}

void mm_returnlines(mm_t mm, void **lines, int count)
{
  for (int i = 0; i < count; i++)
  {
    mm_returnline(mm, lines[i]);
  }
}