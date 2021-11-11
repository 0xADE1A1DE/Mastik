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
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "aes.h"

#include <mastik/util.h>
#include <mastik/synctrace.h>

#define AESSIZE 16

#define NSAMPLES 100000

typedef uint8_t aes_t[AESSIZE];

void usage(char *p) {
  fprintf(stderr, "Usage: %s -aAhH [-s <samples>]\n", p);
  exit(1);
}

void tobinary(char *data, aes_t aes) {
  assert(strlen(data)==AESSIZE*2);
  unsigned int x;
  for (int i = 0; i < AESSIZE; i++) {
    sscanf(data+i*2, "%2x", &x);
    aes[i] = x;
  }
}

void grayspace(int64_t intensity, int64_t min, int64_t max, char c) {
  printf("\e[48;5;%ld;31;1m%c\e[0m", 232+((intensity - min) *24)/(max - min), c);
}

int64_t display(int counts[256], int64_t data[256][1024], int guess, int offset) {
  int64_t min = INT64_MAX;
  int64_t max = INT64_MIN;
  for (int i = 0; i < 256; i++) 
    if (counts[i]) 
      for (int j = 0; j < L1_SETS; j++){
	if (min > data[i][j])
	  min = data[i][j];
	if (max < data[i][j])
	  max = data[i][j];
      }

  if (max == min)
    max++;
  for (int i = 0; i < 256; i++) {
    if (counts[i]) {
      printf("%02X. ", i);
      int set = (((i >> 4) ^ guess) + offset) % L1_SETS;
      if (offset < 0)
	set = -1;
      for (int j = 0; j < L1_SETS; j++) {
	grayspace(data[i][j], min, max, set == j ? '#' : ' ');
      }
      printf("\n");
    }
  }
}

void analyse(int64_t data[256][1024], int *key, int *offset) {
  int64_t max = INT64_MIN;

  for (int guess = 0; guess < 16; guess++) {
    for (int off = 0; off < L1_SETS; off++) {
      int64_t sum = 0LL;
      for (int pt = 0; pt < 16; pt++) {
	int set = (off + (pt ^ guess)) % L1_SETS;
	sum += data[pt << 4][set];
      }
      if (sum > max) {
	max = sum;
	*key = guess;
	*offset = off;
      }
    }
  }
}


void crypto(uint8_t *input, uint8_t *output, void *data) {
  AES_KEY *aeskey = (AES_KEY *)data;
  AES_encrypt(input, output, aeskey);
}




int main(int ac, char **av) {
  int samples = NSAMPLES;
  int round = 1;
  int analysis = 1;
  int heatmap = 1;
  int byte = 0;
  int ch;
  while ((ch = getopt(ac, av, "b:s:12aAhH")) != -1) {
    switch (ch){
      case 's':
	samples = atoi(optarg);
	break;
      case '1':
	round = 1;
	break;
      case '2':
	round = 2;
	break;
      case 'a':
	analysis = 1;
	break;
      case 'A':
	analysis = 0;
	break;
      case 'h':
	heatmap = 1;
	break;
      case 'H':
	heatmap = 0;
	break;
      case 'b':
	byte = atoi(optarg);
	break;
      default:
	usage(av[0]);
    }
  }

  if (round == 2)
    analysis = 0;
  if (!analysis && !heatmap) {
    fprintf(stderr, "No output format specified\n");
    usage(av[0]);
  }
  if (samples <= 0) {
    fprintf(stderr, "Negative number of samples\n");
    usage(av[0]);
  }
  if (byte < 0 || byte >= AESSIZE) {
    fprintf(stderr, "Target byte must be in the range 0--15\n");
    usage(av[0]);
  }


  aes_t key;
  char * keystr = "2b7e151628aed2a6abf7158809cf4f3c";
  tobinary(keystr, key);
  AES_KEY aeskey;
  private_AES_set_encrypt_key(key, 128, &aeskey);

  delayloop(1000000000);

  if (round == 1) {
    st_clusters_t clusters = syncPrimeProbe(samples,
				  AESSIZE,
				  1,
				  NULL,
				  NULL,
				  crypto,
				  &aeskey,
				  0xf0,
          1);
    for (int i = 0; i < 16; i++) {
      int key, offset;
      printf("Key byte %2d", i);
      if (analysis) {
	analyse(clusters[i].avg, &key, &offset);
	printf(" Guess:%1x-\n", key);
      } else {
	offset = -L1_SETS;
	printf("\n");
      }
      if (heatmap) {
	display(clusters[i].count, clusters[i].avg, key, offset);
	printf("\n");
      }
    }
    free(clusters);
  } else if (round == 2) {
    aes_t fixmask, fixdata;
    tobinary("00000000000000000000000000000000", fixmask);
    tobinary("00000000000000000000000000000000", fixdata);

    int col0 = (byte + 4 - (byte >> 2) & 3);
    for (int row = 0; row < 4; row++) {
      int b = row * 4 + ((col0 + row) & 3);
      if (b != byte) 
	fixmask[b] = 0xff;
    }

    st_clusters_t clusters = syncPrimeProbe(samples,
				  AESSIZE,
				  1,
				  fixmask,
				  fixdata,
				  crypto,
				  &aeskey,
				  0xff,
          1);

    display(clusters[byte].count, clusters[byte].avg, 0, -L1_SETS);
    free(clusters);
  }
}
