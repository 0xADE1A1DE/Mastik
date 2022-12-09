#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include <mastik/pt.h>
#include <mastik/low.h>
#include "vlist.h"
#include "mm-impl.h"
#include "mastik/impl.h"

#define DEBUG 1

#define PATTERN_LIST_CAPACITY 50000
struct prime_time {
  ppattern_t* patterns;
  int pattern_count;

  vlist_t evset;

  struct l3info l3info;
  mm_t mm; 
  uint8_t internalmm;
};

pt_t pt_prepare(l3info_t l3info, mm_t mm) {
  // Setup
  pt_t pt = (pt_t) malloc(sizeof(struct prime_time));
  bzero(pt, sizeof(struct prime_time));

  pt->patterns = (ppattern_t *)malloc(PATTERN_LIST_CAPACITY * sizeof(ppattern_t));
  pt->pattern_count = 0;

  if (l3info != NULL)
    bcopy(l3info, &pt->l3info, sizeof(struct l3info));
  fillL3Info(&pt->l3info);
  
  pt->mm = mm;
  if (pt->mm == NULL) {
    pt->mm = mm_prepare(NULL, NULL, (lxinfo_t)l3info);
    pt->internalmm = 1;
  }
  
  if (!mm_initialisel3(pt->mm)) 
    return NULL;

  pt->evset = vl_new();
  _mm_requestlines(pt->mm, L3, 31, pt->l3info.associativity + 1, pt->evset);

  return pt;
}

void pt_release(pt_t pt) {
  free(pt->patterns);
  _mm_returnlines(pt->mm, pt->evset);
  vl_free(pt->evset);
  if (pt->internalmm)
    mm_release(pt->mm);
  bzero(pt, sizeof(struct prime_time));
  free(pt);
}

void patterns_push(pt_t patterns, ppattern_t pattern) {
  assert(patterns->pattern_count < PATTERN_LIST_CAPACITY);
  patterns->patterns[patterns->pattern_count++] = pattern;
}

void test_pattern(pt_t patterns, ppattern_t pattern, int test_count, float* evcr_result) {
  int evc_count = 0;
  vlist_t evset = patterns->evset;
  int associativity = patterns->l3info.associativity;

  for(int t = 0; t < test_count; t++) {
    access_pattern(evset, associativity, pattern);

    uint32_t t_l1 = memaccesstime(vl_get(evset, 0));

    memaccess(vl_get(evset, associativity));
    uint32_t t_evict = memaccesstime(vl_get(evset, 0));
    
    if(t_l1 < L3_THRESHOLD && t_evict > L3_THRESHOLD) 
      evc_count++;
  }
  *evcr_result = (float)evc_count/test_count;
}

void test_patterns(pt_t patterns, int test_count, float* evcr_results) {
  for(int i = 0; i < patterns->pattern_count; i++) {
    test_pattern(patterns, patterns->patterns[i], test_count, evcr_results + i);
#ifdef DEBUG
    if(i%500 == 0 || (i%50 == 0 && test_count > 10000)) {
      printf("test_patterns %d/%d\n", i, patterns->pattern_count);
    }
#endif // DEBUG
  }
}

void time_pattern(pt_t patterns, ppattern_t pattern, int test_count, int* time_result) {
  uint64_t t_start = rdtscp64();
  for(int t = 0; t < test_count; t++) {
    access_pattern(patterns->evset, patterns->l3info.associativity, pattern);
  }
  uint64_t t_end = rdtscp64();
  assert((uint64_t)(int)(t_end - t_start) == t_end - t_start);
  *time_result = (int)(t_end - t_start) / test_count;
}

void time_patterns(pt_t patterns, int test_count, int* time_results) {
  for(int i = 0; i < patterns->pattern_count; i++) {
    time_pattern(patterns, patterns->patterns[i], test_count, time_results + i);
  }
}

void filter_patterns(pt_t patterns, int* indices, int count) {  
  ppattern_t* filtered = (ppattern_t*) malloc(sizeof(ppattern_t) * count);
  for(int i = 0; i < count; i++) {
    filtered[i] = patterns->patterns[indices[i]];
  }
  for(int i = 0; i < count; i++) {
    patterns->patterns[i] = filtered[i];
  }
  free(filtered);
  patterns->pattern_count = count;
}

