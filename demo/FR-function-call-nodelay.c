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
  void *ptr = map_offset("/usr/lib64/libc-2.17.so", 0x6d300);
  if (ptr == NULL)
    exit(0);

  fr_t fr = fr_prepare();
  fr_monitor(fr, ptr);

  uint16_t res[1];
  fr_probe(fr, res);

  int lines=0;
  for (;;) {
    fr_probe(fr, res);
    if (res[0] < 100)
      printf("%4d: %s", ++lines, "Call to puts\n");
  }
}

