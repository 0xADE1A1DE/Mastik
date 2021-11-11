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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mastik/fr.h>
#include <mastik/pda.h>
#include <mastik/util.h>
#include <mastik/symbol.h>
#include <time.h>
#include <sys/utsname.h>

#define SAMPLES 100000
#define SLOT	10000
#define IDLE    500
#define THRESHOLD 100

#define MAX_MONITORED 100
#define MAX_EVICTED 100
#define MAX_PDA_TARGETS 10

#define MAX_PDAS 8

void usage(char *p) {
  fprintf(stderr, "Usage: %s [-s <slotlen>] [-c <maxsamplecount>] [-h <threshold>] [-i <idlecount>]\n"
      		  "                [-p <pdacount>] [-H] [-f <file>] [-S <debugfile>] [-a cpu]\n"
		  "                [-F <outputFileNameFormat>] [-r <runs] [-l <minlen>]\n"
		  "                [-m <monitoraddress>] [-e <evictaddress>] [-t <pdatarget>] ...\n", p);
  exit(1);
}

struct map_entry {
  char *file;
  char *debugfile;
  char *adrsspec;
  uint64_t offset;
  void *map_address;
};

struct config {
  char *progname;
  int samples;
  int slot;
  int threshold;
  int idle;
  int pdacount;
  struct map_entry monitored[MAX_MONITORED];
  int nmonitored;
  struct map_entry evicted[MAX_EVICTED];
  int nevicted;
  struct map_entry pda_targets[MAX_PDA_TARGETS];
  int npdatargets;
  int printheader;
  char *outfilenameformat;
  int runs;
  int minlen;
  int affinity;
};


void fill_map_entry(struct map_entry *e) {
  if (e->file == NULL) {
    fprintf(stderr, "No filename\n");
    exit(1);
  }
  e->offset = sym_getsymboloffset(e->debugfile, e->adrsspec);
  if (e->offset == ~0ULL) {
    fprintf(stderr, "Cannot find %s in %s (%s)\n", e->adrsspec, e->file, e->debugfile);
    exit(1);
  }

  e->map_address = map_offset(e->file, e->offset);
  if (e->map_address == NULL) {
    perror(e->file);
    exit(1);
  }
}



static void printmapentries(FILE * f, struct map_entry *entries, int count, char *name) {
  for (int i = 0; i < count; i++) {
    fprintf(f, "#   %s%d=%s %s 0x%" PRIx64 "\n", name, i, entries[i].file, entries[i].adrsspec, entries[i].offset);
  }
}

static void printuname(FILE *f) {
  struct utsname name;
  uname(&name);
  fprintf(f, "# sysname=%s\n", name.sysname);
  fprintf(f, "# nodename=%s\n", name.nodename);
  fprintf(f, "# release=%s\n", name.release);
  fprintf(f, "# version=%s\n", name.version);
  fprintf(f, "# machine=%s\n", name.machine);
}


void readargs(struct config *c, int ac, char **av) {
  char *file = NULL;
  char *debugfile = NULL;
  int ch;

  c->samples = SAMPLES;
  c->slot = SLOT;
  c->threshold = THRESHOLD;
  c->idle = IDLE;
  c->pdacount = 0;
  c->npdatargets = 0;
  c->nmonitored = 0;
  c->nevicted = 0;
  c->progname = av[0];
  c->printheader = 0;
  c->outfilenameformat = NULL;
  c->runs = 0;
  c->minlen = 0;
  c->affinity = -1;

  while ((ch = getopt(ac, av, "Ha:f:S:s:c:h:i:p:t:m:e:r:F:l:")) != -1) {
    switch (ch) {
      case 'H': 
	c->printheader = 1;
	break;
      case 's':
	c->slot = atoi(optarg);
	break;
      case 'c':
	c->samples = atoi(optarg);
	break;
      case 'a':
	c->affinity = atoi(optarg);
	break;
      case 'h':
	c->threshold = atoi(optarg);
	break;
      case 'i':
	c->idle = atoi(optarg);
	break;
      case 'p':
	c->pdacount = atoi(optarg);
	break;
      case 'f':
	file = optarg;
	debugfile = optarg;
	break;
      case 'S':
	debugfile = optarg;
	break;
      case 't':
	if (c->npdatargets >= MAX_PDA_TARGETS) {
	  fprintf(stderr, "Too many pda targets (Max %d)\n", MAX_PDA_TARGETS);
	  exit(1);
	}
	c->pda_targets[c->npdatargets].file = file;
	c->pda_targets[c->npdatargets].debugfile = debugfile;
	c->pda_targets[c->npdatargets].adrsspec = optarg;
       	fill_map_entry(&c->pda_targets[c->npdatargets]);
	c->npdatargets++;
	break;
      case 'm':
	if (c->nmonitored >= MAX_MONITORED) {
	  fprintf(stderr, "Too many monitored locations(Max %d)\n", MAX_MONITORED);
	  exit(1);
	}
	c->monitored[c->nmonitored].file = file;
	c->monitored[c->nmonitored].debugfile = debugfile;
	c->monitored[c->nmonitored].adrsspec = optarg;
       	fill_map_entry(&c->monitored[c->nmonitored]);
	c->nmonitored++;
	break;
      case 'e':
	if (c->nevicted >= MAX_EVICTED) {
	  fprintf(stderr, "Too many evicted locations(Max %d)\n", MAX_EVICTED);
	  exit(1);
	}
	c->evicted[c->nevicted].file = file;
	c->evicted[c->nevicted].debugfile = debugfile;
	c->evicted[c->nevicted].adrsspec = optarg;
       	fill_map_entry(&c->evicted[c->nevicted]);
	c->nevicted++;
	break;
      case 'F':
        c->outfilenameformat = optarg;
        break;
      case 'r':
        c->runs = atoi(optarg);
        break;
      case 'l':
        c->minlen = atoi(optarg);
        break;
      default: usage(av[0]);
    }
  }

  if (c->nmonitored == 0) 
    usage(av[0]);

  int maxcpus = ncpus();
  if (maxcpus < 0)
    maxcpus = MAX_PDAS;

  if (c->pdacount < 0)
    c->pdacount += maxcpus;

  if (c->pdacount > maxcpus) {
    fprintf(stderr, "Too many performance degradation attack threads. (Max %d)\n", maxcpus);
    exit(1);
  }

  if (c->runs == 0) {
    c->runs = 1;
    c->outfilenameformat = NULL;
  }

  if (c->minlen <= 0)
    c->minlen = c->idle * 3;
  if (c->minlen > c->samples)
    c->minlen = c->samples;
}


