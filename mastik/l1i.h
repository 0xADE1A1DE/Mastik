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

#ifndef __L1I_H__
#define __L1I_H__ 1

#define L1I_SETS 64

typedef struct l1ipp *l1ipp_t;

l1ipp_t l1i_prepare();
void l1i_release(l1ipp_t l1i);

int l1i_monitor(l1ipp_t l1i, int line);
int l1i_unmonitor(l1ipp_t l1i, int line);
void l1i_monitorall(l1ipp_t l1i);
void l1i_unmonitorall(l1ipp_t l1i);
int l1i_getmonitoredset(l1ipp_t l1i, int *lines, int nlines);
void l1i_randomise(l1ipp_t l1);

void l1i_probe(l1ipp_t l1, uint16_t *results);

// Slot is currently not implemented
int l1i_repeatedprobe(l1ipp_t l1, int nrecords, uint16_t *results, int slot);



#endif // __L1I_H__

