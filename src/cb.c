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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>

#include <mastik/low.h>
#include <mastik/cb.h>


#define CBT_BANK_COLLISION	0
#define CBT_FALSE_DEPENDENCY	1

#define CB_CODESIZE (4*4096)
#define CB_BUFSIZE (16*4096)

static uint8_t cb_prefix[] = {
  0x49, 0x89, 0xd3,				// mov    %rdx,%r11
};

static uint8_t cb_prefix_bc[] = {
  0x49, 0x89, 0xd3,				// mov    %rdx,%r11
};


static uint8_t cb_loopstart[] = {
  0x0f, 0x01, 0xf9,				// rdtscp
  0x41, 0x89, 0x03,				// mov    %eax,(%r11)
  0x4d, 0x8d, 0x5b, 0x04			// lea    0x4(%r11),%r11
};

static uint8_t cb_suffix[] = {
  0xff, 0xce,					// dec    %esi
  0x0f, 0x85, 0x74, 0xff, 0xff, 0xff,		// jne    <loop start>
  0x0f, 0x01, 0xf9,				// rdtscp
  0xc3						// retq
};

#define MAX_COMMAND_LEN 8
struct cb_command {
  int len;
  uint8_t command[MAX_COMMAND_LEN];
};

static struct cb_command cb_bank_conflict_commands[] = {
  6, { 0x03, 0x87, 0x00, 0x00, 0x00, 0x00},	// add    0x0000(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x00, 0x00, 0x00},	// add    0x0040(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x00, 0x00, 0x00},	// add    0x0080(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x00, 0x00, 0x00},	// add    0x00c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x01, 0x00, 0x00},	// add    0x0100(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x01, 0x00, 0x00},	// add    0x0140(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x01, 0x00, 0x00},	// add    0x0180(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x01, 0x00, 0x00},	// add    0x01c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x02, 0x00, 0x00},	// add    0x0200(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x02, 0x00, 0x00},	// add    0x0240(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x02, 0x00, 0x00},	// add    0x0280(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x02, 0x00, 0x00},	// add    0x02c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x03, 0x00, 0x00},	// add    0x0300(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x03, 0x00, 0x00},	// add    0x0340(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x03, 0x00, 0x00},	// add    0x0380(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x03, 0x00, 0x00},	// add    0x03c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x04, 0x00, 0x00},	// add    0x0400(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x04, 0x00, 0x00},	// add    0x0440(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x04, 0x00, 0x00},	// add    0x0480(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x04, 0x00, 0x00},	// add    0x04c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x05, 0x00, 0x00},	// add    0x0500(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x05, 0x00, 0x00},	// add    0x0540(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x05, 0x00, 0x00},	// add    0x0580(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x05, 0x00, 0x00},	// add    0x05c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x06, 0x00, 0x00},	// add    0x0600(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x06, 0x00, 0x00},	// add    0x0640(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x06, 0x00, 0x00},	// add    0x0680(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x06, 0x00, 0x00},	// add    0x06c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x07, 0x00, 0x00},	// add    0x0700(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x07, 0x00, 0x00},	// add    0x0740(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x07, 0x00, 0x00},	// add    0x0700(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x07, 0x00, 0x00},	// add    0x07c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x08, 0x00, 0x00},	// add    0x0800(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x08, 0x00, 0x00},	// add    0x0840(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x08, 0x00, 0x00},	// add    0x0880(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x08, 0x00, 0x00},	// add    0x08c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x09, 0x00, 0x00},	// add    0x0900(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x09, 0x00, 0x00},	// add    0x0940(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x09, 0x00, 0x00},	// add    0x0980(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x09, 0x00, 0x00},	// add    0x09c0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x0a, 0x00, 0x00},	// add    0x0a00(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x0a, 0x00, 0x00},	// add    0x0a40(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x0a, 0x00, 0x00},	// add    0x0a80(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x0a, 0x00, 0x00},	// add    0x0ac0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x0b, 0x00, 0x00},	// add    0x0b00(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x0b, 0x00, 0x00},	// add    0x0b40(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x0b, 0x00, 0x00},	// add    0x0b80(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x0b, 0x00, 0x00},	// add    0x0bc0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x0c, 0x00, 0x00},	// add    0x0c00(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x0c, 0x00, 0x00},	// add    0x0c40(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x0c, 0x00, 0x00},	// add    0x0c80(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x0c, 0x00, 0x00},	// add    0x0cc0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x0d, 0x00, 0x00},	// add    0x0d00(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x0d, 0x00, 0x00},	// add    0x0d40(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x0d, 0x00, 0x00},	// add    0x0d80(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x0d, 0x00, 0x00},	// add    0x0dc0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x0e, 0x00, 0x00},	// add    0x0e00(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x0e, 0x00, 0x00},	// add    0x0e40(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x0e, 0x00, 0x00},	// add    0x0e80(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x0e, 0x00, 0x00},	// add    0x0ec0(%rdi), %esi

