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
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <mastik/low.h>
#include <mastik/pda.h>

#include "vlist.h"

struct pda { 
  vlist_t vl;
  pid_t child;
  int modified;
  int active;
};



pda_t pda_prepare() {
  pda_t rv = malloc(sizeof(struct pda));
  rv->vl = vl_new();
  rv->child = -1;
  rv->modified = 0;
  rv->active = 0;
  return rv;
}

void pda_release(pda_t pda) {
  pda_deactivate(pda);
  vl_free(pda->vl);
  pda->vl = NULL;
  free(pda);
}


int pda_target(pda_t pda, void *adrs) {
  assert(pda != NULL);
  assert(adrs != NULL);
  vl_push(pda->vl, adrs);
  pda->modified = 1;
  return 1;
}


int pda_untarget(pda_t pda, void *adrs) {
  assert(pda != NULL);
  assert(adrs != NULL);
  int i;
  int count = 0;
  while ((i = vl_find(pda->vl, adrs)) >= 0) {
    vl_del(pda->vl, i);
    count++;
  }
  pda->modified = count != 0;
  return count;
}


int pda_gettargetedset(pda_t pda, void **adrss, int nlines) {
  assert(pda != NULL);

  if (adrss != NULL) {
    int l = vl_len(pda->vl);
    if (l > nlines)
      l = nlines;
    for (int i = 0; i < l; i++)
      adrss[i] = vl_get(pda->vl, i);
  }
  return vl_len(pda->vl);
}

void pda_randomise(pda_t pda) {
  assert(pda != NULL);
  assert(0);

  pda->modified = 1;
}



static void pda_flush(pda_t pda) {
  void *p1,*p2, *p3, *p4;
  vlist_t vl = pda->vl;
  int len = vl_len(vl);

  switch (len) {
    case 0: return;
    case 1:
	    p1 = vl_get(pda->vl, 0);
	    for (;;) {
	      clflush(p1);
	    }
    case 2:
	    p1 = vl_get(pda->vl, 0);
	    p2 = vl_get(pda->vl, 1);
	    for (;;) {
	      clflush(p1);
	      clflush(p2);
	    }
    case 3:
	    p1 = vl_get(pda->vl, 0);
	    p2 = vl_get(pda->vl, 1);
	    p3 = vl_get(pda->vl, 1);
	    for (;;) {
	      clflush(p1);
	      clflush(p2);
	      clflush(p3);
	    }
    case 4:
	    p1 = vl_get(pda->vl, 0);
	    p2 = vl_get(pda->vl, 1);
	    p3 = vl_get(pda->vl, 1);
	    p4 = vl_get(pda->vl, 1);
	    for (;;) {
	      clflush(p1);
	      clflush(p2);
	      clflush(p3);
	      clflush(p4);
	    }
    default:
	    vl = pda->vl;
	    for (;;) {
	      for (int i = 0; i < len; i++)
		clflush(vl_get(vl, i));
	    }
    }
}

static void setautodeath() {
#ifdef HAVE_SYS_PRCTL_H
  prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif
}

void pda_activate(pda_t pda) {
  if (pda->active) {
    if (!pda->modified)
      return;
    pda_deactivate(pda);
  }

  if (vl_len(pda->vl) == 0)
    return;

  pda->child = fork();
  switch (pda->child) {
    case -1:
      return;
    case 0:
      setautodeath();
      pda_flush(pda);
      // unreached
    default:
      pda->active = 1;
      pda->modified = 0;
      return;
  }
}

void pda_deactivate(pda_t pda) {
  if (!pda->active)
    return;
  if (pda->child > 0) {
    kill(pda->child, SIGKILL);
    wait4(pda->child, NULL, 0, NULL);
  }
  pda->child = -1;
  pda->active = 0;
  return;
}

int pda_isactive(pda_t pda) {
  return pda->active;
}


