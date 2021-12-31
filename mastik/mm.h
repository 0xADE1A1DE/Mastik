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

#ifndef __MM_H__
#define __MM_H__ 1

#include <mastik/low.h>
#include <mastik/info.h>

#ifdef MAP_HUGETLB
#define HUGEPAGES MAP_HUGETLB
#endif
#ifdef VM_FLAGS_SUPERPAGE_SIZE_2MB
#define HUGEPAGES VM_FLAGS_SUPERPAGE_SIZE_2MB
#endif

#ifdef HUGEPAGES
#define HUGEPAGEBITS 21
#define HUGEPAGESIZE (1<<HUGEPAGEBITS)
#define HUGEPAGEMASK (HUGEPAGESIZE - 1)
#endif


enum pagetype {
  PAGETYPE_SMALL,
  PAGETYPE_HUGE
};
typedef enum pagetype pagetype_e;

enum cachelevel {
  L1,
  L2,
  L3
};
typedef enum cachelevel cachelevel_e;


typedef struct mm* mm_t;

mm_t mm_prepare(lxinfo_t l1info, lxinfo_t l2info, lxinfo_t l3info);

void* mm_requestline(mm_t mm, cachelevel_e cachelevel, int line);

void mm_requestlines(mm_t mm, cachelevel_e cachelevel, int line, void** lines, int count);

void mm_returnline(mm_t mm, void* line);

void mm_returnlines(mm_t mm, void** lines, int count);

void mm_release(mm_t mm);

int mm_initialisel3(mm_t mm);


#endif // __MM_H__
