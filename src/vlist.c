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
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include "vlist.h"

extern void *vl_get(vlist_t vl, int ind);
extern int vl_len(vlist_t vl);

#define VLIST_DEF_SIZE 16

vlist_t vl_new() {
  vlist_t vl = (vlist_t)malloc(sizeof(struct vlist));
  vl->size = VLIST_DEF_SIZE;
  vl->data = (void **)calloc(VLIST_DEF_SIZE, sizeof(void *));
  vl->len = 0;
  return vl;
}

void vl_free(vlist_t vl) {
  assert(vl != NULL);
  free(vl->data);
  bzero(vl, sizeof(struct vlist));
  free(vl);
}

void vl_set(vlist_t vl, int ind, void *dat) {
  assert(vl != NULL);
  assert(dat != NULL);
  assert(ind < vl->len);
  vl->data[ind] = dat;
}

static void vl_setsize(vlist_t vl, int size) {
  assert(vl != NULL);
  assert(size >= vl->len);
  void **old = vl->data;
  vl->data = (void **)realloc(old, size * sizeof(void *));
  vl->size = size;
}


int vl_push(vlist_t vl, void *dat) {
  assert(vl != NULL);
  assert(dat != NULL);
  if (vl->len == vl->size)
    vl_setsize(vl, vl->size * 2);
  assert(vl->len < vl->size);
  vl->data[vl->len++] = dat;
  return vl->len - 1;
}

void *vl_pop(vlist_t vl) {
  assert(vl != NULL);
  if (vl->len == 0)
    return NULL;
  return vl->data[--vl->len];
}

void *vl_del(vlist_t vl, int ind) {
  assert(vl != NULL);
  assert(ind < vl->len);
  void * rv = vl->data[ind];
  vl->data[ind] = vl->data[--vl->len];
  return rv;
}

void *vl_poprand(vlist_t vl) {
  assert(vl != NULL);
  if (vl->len == 0)
    return NULL;
  int ind = random() % vl->len;
  void *rv = vl->data[ind];
  vl_del(vl, ind);
  return rv;
}

void vl_insert(vlist_t vl, int ind, void *dat) {
  assert(vl != NULL);
  assert(dat != NULL);
  assert(ind <= vl->len);
  if (ind == vl->len) {
    vl_push(vl, dat);
  } else {
    vl_push(vl, vl->data[ind]);
    vl->data[ind] = dat;
  }
}

int vl_find(vlist_t vl, void *dat) {
  assert(vl != NULL);
  assert(dat != NULL);
  for (int i = 0; i < vl->len; i++)
    if (vl->data[i] == dat)
      return i;
  return -1;
}

