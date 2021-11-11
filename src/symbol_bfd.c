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
#ifndef HAVE_BFD_H
#error Must have bfd to compile this
#endif

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <bfd.h>
#if defined(HAVE_LIBDWARF_H)
#include <libdwarf.h>
#elif defined(HAVE_LIBDWARF_LIBDWARF_H)
#include <libdwarf/libdwarf.h>
#endif

#include <mastik/symbol.h>

static void initialise() {
  static int init = 0;
  if (init)
    return;
    
  bfd_init();
  init = 1;
}

uint64_t sym_loadersymboloffset(const char *file, const char *name) {
  initialise();

  bfd *abfd = NULL;
  asymbol **symbol_table = NULL;
  uint64_t rv = ~0ULL;

  abfd = bfd_openr(file, "default");
  if (abfd == NULL)
    goto out;



  if (!bfd_check_format(abfd, bfd_object)) {
    if (bfd_get_error () != bfd_error_file_ambiguously_recognized) 
      goto out;
  }

  long storage_needed = bfd_get_symtab_upper_bound (abfd);
  if (storage_needed <= 0) 
    goto out;

  symbol_table = (asymbol **) malloc (storage_needed);

  long number_of_symbols = bfd_canonicalize_symtab (abfd, symbol_table);
  if (number_of_symbols < 0) 
    goto out;

  for (long i = 0; i < number_of_symbols; i++) {
    if (!strcmp(name, symbol_table[i]->name)) {
      struct bfd_section *section = symbol_table[i]->section;
      rv = symbol_table[i]->value + section->filepos;
      goto out;
    }
  }


out:
  if (symbol_table != NULL)
    free(symbol_table);
  if (abfd != NULL)
    bfd_close(abfd);
  return rv;
}

uint64_t sym_addresstooffset(const char *file, uint64_t address) {
  initialise();

  bfd *abfd = NULL;
  uint64_t rv = ~0ULL;

  abfd = bfd_openr(file, "default");
  if (abfd == NULL)
    goto out;

  if (!bfd_check_format(abfd, bfd_object)) {
    if (bfd_get_error () != bfd_error_file_ambiguously_recognized) 
      goto out;
  }


// Interface for bfd changed in verison 2.33, support older versions as well
#ifdef HAVE_LEGACY_BFD

  for (asection *s = abfd->sections; s; s = s->next) {
    if (bfd_get_section_flags (abfd, s) & (SEC_LOAD)) {
      uint64_t vma = bfd_section_vma(abfd, s);
      uint64_t size = bfd_section_size(abfd, s);
      if (address >= vma && address - vma  < size) {
	rv = address - vma + s->filepos;
	break;
      }
    }
  }

#else

  for (asection *s = abfd->sections; s; s = s->next) {
    if (bfd_section_flags (s) & (SEC_LOAD)) {
      uint64_t vma = bfd_section_vma(s);
      uint64_t size = bfd_section_size(s);
      if (address >= vma && address - vma  < size) {
	rv = address - vma + s->filepos;
	break;
      }
    }
  }

#endif

out:
  if (abfd != NULL)
    bfd_close(abfd);
  return rv;
}


#ifdef HAVE_DWARF
static void dwarf_handler(Dwarf_Error error, Dwarf_Ptr errarg) {
  Dwarf_Debug dbg = *(Dwarf_Debug *)errarg;
  dwarf_dealloc(dbg, error, DW_DLA_ERROR);
}
#endif

uint64_t sym_debuglineoffset(const char *file, const char *src, int lineno) {
#ifdef HAVE_DWARF
  Dwarf_Debug dbg = NULL;
  int res = DW_DLV_ERROR;
  Dwarf_Error error = NULL;
  Dwarf_Unsigned next_cu_header;
  Dwarf_Die die = NULL;
  char *name = NULL;
  Dwarf_Line *linebuf = NULL;
  Dwarf_Signed linecount;
  Dwarf_Unsigned line;
  Dwarf_Addr lineaddr;
  int fd = -1;
  uint64_t rv = -1;

  fd = open(file, O_RDONLY);
  if(fd < 0)
    goto out;

  res = dwarf_init(fd, DW_DLC_READ, dwarf_handler, (Dwarf_Ptr)&dbg, &dbg, &error);
  if (res != DW_DLV_OK) {
    if (error)
      free(error);
    goto out;
  }

  for (;;) {
    res = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, &next_cu_header, NULL);
    if (res != DW_DLV_OK)
      goto out;

    // The first sibling is a DW_TAG_compile_unit
    res = dwarf_siblingof(dbg, NULL, &die, NULL);
    if (res != DW_DLV_OK) 
      goto out;

    res = dwarf_diename(die, &name, NULL);
    if (res != DW_DLV_OK)
      goto out;

    int found = !strcmp(name, src);

    dwarf_dealloc(dbg, name, DW_DLA_STRING);
    name = NULL;
    if (found)
      break;
    
    dwarf_dealloc(dbg, die, DW_DLA_DIE);
    die = NULL;
  }

  res = dwarf_srclines(die, &linebuf, &linecount, &error);
  if(res != DW_DLV_OK) 
    goto out;
  int nextline = 0;
  uint64_t nextaddr = ~0ULL;
  for (int i = 0; i < linecount; i++) {
    dwarf_lineno(linebuf[i], &line, &error);
    dwarf_lineaddr(linebuf[i], &lineaddr, &error);
    if (rv == ~0ULL) {
      if (lineno == line)
	rv = lineaddr;
      else if (line > lineno) {
	if (nextline == 0 || nextline > line) {
	  nextline = line;
	  nextaddr = lineaddr;
	}
      }
    }
    dwarf_dealloc(dbg, linebuf[i], DW_DLA_LINE);
  }

  dwarf_dealloc(dbg, linebuf, DW_DLA_LIST);

  if (rv == ~0ULL) 
    rv = nextaddr;
  if (rv != ~0ULL)
    rv = sym_addresstooffset(file, rv);


out:
  if (name != NULL)
    dwarf_dealloc(dbg, name, DW_DLA_STRING);
  if (die != NULL)
    dwarf_dealloc(dbg, die, DW_DLA_DIE);
  if (dbg != NULL)
    dwarf_finish(dbg, NULL);
  if (fd >=0)
    close(fd);
  return rv;
#else
  return ~0ULL;
#endif
}