void printheader(FILE *f, struct config *c, int lines, int run, fr_t fr) {
  if (!c->printheader)
    return;
  time_t now = time(NULL);
  fprintf(f, "# %s starting at %.24s\n", c->progname, ctime(&now));
  fprintf(f, "################# CONFIG #################\n");
  fprintf(f, "# slot=%d\n", c->slot);
  fprintf(f, "# samples=%d\n", c->samples);
  fprintf(f, "# threshold=%d\n", c->threshold);
  fprintf(f, "# idle=%d\n", c->idle);
  fprintf(f, "# minlen=%d\n", c->minlen);
  fprintf(f, "# runs=%d\n", c->runs);
  fprintf(f, "# pdathreads=%d\n", c->pdacount);
  fprintf(f, "# nmonitored=%d\n", c->nmonitored);
  printmapentries(f, c->monitored, c->nmonitored, "monitor");
  fprintf(f, "# nevicted=%d\n", c->nevicted);
  printmapentries(f, c->evicted, c->nevicted, "evict");
  fprintf(f, "# npdatargets=%d\n", c->npdatargets);
  printmapentries(f, c->pda_targets, c->npdatargets, "target");
  fprintf(f, "# affinity=");
  if (c->affinity == -1)
    fprintf(f, "all\n");
  else
    fprintf(f, "%d\n", c->affinity);
  fprintf(f, "############## SYSTEM INFO ###############\n");
  fprintf(f, "# mastik_version=%s\n", mastik_version());
  printuname(f);
  fprintf(f, "################## DATA ##################\n");
  fprintf(f, "# run=%d\n", run);
  fprintf(f, "# datapoints=%d\n", lines);
}

void printdata(char *fn, struct config *c, int lines, uint16_t *res, int run, fr_t fr) {

  FILE *f = stdout;
  if (fn)
    f = fopen(fn, "w");
  printheader(f, c, lines, run, fr);
  for (int i = 0; i < lines; i++) {
    for (int j = 0; j < c->nmonitored; j++)
      fprintf(f, "%d ", res[i * c->nmonitored + j]);
    putc('\n', f);
  }
  if (fn)
    fclose(f);
}

int main(int ac, char **av) {
  struct config c;
  pda_t *pdas = NULL;
  fr_t fr = fr_prepare();

  readargs(&c, ac, av);

  for (int i = 0; i < c.nmonitored; i++)
    fr_monitor(fr, c.monitored[i].map_address);

  for (int i = 0; i < c.nevicted; i++)
    fr_evict(fr, c.evicted[i].map_address);



  uint16_t *res = malloc(c.samples * c.nmonitored * sizeof(uint16_t));
  for (int i = 0; i < c.samples * c.nmonitored ; i+= 4096/sizeof(uint16_t))
    res[i] = 1;
  fr_probe(fr, res);

  if (c.pdacount > 0) {
    pdas = calloc(c.pdacount, sizeof(pda_t));
    for (int i = 0; i < c.pdacount; i++) {
      pdas[i] = pda_prepare();
      for (int j = 0; j < c.npdatargets; j++)
	pda_target(pdas[i], c.pda_targets[j].map_address);
      pda_activate(pdas[i]);
    }
  }


  if (c.affinity != -1)
    setaffinity(c.affinity);
    for (int r = 0; r < c.runs; r++) {
      int l = fr_trace(fr, c.samples, res, c.slot, c.threshold, c.idle);

    char *outfile=NULL;
    if (c.outfilenameformat != NULL)
      asprintf(&outfile, c.outfilenameformat, r);
    printdata(outfile, &c, l, res, r, fr);
    if (c.outfilenameformat != NULL)
      free(outfile);
  }

  if (c.pdacount > 0) {
    for (int i = 0; i < c.pdacount; i++)
      pda_release(pdas[i]);
  }

  free(res);
  fr_release(fr);
}

