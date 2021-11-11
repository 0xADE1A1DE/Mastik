/*
 * Copyright 2017 CSIRO
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

#ifndef __CB_H__
#define __CB_H__ 1

#include <stddef.h>
#include <stdint.h>


#define CBT_BANK_CONFLICT	0
#define CBT_FALSE_DEPENDENCY	1



typedef struct cb *cb_t;

cb_t cb_prepare(int type);
void cb_release(cb_t cb);

int cb_maxoffset(cb_t cb);
int cb_offsetmask(cb_t cb);

int cb_monitor(cb_t cb, int offset, int accesses);
int cb_getmonitored_offset(cb_t cb);
int cb_getmonitored_accesses(cb_t cb);

void cb_probe(cb_t cb, uint32_t *results);
void cb_bprobe(cb_t cb, uint32_t *results);

int cb_repeatedprobe(cb_t cb, int nrecords, uint32_t *results);
uint32_t cb_repeatedproberaw(cb_t cb, int nrecords, uint32_t *results);



#endif // __CB_H__

