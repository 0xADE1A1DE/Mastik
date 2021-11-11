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

#include "config.h"
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <mastik/symbol.h>


/*
 * Convert a symbolic reference to a file offset.  Accepted formats are:
 *    <offset>     - offset to file
 *    @<address>   - virtual address of a location
 *    <name>       - symbolic name, usually a function
 *    <src>:<line> - Start of a source line
 * All optionally followed by [+-]<offset>
 */
uint64_t sym_getsymboloffset(const char *file, const char *symbol) {
  char *symcopy = strdup(symbol);
  uint64_t rv = ~0ULL;
  char *srcfile = NULL;
  uint32_t lineno = 0;
  int offset = 0;

  char *ptr = strchr(symcopy, ':');
  if (ptr != NULL) {
    *ptr = '\0';
    srcfile = symcopy;
    ptr++;
  } else {
    ptr = symcopy;
  }

  char *tmp = strchr(ptr, '+');
  if (tmp != NULL) {
    *tmp = '\0';
    tmp++;
    offset = strtoul(tmp, NULL, 0);
  } else {
    tmp = strchr(ptr, '-');
    if (tmp != NULL) {
      *tmp = '\0';
      tmp++;
      offset = -strtoul(tmp, NULL, 0);
    }
  }

  if (srcfile) {
    lineno = strtoul(ptr, NULL, 0);
    rv = sym_debuglineoffset(file, srcfile, lineno);
  } else if (*ptr == '@') {
    rv = sym_addresstooffset(file, strtoull(ptr+1, NULL, 0));
  } else if (isdigit(*ptr)) {
    rv = strtoull(ptr, NULL, 0);
  } else {
    rv = sym_loadersymboloffset(file, ptr);
  }

  if (rv != ~0ULL)
    rv += offset;

out:
  if (symcopy)
    free(symcopy);
  return rv;
}


#ifndef HAVE_SYMBOLS
uint64_t sym_loadersymboloffset(const char *file, const char *name) {
  return ~0ULL;
}

uint64_t sym_addresstooffset(const char *file, uint64_t address) {
  return ~0ULL;
}

uint64_t sym_debuglineoffset(const char *file, const char *src, int lineno) {
  return ~0ULL;
}
#endif

