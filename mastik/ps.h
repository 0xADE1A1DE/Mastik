#ifndef __PS_H__
#define __PS_H__ 1

#include <mastik/lx.h>
#include <mastik/l3.h>

#define PATTERN_CAPACITY 20
#define PATTERN_TARGET 0xFF
struct prime_pattern {
  uint8_t repeat;
  uint8_t stride;
  // The largest offset from i
  uint8_t width;
  uint8_t length;
  // 0xFF denotes the target, other values denote offset from i
  uint8_t pattern[PATTERN_CAPACITY];
};
typedef struct prime_pattern ppattern_t;
ppattern_t construct_pattern(uint8_t repeat, uint8_t stride, uint8_t* pattern, uint8_t length);
void dump_ppattern(ppattern_t pattern);
void access_pattern(vlist_t evset, int associativity, ppattern_t pattern);

typedef struct ps *ps_t;
ps_t ps_prepare(ppattern_t prime_pattern, l3info_t l3info, mm_t mm);

int ps_monitor(ps_t ps, int line);
int ps_unmonitor(ps_t ps, int line);
void ps_unmonitorall(ps_t ps);
int ps_getmonitoredset(ps_t ps, int *lines, int nlines);

void ps_prime(ps_t ps);
void ps_scope(ps_t ps, uint16_t* results);
// Returns the first line that is accessed, or (-1) if no monitored line
// was accessed before running through all iterations
int ps_prime_and_scope(ps_t ps, int iterations);

void ps_release(ps_t ps);

#endif // __PS_H__
