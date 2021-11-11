/* 
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this file,
You can obtain one at https://mozilla.org/MPL/2.0/.

Copyright (c) 2018, Stephan van Schaik 


Imported from https://github.com/vusec/xlate/blob/master/include/xlate/x86-64/tsx.h
*/

#pragma once

#define XBEGIN_INIT     (~0u)
#define XABORT_EXPLICIT (1 << 0)
#define XABORT_RETRY    (1 << 1)
#define XABORT_CONFLICT (1 << 2)
#define XABORT_CAPACITY (1 << 3)
#define XABORT_DEBUG    (1 << 4)
#define XABORT_NESTED   (1 << 5)
#define XABORT_CODE(x)  (((x) >> 24) & 0xff)

static inline unsigned xbegin(void)
{
    uint32_t ret = XBEGIN_INIT;

    asm volatile(
    "xbegin 1f\n"
    "1:\n"
    : "+a" (ret)
    :: "memory");

    return ret;
}

static inline void xend(void)
{
    asm volatile(
    "xend\n"
    ::: "memory");
}
