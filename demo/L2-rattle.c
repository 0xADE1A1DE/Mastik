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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <sys/mman.h>
#include <mastik/l2.h>
#include <mastik/mm.h>

int main(int ac, char **av) {
  const uint64_t nsets = 1024;
  const uint64_t nways = 8;
  const uint64_t bufSize = nsets * nways * L2_CACHELINE;
  
  const uint64_t L2_STRIDE = nsets * L2_CACHELINE;
  
  char* buffer = mmap(0, bufSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON|HUGEPAGES, -1, 0);
  const uint64_t targetSet = 78;
  const uint64_t targetAddress = 78 * L2_CACHELINE;
  const uint64_t targetSet2 = 81;
  const uint64_t targetAddress2 = 81 * L2_CACHELINE;
  for (;;) {
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress + L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress + 2 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress + 3 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress + 4 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress + 5 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress + 6 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress + 7 * L2_STRIDE] += i;
      
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2 + L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2 + 2 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2 + 3 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2 + 4 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2 + 5 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2 + 6 * L2_STRIDE] += i;
    for (int i = 0; i < 5000; i++)
      buffer[targetAddress2 + 7 * L2_STRIDE] += i;
  }
}