float* evcr_results;
int evcr_compare(const void* a, const void* b) {
  int a_idx = *(int*)a;
  int b_idx = *(int*)b;
  float a_evcr = evcr_results[a_idx];
  float b_evcr = evcr_results[b_idx];
  if(a_evcr > b_evcr) return -1;
  if(a_evcr < b_evcr) return 1;
  return 0;
}
int* time_results;
int time_compare(const void* a, const void* b) {
  int a_idx = *(int*)a;
  int b_idx = *(int*)b;
  return time_results[a_idx] - time_results[b_idx];
}

void filter_evcr(pt_t patterns, int test_count, int filter_count) {
  evcr_results = malloc(sizeof(float)*patterns->pattern_count);
  test_patterns(patterns, test_count, evcr_results);

  int* indices = malloc(sizeof(int)*patterns->pattern_count);
  for(int i = 0; i < patterns->pattern_count; i++) {
    indices[i] = i;
  }
  qsort(indices, patterns->pattern_count, sizeof(int), evcr_compare);

#ifdef DEBUG
  printf("Top EVCrs: %.3f%% %.3f%% %.3f%% (%.3f%%)\n", 100*evcr_results[indices[0]], 100*evcr_results[indices[1]], 100*evcr_results[indices[2]], 100*evcr_results[indices[patterns->pattern_count - 1]]);
#endif // DEBUG

  free(evcr_results);

  filter_patterns(patterns, indices, filter_count);
  free(indices);
}

void filter_time(pt_t patterns, int test_count, int filter_count) {
  time_results = malloc(sizeof(int)*patterns->pattern_count);
  time_patterns(patterns, test_count, time_results);

#ifdef DEBUG
  printf("Top EVCrs times: %d %d %d (%d)\n", time_results[0], time_results[1], time_results[2], time_results[patterns->pattern_count - 1]);
#endif // DEBUG

  int* indices = malloc(sizeof(int)*patterns->pattern_count);
  for(int i = 0; i < patterns->pattern_count; i++) {
    indices[i] = i;
  }
  qsort(indices, patterns->pattern_count, sizeof(int), time_compare);
  #ifdef DEBUG
    printf("Debug times: %d %d %d (%d)\n", time_results[indices[0]], time_results[indices[1]], time_results[indices[2]], time_results[indices[patterns->pattern_count - 1]]);
  #endif // DEBUG
  free(time_results);

  filter_patterns(patterns, indices, filter_count);
  free(indices);
}

void mutate_repeat_subpatterns(pt_t patterns, bool less) {
  int cur_count = patterns->pattern_count;
  for(int i = 0; i < cur_count; i++) {
    if(less && (random() % 2) == 0)
      continue;
    ppattern_t pat = patterns->patterns[i];
    int start = random() % pat.length;
    int sub_len = 1 + random() % (pat.length - start);

    ppattern_t mut = pat;
    for(int k = 0; k < sub_len; k++) {
      mut.pattern[start + sub_len + k] = pat.pattern[start + k];
    }
    for(int k = 0; k < pat.length - sub_len - start; k++){
      mut.pattern[start + sub_len + sub_len + k] = pat.pattern[start + sub_len + k];
    }

    assert(pat.length + sub_len <= PATTERN_CAPACITY);
    mut.length += sub_len;

    patterns_push(patterns, mut);
  }
}

void shuffle(uint8_t* arr, int size) {
  if(size > 1) {
    for(int i = size - 1; i > 0; i--) {
      int j = rand() % (i + 1);
      uint8_t temp = arr[j];
      arr[j] = arr[i];
      arr[i] = temp;
    }
  }
}

