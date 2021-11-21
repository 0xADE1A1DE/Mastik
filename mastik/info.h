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

#ifndef INFO_H__
#define INFO_H__ 1

#include <mastik/low.h>

typedef void (*lxprogressNotification_t)(int count, int est, void *data);

struct lxinfo {
  int associativity;
  int sets;
  int bufsize;
  int flags;
  void *progressNotificationData;
  
  int slices;
  lxprogressNotification_t progressNotification;
  union cpuid cpuidInfo;
};
typedef struct lxinfo *lxinfo_t;

#endif // __INFO_H__