  6, { 0x03, 0x87, 0x00, 0x0f, 0x00, 0x00},	// add    0x0f00(%rdi), %eax
  6, { 0x03, 0x8f, 0x40, 0x0f, 0x00, 0x00},	// add    0x0f40(%rdi), %ecx
  6, { 0x03, 0x97, 0x80, 0x0f, 0x00, 0x00},	// add    0x0f00(%rdi), %edx
  6, { 0x03, 0xb7, 0xc0, 0x0f, 0x00, 0x00},	// add    0x0fc0(%rdi), %esi
};

static struct cb_command cb_false_dependency_commands[] = {
  6, { 0x01, 0x87, 0x00, 0x00, 0x00, 0x00},	// add    %eax, 0x0000(%rdi)
  6, { 0x01, 0x87, 0x00, 0x10, 0x00, 0x00},	// add    %eax, 0x1000(%rdi)
  6, { 0x01, 0x87, 0x00, 0x20, 0x00, 0x00},	// add    %eax, 0x2000(%rdi)
  6, { 0x01, 0x87, 0x00, 0x30, 0x00, 0x00},	// add    %eax, 0x3000(%rdi)
  6, { 0x01, 0x87, 0x00, 0x40, 0x00, 0x00},	// add    %eax, 0x4000(%rdi)
  6, { 0x01, 0x87, 0x00, 0x50, 0x00, 0x00},	// add    %eax, 0x5000(%rdi)
  6, { 0x01, 0x87, 0x00, 0x60, 0x00, 0x00},	// add    %eax, 0x6000(%rdi)
  6, { 0x01, 0x87, 0x00, 0x70, 0x00, 0x00}	// add    %eax, 0x7000(%rdi)

};


typedef uint32_t (*cb_measure_t)(uintptr_t base /* %rdi */, uint32_t count /* rsi */,  uint32_t *results /* rdx */);

#define ARRAYSIZE(x) x, sizeof(x)/sizeof(x[0])
struct cbinfo {
  int maxoffset;
  int offsetmask;
  uint8_t *prefix;
  int prefix_len;
  uint8_t *loopstart;
  int loopstart_len;
  struct cb_command *commands;
  int ncommands;
  uint8_t *suffix;
  int suffix_len;
  int suffix_jmpoffset_start;
  int default_accesses;
};

static struct cbinfo cbinfo[] = {
  {  // CBT_BANK_COLLISION 
    63, 0x03, 
    ARRAYSIZE(cb_prefix_bc), 
    ARRAYSIZE(cb_loopstart),
    ARRAYSIZE(cb_bank_conflict_commands),
    ARRAYSIZE(cb_suffix), 4,
    256
  },
  { // CBT_FALSE_DEPENDENCY
    4095, 0x03,
    ARRAYSIZE(cb_prefix), 
    ARRAYSIZE(cb_loopstart),
    ARRAYSIZE(cb_false_dependency_commands),
    ARRAYSIZE(cb_suffix), 4,
    32
  }
};

