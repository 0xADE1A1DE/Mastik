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
#include <mastik/fr.h>
#include <mastik/util.h>

int main(int ac, char **av) {
  fr_t fr = fr_prepare();

  void *ptr = map_offset("FR-1-file-access.c", 0);
  fr_monitor(fr, ptr);

  uint16_t res[1];
  fr_probe(fr, res);

  for (;;) {
    fr_probe(fr, res);
    if (res[0] < 100)
      printf("FR-1-file-access.c  accessed\n");
    delayloop(10000);
  }
}

