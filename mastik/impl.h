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

#ifndef _IMPL_H__
#define _IMPL_H__ 1

#include <stdint.h>
#include <unistd.h>

#include <mastik/lx.h>

#define LNEXT(t) (*(void **)(t))
#define OFFSET(p, o) ((void *)((uintptr_t)(p) + (o)))
#define NEXTPTR(p) (OFFSET((p), sizeof(void *)))

#define IS_MONITORED(monitored, setno) ((monitored)[(setno)>>5] & (1 << ((setno)&0x1f)))
#define SET_MONITORED(monitored, setno) ((monitored)[(setno)>>5] |= (1 << ((setno)&0x1f)))
#define UNSET_MONITORED(monitored, setno) ((monitored)[(setno)>>5] &= ~(1 << ((setno)&0x1f)))

#define LXFLAG_NOHUGEPAGES	0x01
#define LXFLAG_USEPTE		0x02 
#define LXFLAG_NOPROBE		0x04
#define LXFLAG_QUADRATICMAP	0x08	// Defaults to this if huge pages is specified
#define LXFLAG_LINEARMAP	0x10	// Defaults to this if small pages is specified

#define LX_CACHELINE 0x40

int probetime(void *pp);
int bprobetime(void *pp);

void lx_probe(lxpp_t lx, uint16_t *results);
void lx_bprobe(lxpp_t lx, uint16_t *results);
void lx_probecount(lxpp_t lx, uint16_t *results);
void lx_bprobecount(lxpp_t lx, uint16_t *results);

int lx_repeatedprobe(lxpp_t lx, int nrecords, uint16_t *results, int slot);
int lx_repeatedprobecount(lxpp_t lx, int nrecords, uint16_t *results, int slot);

void lx_randomise(lxpp_t lx);
int lx_getmonitoredset(lxpp_t lx, int *lines, int nlines); 

int lx_monitor(lxpp_t lx, int line);
void lx_monitorall(lxpp_t lx);

int lx_unmonitor(lxpp_t lx, int line);
void lx_unmonitorall(lxpp_t lx);

int lx_getlxinfo(lxpp_t lx, lxinfo_t lxinfo);

// Type of callback for setup and execute for synchronized PP or ET
typedef void (*lx_sync_cb)(lxpp_t l1, int recnum, void *data);

// Synchronized Prime+Probe
int lx_syncpp(lxpp_t lx, int nrecords, uint16_t *results, lx_sync_cb setup, lx_sync_cb exec, void *data);

void lx_dummy_cb(lxpp_t l1, int recnum, void *data);

void lx_release(lxpp_t lx);

#endif // __IMPL_H__