struct cb {
  int type;
  int offset;
  int accesses;
  struct cbinfo *cbinfo;
  cb_measure_t code;
  uintptr_t buffer;
};



cb_t cb_prepare(int type) {
  cb_t cb = (cb_t)malloc(sizeof(struct cb));
  if (cb == NULL)
    return NULL;
  cb->type = type;
  cb->offset = 0;
  cb->accesses = 0;
  cb->cbinfo = &cbinfo[type];
  void *code = mmap(NULL, CB_CODESIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANON|MAP_PRIVATE, -1, 0);
  if (code == MAP_FAILED) {
    free(cb);
    return NULL;
  }
  cb->code = (cb_measure_t)code;
  void *buffer = mmap(NULL, CB_BUFSIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
  if (buffer == MAP_FAILED) {
    munmap((void *)cb->code, CB_CODESIZE);
    free(cb);
    return NULL;
  }
  cb->buffer = (uintptr_t)buffer;
  cb_monitor(cb, 0, 0);
  return cb;
}


void cb_release(cb_t cb) {
  if (cb == NULL)
    return;
  if (cb->code)
    munmap((void *)cb->code, CB_CODESIZE);
  if (cb->buffer)
    munmap((void *)cb->buffer, CB_BUFSIZE);
  free(cb);
}

int cb_maxoffset(cb_t cb) {
  assert(cb != NULL);
  return cb->cbinfo->maxoffset;
}


int cb_offsetmask(cb_t cb) {
  assert(cb != NULL);
  return cb->cbinfo->offsetmask;
}

int cb_monitor(cb_t cb, int offset, int accesses) {
  assert(cb != NULL);
  struct cbinfo *cbinfo = cb->cbinfo;

  int rv = 0;
  cb->offset = offset;
  if (accesses == 0)
    accesses = cbinfo->default_accesses;
  cb->accesses = accesses;
  uint8_t *p = (uint8_t *)cb->code;
  uint8_t *start = p;

  memcpy(p, cbinfo->prefix, cbinfo->prefix_len);
  p += cbinfo->prefix_len;
  memcpy(p, cbinfo->loopstart, cbinfo->loopstart_len);
  p += cbinfo->loopstart_len;
  for (int i = 0; i < accesses; i++) {
    struct cb_command *command = &cbinfo->commands[i % cbinfo->ncommands];
    if (p - start + command->len + cbinfo->suffix_len > CB_CODESIZE) {
      cb->accesses = i;
      rv = -1;
      break;
    }
    memcpy(p, command->command, command->len);
    p += command->len;
  }
  memcpy(p, cbinfo->suffix, cbinfo->suffix_len);

  // Patch jump
  start += cbinfo->prefix_len;				// Start is now the start of the loop
  p += cbinfo->suffix_jmpoffset_start; 			// p points to the start of the jump offset
  int32_t jmpoffset = start - p - sizeof(jmpoffset); 	// offset is the distance from the jump end to the loop start
  memcpy(p, &jmpoffset, sizeof(jmpoffset));


  return rv;
}




int cb_getmonitored_offset(cb_t cb) {
  assert(cb != NULL);
  return cb->offset;
}

int cb_getmonitored_accesses(cb_t cb) {
  assert(cb != NULL);
  return cb->accesses;
}

void cb_probe(cb_t cb, uint32_t *results) {
  cb_repeatedprobe(cb, 1, results);
}

void cb_bprobe(cb_t cb, uint32_t *results) {
  cb_repeatedprobe(cb, 1, results);
}

int cb_repeatedprobe(cb_t cb, int nrecords, uint32_t *results) {
  uint32_t last  = cb_repeatedproberaw(cb, nrecords, results);
  for (int i = 0; i < nrecords - 1; i++) 
    results[i] = results[i+1] - results[i];
  results[nrecords-1] = last - results[nrecords-1];
  return 0;
}


uint32_t cb_repeatedproberaw(cb_t cb, int nrecords, uint32_t *results) {
  return  (*cb->code)(cb->buffer + cb->offset, nrecords, results);
}


