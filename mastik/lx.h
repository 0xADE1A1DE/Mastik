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

#ifndef __LX_H__
#define __LX_H__ 1

#include <stdint.h>

#include <mastik/low.h>
#include <mastik/mm.h>
#include <mastik/info.h>

struct vlist;
typedef struct vlist *vlist_t;

struct lxpp {
  void **monitoredhead;
  int nmonitored;
  
  int *monitoredset;
  
  uint32_t *monitoredbitmap;
  
  int level;
  size_t totalsets;
  
  int ngroups;
  int groupsize;
  vlist_t *groups;
  
  struct lxinfo lxinfo;
  
  mm_t mm;
  uint8_t internalmm;
};

typedef struct lxpp *lxpp_t;


#endif // __LX_H__
