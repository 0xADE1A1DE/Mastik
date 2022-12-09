#include <stdio.h>
#include <stdlib.h>

#include <mastik/ps.h>
#include <mastik/pt.h>
#include <mastik/l3.h>

int main() {
  pt_t patterns = pt_prepare(NULL, NULL);
  pt_results_t results = generate_prime_patterns(patterns);

  printf("EVCr    | Cycles | Pattern\n");
  for(int i = 0; i < PT_RESULTS_COUNT; i++){
    printf("%7.3f%% | %6d | ", results->evcrs[i]*100, results->cycles[i]);
    dump_ppattern(results->patterns[i]);
    printf("\n");
  }

  free(results);
}