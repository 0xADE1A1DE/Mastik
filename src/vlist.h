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

#ifndef __VLIST_H__
#define __VLIST_H__

#include <assert.h>

typedef struct vlist *vlist_t;

vlist_t vl_new();
void vl_free(vlist_t vl);

inline void *vl_get(vlist_t vl, int ind);
void vl_set(vlist_t vl, int ind, void *dat);
int vl_push(vlist_t vl, void *dat);
void *vl_pop(vlist_t vl);
void *vl_poprand(vlist_t vl);
void *vl_del(vlist_t vl, int ind);
inline int vl_len(vlist_t vl);
void vl_insert(vlist_t vl, int ind, void *dat);
int vl_find(vlist_t vl, void *dat);


//---------------------------------------------
// Implementation details
//---------------------------------------------

struct vlist {
  int size;
  int len;
  void **data;
};

inline void *vl_get(vlist_t vl, int ind) {
  assert(vl != NULL);
  assert(ind < vl->len);
  return vl->data[ind];
}

inline int vl_len(vlist_t vl) {
  assert(vl != NULL);
  return vl->len;
}



#endif // __VLIST_H__
