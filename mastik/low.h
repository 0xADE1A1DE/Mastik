/*
 * Copyright 2016,2020 CSIRO
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

#ifndef __LOW_H__
#define __LOW_H__ 1

#include <stdint.h>

#define L3_THRESHOLD 140

#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif
#define PAGE_SIZE 4096


static inline int memaccess(void *v) {
  int rv;
  asm volatile("mov (%1), %0": "+r" (rv): "r" (v):);
  return rv;
}

static inline uint32_t memaccesstime(void *v) {
  uint32_t rv;
  asm volatile (
      "mfence\n"
      "lfence\n"
      "rdtscp\n"
      "mov %%eax, %%esi\n"
      "mov (%1), %%eax\n"
      "rdtscp\n"
      "sub %%esi, %%eax\n"
      : "=&a" (rv): "r" (v): "ecx", "edx", "esi");
  return rv;
}

static inline void clflush(void *v) {
  asm volatile ("clflush 0(%0)": : "r" (v):);
}

static inline uint32_t rdtscp() {
  uint32_t rv;
  asm volatile ("rdtscp": "=a" (rv) :: "edx", "ecx");
  return rv;
}

static inline uint64_t rdtscp64() {
  uint32_t low, high;
  asm volatile ("rdtscp": "=a" (low), "=d" (high) :: "ecx");
  return (((uint64_t)high) << 32) | low;
}

static inline void mfence() {
  asm volatile("mfence");
}

static inline void lfence() {
  asm volatile("lfence");
}

static inline void walk(void *p, int count) {
  if (p == NULL)
    return;
  asm volatile(
    "movq %0, %%rsi\n"
    "1:\n"
    "movq (%0), %0\n"
    "cmpq %0, %%rsi\n"
    "jnz 1b\n"
    "decl %1\n"
    "jnz 1b\n"
    : "+r" (p), "+r" (count)::"rsi");
}


struct cpuidRegs {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

#define CPUID_CACHEINFO		4


#define CACHETYPE_NULL          0
#define CACHETYPE_DATA          1
#define CACHETYPE_INSTRUCTION   2
#define CACHETYPE_UNIFIED       3

struct cpuidCacheInfo {
  uint32_t      type:5;
  uint32_t      level:3;
  uint32_t      selfInitializing:1;
  uint32_t      fullyAssociative:1;
  uint32_t      reserved1:4;
  uint32_t      logIds:12;
  uint32_t      phyIds:6;

  uint32_t      lineSize:12;
  uint32_t      partitions:10;
  uint32_t      associativity:10;

  uint32_t      sets:32;

  uint32_t      wbinvd:1;
  uint32_t      inclusive:1;
  uint32_t      complexIndex:1;
  uint32_t      reserved2:29;
};

union cpuid {
  struct cpuidRegs regs;
  struct cpuidCacheInfo cacheInfo;
};

inline void cpuid(union cpuid *c) {
  asm volatile ("cpuid": "+a" (c->regs.eax), "+b" (c->regs.ebx), "+c" (c->regs.ecx), "+d" (c->regs.edx));
}

static inline int slotwait(uint64_t slotend) {
  if (rdtscp64() > slotend)
    return 1;
  while (rdtscp64() < slotend)
    ;
  return 0;
}



#endif //__LOW_H__
