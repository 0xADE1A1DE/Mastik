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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mastik/fr.h>
#include <mastik/util.h>

#define SAMPLES 100000

int forkslave(void *ptr);
int compare(const void *p1, const void *p2);


int main(int ac, char **av) {
  fr_t fr = fr_prepare();

  void *ptr = map_offset("FR-threshold.c", 0);
  fr_monitor(fr, ptr);

  uint16_t *memory = malloc(SAMPLES * sizeof(uint16_t));
  memset(memory, 1, SAMPLES * sizeof(uint16_t));
  delayloop(1000000000);

  for (int i = 0; i < SAMPLES; i++)  {
    fr_probe(fr, memory + i);
    delayloop(10000);
  }

  pid_t pid = forkslave(ptr);
  if (pid == -1) {
    perror("fork");
    exit(1);
  }

  uint16_t *cache = malloc(SAMPLES * sizeof(uint16_t));
  memset(cache, 1, SAMPLES * sizeof(uint16_t));

  for (int i = 0; i < SAMPLES; i++)  {
    fr_probe(fr, cache + i);
    delayloop(10000);
  }

  kill(pid, SIGKILL);

  qsort(memory, SAMPLES, sizeof(uint16_t), compare);
  qsort(cache, SAMPLES, sizeof(uint16_t), compare);
  printf("             :  Mem  Cache\n");
  printf("Minimum      : %5d %5d\n", memory[0], cache[0]);
  printf("Bottom decile: %5d %5d\n", memory[SAMPLES/10], cache[SAMPLES/10]);
  printf("Median       : %5d %5d\n", memory[SAMPLES/2], cache[SAMPLES/2]);
  printf("Top decile   : %5d %5d\n", memory[(SAMPLES * 9)/10], cache[(SAMPLES * 9)/10]);
  printf("Maximum      : %5d %5d\n", memory[SAMPLES-1], cache[SAMPLES-1]);

  free(memory);
  free(cache);
  fr_release(fr);
}

volatile int a;

int forkslave(void *ptr) {
  pid_t pid = fork();
  if (pid != 0)
    return pid;
  for(;;)
    a += *(char *)ptr;
}

int compare(const void *p1, const void *p2) {
  uint16_t u1 = *(uint16_t *)p1;
  uint16_t u2 = *(uint16_t *)p2;

  return (int)u1 - (int)u2;
}

