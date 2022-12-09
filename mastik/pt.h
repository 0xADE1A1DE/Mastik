#ifndef __PRIME_TIME_H__
#define __PRIME_TIME_H__ 1

#include <mastik/ps.h>

typedef struct prime_time* pt_t;

#define PT_RESULTS_COUNT 100
struct pt_results {
    ppattern_t patterns[PT_RESULTS_COUNT];
    float evcrs[PT_RESULTS_COUNT];
    int cycles[PT_RESULTS_COUNT];
};
typedef struct pt_results* pt_results_t;

pt_t pt_prepare(l3info_t l3info, mm_t mm);
void pt_release(pt_t pt);

// Results have to be free()-ed
pt_results_t generate_prime_patterns(pt_t patterns);

#endif // __PRIME_TIME_H__
