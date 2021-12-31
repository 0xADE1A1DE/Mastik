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

#ifndef MM_IMPL_H
#define MM_IMPL_H

struct mm {
  vlist_t memory;
  size_t pagesize;
  
  struct lxinfo l1info;
  struct lxinfo l2info;
  struct lxinfo l3info;
  
  int l3ngroups;
  int l3groupsize;
  vlist_t *l3groups;
  void* l3buffer;
  
  pagetype_e pagetype;
};

void _mm_requestlines(mm_t mm, cachelevel_e cachelevel, int line, int count, vlist_t list);
void _mm_returnlines(mm_t mm, vlist_t line);
int timeevict(vlist_t es, void *candidate);

#endif
