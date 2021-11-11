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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <libkern/OSByteOrder.h>
#include <mach/mach.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#ifndef __APPLE__
#error No use compiling a Mac OS X file here
#endif


static void *map_file(const char *name, off_t *size) {
  struct stat sb;
  int fd = open(name, O_RDONLY);
  if (fd < 0)
    return MAP_FAILED;
  if (fstat(fd, &sb) < 0) {
    close(fd);
    return MAP_FAILED;
  }

  *size = sb.st_size;
  void *rv = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return rv;
}

static void *handle_fat(void *adrs) {
  if (OSSwapBigToHostInt32(*(uint32_t *)adrs) != FAT_MAGIC)
    return adrs;

  struct host_basic_info basic_info;
  unsigned int count = sizeof(basic_info)/sizeof(int);
  if (host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)(&basic_info), &count) != KERN_SUCCESS) 
    return 0;

  struct fat_header *fh = (struct fat_header *)adrs;
  uint32_t narch = OSSwapBigToHostInt32(fh->nfat_arch);

  struct fat_arch *archs = (struct fat_arch *)(fh + 1);
  int index = -1;
  for (int i = 0; i < narch; i++) {
    uint32_t fatcputype = OSSwapBigToHostInt32(archs[i].cputype);

    // We choose the 64-bit ABI if exists.
    if (fatcputype == (basic_info.cpu_type | CPU_ARCH_ABI64))
      index = i;
    if (fatcputype == basic_info.cpu_type && index != -1)
      index = i;
  }

  if (index == -1)
    return NULL;
  return (void *)((uintptr_t)adrs + OSSwapBigToHostInt32(archs[index].offset));
}


static uint64_t addrstooffset64(void *base, uint64_t vaddr) {
  struct mach_header_64 *mh = (struct mach_header_64 *)base;
  struct load_command *lc = (struct load_command *)(mh + 1);
  int count = mh->ncmds;

  while (--count) {
    switch (lc->cmd) {
      case LC_SEGMENT: // Can this even happen?
	break;
      case LC_SEGMENT_64: 
	{
	  struct segment_command_64 *sc = (struct segment_command_64 *)lc;
	  if (vaddr >= sc->vmaddr && (vaddr - sc->vmaddr) < sc->filesize)
	    return vaddr - sc->vmaddr + sc->fileoff;
	  break;
	}
    }
    lc = (struct load_command *)((uintptr_t)lc + lc->cmdsize);
  }

  return ~0ULL;
}

static uint64_t getsymoffset64(void *base, const char *name) {
  struct mach_header_64 *mh = (struct mach_header_64 *)base;
  struct load_command *lc = (struct load_command *)(mh + 1);
  int count = mh->ncmds;

  while (--count) {
    switch (lc->cmd) {
      case LC_SYMTAB:
	{
	  struct symtab_command *sc = (struct symtab_command *)lc;

	  struct nlist_64 *symbols = (struct nlist_64 *)((uintptr_t)base + sc->symoff);
	  for (int i = 0; i < sc->nsyms; i++) {
	    char *p = (char *)base + sc->stroff + symbols[i].n_un.n_strx;
	    if ((symbols[i].n_type & N_TYPE) != N_SECT)
	      continue;
	    if (*p == '_')
	      p++;
	    if (strcmp(p, name))
	      continue;
	    return addrstooffset64(base, symbols[i].n_value);
	  }
	}
	break;
    }

    lc = (struct load_command *)((uintptr_t)lc + lc->cmdsize);
  }

  return ~0ULL;
}

uint64_t sym_loadersymboloffset(const char *file, const char *name) {
  uint64_t rv = ~0ULL;
  off_t size = 0;
  void *adrs = map_file(file, &size);
  void *base = handle_fat(adrs);
  if (base == NULL)
    goto out;
  switch (*(uint32_t *)base) {
    /*
    case MH_MAGIC:
      rv = getsymoffset32(base, name);
      break;
    */
    case MH_MAGIC_64:
      rv = getsymoffset64(base, name);
      break;
  }
  if (rv != ~0ULL)
    rv += (uintptr_t)base - (uintptr_t)adrs;
out:
  if (adrs != MAP_FAILED)
    munmap(adrs, size);
  return rv;
}


uint64_t sym_addresstooffset(const char *file, uint64_t address) {
  uint64_t rv = ~0ULL;
  off_t size = 0;
  void *adrs = map_file(file, &size);
  
  // Later we might want to support fat files here...

  switch (*(uint32_t *)adrs) {
    /*
    case MH_MAGIC:
      rv = addrstooffset32(adrs, address);
      break;
    */
    case MH_MAGIC_64:
      rv = addrstooffset64(adrs, address);
      break;
  }
  munmap(adrs, size);
  return rv;
}


uint64_t sym_debuglineoffset(const char *file, const char *src, int lineno) {
  return ~0ULL;
}





