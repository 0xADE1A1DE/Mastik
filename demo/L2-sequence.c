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
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>

#include <sys/mman.h>
#define L2_SETS 1024

#include <mastik/mm.h>
#include <mastik/lx.h>
#include <mastik/l1.h>
#include <mastik/l2.h>
#include <mastik/l3.h>

#define TEST_RANGE 200

void grayspace(int64_t intensity, int64_t min, int64_t max, char c)
{
  printf("\e[48;5;%ld;31;1m%c\e[0m", 232 + ((intensity - min) * 24) / (max - min), c);
}

void display(int64_t *data[L2_SETS])
{
  int64_t min = INT64_MAX;
  int64_t max = INT64_MIN;

  for (int i = 0; i < TEST_RANGE; i++)
  {
    for (int j = 0; j < L2_SETS; j++)
    {
      if (min > data[i][j])
        min = data[i][j];
      if (max < data[i][j])
        max = data[i][j];
    }
  }

  if (max == min)
    max++;
  for (int itr = 0; itr < TEST_RANGE; itr++)
  {
    printf("%05X. ", itr);
    for (int j = 0; j < L2_SETS; j++)
    {
      grayspace(data[itr][j], min, max, ' ');
    }
    printf("\n");
  }
}

char *readBuffer;

void exec(lxpp_t l1, int recnum, void *data)
{
  register uint16_t num = (uint16_t)(uintptr_t)data;
  for (volatile int i = 0; i < 100; i++)
    readBuffer[num * L2_CACHELINE] = i;
  return;
}

#define SCALE 1000
#define SAMPLES 10000

static void normalise(int64_t *avg[TEST_RANGE])
{
  uint64_t set_avg[1024];
  bzero(set_avg, sizeof(set_avg));

  for (int key = 0; key < TEST_RANGE; key++)
  {
    for (int set = 0; set < L2_SETS; set++)
    {
      avg[key][set] = (SCALE * avg[key][set] / SAMPLES);
      set_avg[set] += avg[key][set];
    }
  }
  for (int i = 0; i < 1024; i++)
  {
    set_avg[i] /= TEST_RANGE;
  }
  for (int key = 0; key < TEST_RANGE; key++)
  {
    for (int set = 0; set < 1024; set++)
    {
      avg[key][set] -= set_avg[set];
    }
  }
}

int main(int argc, char **argv)
{
  l2pp_t l2 = l2_prepare(NULL, NULL);
  l2info_t l2info = (l2info_t)malloc(sizeof(struct l2info));

  l2_getl2info(l2, l2info);
  const int nsets = l2info->sets;

  int monitoredSets = l2_getmonitoredset(l2, NULL, nsets);
  readBuffer = mmap(NULL, monitoredSets * L2_CACHELINE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | HUGEPAGES, -1, 0);

  int64_t *avg[TEST_RANGE];
  for (int i = 0; i < TEST_RANGE; i++)
  {
    avg[i] = calloc(1024, sizeof(int64_t));
  }

  for (int i = 0; i < TEST_RANGE; i++)
  {
    uint16_t key = i;

    int *map = calloc(monitoredSets, sizeof(int));
    uint16_t *res = calloc(SAMPLES * monitoredSets, sizeof(uint16_t));
    l2_randomise(l2);
    l2_getmonitoredset(l2, map, monitoredSets);

    l2_syncpp(l2, SAMPLES, res, lx_dummy_cb, exec, (void *)(uint64_t)key);

    for (int sample = 0; sample < SAMPLES; sample++)
    {
      for (int idx = 0; idx < monitoredSets; idx++)
      {
        // printf("%d\n", map[idx]);
        avg[i][map[idx]] += res[idx + monitoredSets * sample];
      }
    }

    free(map);
    free(res);
  }
  normalise(avg);
  display(avg);
  munmap(readBuffer, monitoredSets * L2_CACHELINE);
}
