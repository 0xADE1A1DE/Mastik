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
#include <mastik/pda.h>
#include <mastik/util.h>
#include <mastik/symbol.h>

#include <sys/types.h>
#include <sys/wait.h>

#define BINARY "gpg-1.4.13"

int main(int ac, char **av) {
  pda_t pda = pda_prepare();

  void *ptr = map_offset(BINARY, sym_getsymboloffset(BINARY, "mpihelp_sub_n")); 
  void *ptr2 = map_offset(BINARY, sym_getsymboloffset(BINARY, "mpihelp_sub_n+64"));  
  if (ptr == NULL || ptr2==NULL) {
    printf("Bad symbol translation\n");
    exit(1);
  }
  pda_target(pda, ptr);
  pda_target(pda, ptr2);

  pda_activate(pda);
  wait(NULL);
}