void mutate_permute_order(pt_t patterns, int permutes, bool less) {
  int cur_count = patterns->pattern_count;
  for(int i = 0; i < cur_count; i++) {
    if(less && (random() % 2) == 0)
      continue;

    int pat_len = patterns->patterns[i].length;
    int num_perms = 1;
    if(pat_len == 1) {
      continue;
    } else if(pat_len == 2) {
      num_perms = 1;
    } else if(pat_len == 3) {
      num_perms = 4;
    } else {
      num_perms = permutes;
    }

    for(int j = 0; j < num_perms; j++){
      ppattern_t pat = patterns->patterns[i];
      shuffle(pat.pattern, pat_len);
      patterns_push(patterns, pat);
    }
  }
}

void mutate_interleave_target(pt_t patterns, bool less) {
  int cur_count = patterns->pattern_count;
  for(int i = 0; i < cur_count; i++) {
    if(less && (random() % 2) == 0)
      continue;
  
    int pat_len = patterns->patterns[i].length;
    int max_iter = 4;
    if(pat_len == 1) {
      max_iter = 1;
    } else if(pat_len == 2) {
      max_iter = 2;
    }
    for(int j = 0; j < max_iter; j++) {
      int num_target = 1;
      if(pat_len > 1) {
        if(random() % 2 == 0)
          num_target++;
        if(pat_len > 3) {
          if(random() % 8 == 0)
            num_target++;
        }
      }

      ppattern_t orig = patterns->patterns[i];
      ppattern_t pat = orig;
      for(int k = 0; k < num_target; k++) {
        int idx = random() % (pat.length + 1);
        for(int a = idx; a < pat.length; a++) {
          pat.pattern[a+1] = orig.pattern[a];
        }
        pat.pattern[idx] = PATTERN_TARGET;
        pat.length++;

        orig = pat;
      }
      patterns_push(patterns, pat);
    }
  }
}

pt_results_t produce_results(pt_t patterns) {
  assert(patterns->pattern_count == PT_RESULTS_COUNT);

  pt_results_t results = (pt_results_t)malloc(sizeof(struct pt_results));
  memcpy(results->patterns, patterns->patterns, PT_RESULTS_COUNT * sizeof(ppattern_t));

  test_patterns(patterns, 1000000, (float*) &results->evcrs);
  time_patterns(patterns, 100, (int*) &results->cycles);

  return results;
}

// "PrimeTime"
pt_results_t generate_prime_patterns(pt_t patterns) {
  // 1. Generate initial patterns
  ppattern_t template;
  for(int i = 0; i < 16; i++){
    template.pattern[i] = i;
  }
  for(int r = 1; r <= 8; r++) {
    template.repeat = r;
    for(int s = 1; s <= 4; s++) {
      template.stride = s;
      for(int w = 0; w < 4; w++){
        template.width = w;
        template.length = w+1;
        patterns_push(patterns, template);
      }
    }
  }

#ifdef DEBUG
  printf("%d initial patterns\n", patterns->pattern_count);
#endif // DEBUG

  // 2. Mutation:
  //  - Repeated access to (sub-)patterns
  //  - Permuation of access orders
  //  - Interleaving of target accesses
  mutate_repeat_subpatterns(patterns, false);
  mutate_permute_order(patterns, 16, false);
  mutate_interleave_target(patterns, false);

#ifdef DEBUG
  printf("%d initial mutated patterns\n", patterns->pattern_count);
#endif // DEBUG

  // 3. Measurements: Test 10'000 times
  //  - Filter to 150 highest EVCr
  //  - Filter to 100 fastest
  filter_evcr(patterns, 10000, 150);
  filter_time(patterns, 20, 100);

#ifdef DEBUG
  printf("Filtered to top %d patterns\n", patterns->pattern_count);
#endif // DEBUG

  // 4. Further mutations of candidates
  mutate_repeat_subpatterns(patterns, true);
  mutate_permute_order(patterns, 4, true);
  mutate_interleave_target(patterns, true);

#ifdef DEBUG
  printf("%d further mutated patterns\n", patterns->pattern_count);
#endif // DEBUG

  // 5. Measurements: Test 100'000 times
  //  - Filter to 150 Highest EVCr
  //  - Filter to 100 fastest  
  filter_evcr(patterns, 100000, 150);
  filter_time(patterns, 100, PT_RESULTS_COUNT);

  return produce_results(patterns);
}