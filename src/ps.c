#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>

#include <mastik/ps.h>

#include "vlist.h"
#include "mm-impl.h"
#include "mastik/impl.h"

ppattern_t construct_pattern(uint8_t repeat, uint8_t stride, uint8_t* pattern, uint8_t length) {
  assert(length <= PATTERN_CAPACITY);
  
  ppattern_t pat;
  pat.length = length;
  pat.repeat = repeat;
  pat.stride = stride;
  
  pat.width = 0;
  for(int i = 0; i < length; i++) {
    pat.pattern[i] = pattern[i];

    if(pattern[i] != PATTERN_TARGET && pattern[i] > pat.width) {
      pat.width = pattern[i];
    }
  }

  return pat;
}

void dump_ppattern(ppattern_t pat) {
  printf("R%d_S%d_P", pat.repeat, pat.stride);
  for(int i = 0; i < pat.length; i++) {
    if(pat.pattern[i] == PATTERN_TARGET) {
      printf("S");
    } else {
      printf("%d", pat.pattern[i]);
    }
  }
}

void access_pattern(vlist_t evset, int assoc, ppattern_t pattern) {
  for(int r = 0; r < pattern.repeat; r++) {
    for(int i = 0; i < assoc - pattern.width; i++) {
      for(int j = 0; j < pattern.length; j++) {
        int a = pattern.pattern[j];
        if(a == PATTERN_TARGET) {
          memaccess(vl_get(evset, 0));
        } else {
          assert(a <= pattern.width);
          memaccess(vl_get(evset, i + a));
        }
      }
    }
  }
}


struct ps { 
  ppattern_t prime_pattern;
  
  vlist_t *monitored_evsets;
  int nmonitored;
  int *monitoredset;
  uint32_t *monitoredbitmap;

  size_t totalsets;
  int ngroups;
  int groupsize;
  vlist_t *groups;
  
  struct l3info l3info;
  
  mm_t mm; 
  uint8_t internalmm;
};

ps_t ps_prepare(ppattern_t prime_pattern, l3info_t l3info, mm_t mm) {
  // Setup
  ps_t ps = (ps_t)malloc(sizeof(struct ps));
  bzero(ps, sizeof(struct ps));

  ps->prime_pattern = prime_pattern;

  if (l3info != NULL)
    bcopy(l3info, &ps->l3info, sizeof(struct l3info));
  fillL3Info(&ps->l3info);
  
  ps->mm = mm;
  if (ps->mm == NULL) {
    ps->mm = mm_prepare(NULL, NULL, (lxinfo_t)l3info);
    ps->internalmm = 1;
  }
  
  if (!mm_initialisel3(ps->mm)) 
    return NULL;
  
  ps->ngroups = ps->mm->l3ngroups;
  ps->groupsize = ps->mm->l3groupsize;
  
  // Allocate monitored set info
  ps->totalsets = ps->ngroups * ps->groupsize;
  ps->monitoredbitmap = (uint32_t *)calloc((ps->totalsets/32) + 1, sizeof(uint32_t));
  ps->monitoredset = (int *)malloc(ps->totalsets * sizeof(int));
  ps->monitored_evsets = (vlist_t *)malloc(ps->totalsets * sizeof(vlist_t));
  ps->nmonitored = 0;

  return ps;
}


int ps_monitor(ps_t ps, int line) {
  if(line < 0 || line >= ps->totalsets)
    return 0;
  if (IS_MONITORED(ps->monitoredbitmap, line))
    return 0;
  
  int associativity = ps->l3info.associativity;
  
  vlist_t vl = vl_new();
  _mm_requestlines(ps->mm, L3, line, associativity, vl);

  ps->monitoredset[ps->nmonitored] = line;
  ps->monitored_evsets[ps->nmonitored] = vl;
  SET_MONITORED(ps->monitoredbitmap, line);
  ps->nmonitored++;
  return 1;
}

int ps_unmonitor(ps_t ps, int line) {
  if (line < 0 || line >= ps->totalsets) 
    return 0;
  if (!IS_MONITORED(ps->monitoredbitmap, line))
    return 0;
  UNSET_MONITORED(ps->monitoredbitmap, line);
  for (int i = 0; i < ps->nmonitored; i++)
    if (ps->monitoredset[i] == line) {
      --ps->nmonitored;
      ps->monitoredset[i] = ps->monitoredset[ps->nmonitored];
      
      _mm_returnlines(ps->mm, ps->monitored_evsets[i]);
      vl_free(ps->monitored_evsets[i]);
      
      ps->monitored_evsets[i] = ps->monitored_evsets[ps->nmonitored];
      
      break;
    }
  return 1;
}

void ps_unmonitorall(ps_t ps) {
  for (int i = 0; i < ps->totalsets / 32; i++)
    ps->monitoredbitmap[i] = 0;
  for (int i = 0; i < ps->nmonitored; i++) {
    _mm_returnlines(ps->mm, ps->monitored_evsets[i]);
    vl_free(ps->monitored_evsets[i]);
  }
  ps->nmonitored = 0;
}

int ps_getmonitoredset(ps_t ps, int *lines, int nlines) {
  if (lines == NULL || nlines == 0)
    return ps->nmonitored;
  if (nlines > ps->nmonitored)
    nlines = ps->nmonitored;
  bcopy(ps->monitoredset, lines, nlines * sizeof(int));
  return ps->nmonitored;
}

void ps_prime(ps_t ps) {
  for (int i = 0; i < ps->nmonitored; i++) {
    access_pattern(ps->monitored_evsets[i], ps->l3info.associativity, ps->prime_pattern);
  }
}

void ps_scope(ps_t ps, uint16_t* results) {
  for (int i = 0; i < ps->nmonitored; i++) {
    int t = memaccesstime(vl_get(ps->monitored_evsets[i], 0));
    results[i] = t > UINT16_MAX ? UINT16_MAX : t;
  }
}

int ps_prime_and_scope(ps_t ps, int iterations) {
  ps_prime(ps);

  for(int i = 0; i < iterations; i++) {
    for (int i = 0; i < ps->nmonitored; i++) {
      int t = memaccesstime(vl_get(ps->monitored_evsets[i], 0));
      if(t > L3_THRESHOLD) {
        return ps->monitoredset[i];
      }
    }
  }

  return -1;
}

void ps_release(ps_t ps) {
  free(ps->monitoredbitmap);
  free(ps->monitoredset);
  for(int i = 0; i < ps->nmonitored; i++) {
    _mm_returnlines(ps->mm, ps->monitored_evsets[i]);
    vl_free(ps->monitored_evsets[i]);
  }
  free(ps->monitored_evsets);
  if (ps->internalmm)
    mm_release(ps->mm);
  bzero(ps, sizeof(struct ps));
  free(ps);
}
