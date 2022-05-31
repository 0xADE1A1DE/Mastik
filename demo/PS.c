#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <mastik/ps.h>
#include <mastik/low.h>
#include <mastik/util.h>

enum SHARED_STATE {
  STATE_INIT,
  STATE_SHOULD_ACCESS,
};
volatile enum SHARED_STATE* shared_state;

pid_t setup_child(void* target) {
  shared_state = mmap(NULL, sizeof(enum SHARED_STATE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *shared_state = STATE_INIT;

  pid_t child_pid = fork();
  if(child_pid == 0) {
    prctl(PR_SET_PDEATHSIG, SIGKILL);
    setaffinity(3);
    while(1) {
      while(*shared_state == STATE_INIT) {}
      mfence();
      memaccesstime(target);
      mfence();
      *shared_state = STATE_INIT;
    }
  }

  return child_pid;
}

void cleanup_child(pid_t pid) {
  kill(pid, SIGKILL);
  waitpid(pid, 0, 0);
  munmap((void*)shared_state, sizeof(enum SHARED_STATE));
}

#define TARGET_LINE 31
#define TRIALS 100
int main() {
  // Intel Core i5-8250U: R4_S4_P0S123S1231S23 [99.98% Success rate, 1625 cycles]
  uint8_t _pat[13] = {0, PATTERN_TARGET, 1, 2, 3, PATTERN_TARGET, 1, 2, 3, 1, PATTERN_TARGET, 2, 3};
  ppattern_t best_pat = construct_pattern(4, 4, _pat, sizeof(_pat));

  mm_t mm = mm_prepare(NULL, NULL, NULL);
  assert(mm);
  ps_t ps = ps_prepare(best_pat, NULL, mm);
  assert(ps);

  void* target = mm_requestline(mm, L3, TARGET_LINE);
  // NOTE: We are somwhat abusing this for demonstration purposes -
  //       we are relying on the fact that Linux uses copy-on-write for
  //       for MAP_PRIVATE mmap-allocations: This line comes from the
  //       mm->memory buffer, which is MAP_PRIVATE and so in theory it
  //       is not shared with the forked process, but practically it is.

  ps_monitor(ps, TARGET_LINE);

  pid_t child_pid = setup_child(target);
  setaffinity(2);

  int success = 0;
  for(int i = 0; i < TRIALS; i++) {
    ps_prime(ps);

    uint16_t t_res;

    ps_scope(ps, &t_res);
    if(t_res > L3_THRESHOLD)
      continue;

    ps_scope(ps, &t_res);
    if(t_res > L3_THRESHOLD)
      continue;

    *shared_state = STATE_SHOULD_ACCESS;
    mfence();
    while(*shared_state == STATE_SHOULD_ACCESS) {}
    mfence();

    ps_scope(ps, &t_res);
    if(t_res < L3_THRESHOLD)
      continue;

    success += 1;
  }

  printf("%d/%d Successful\n", success, TRIALS);

  ps_release(ps);
  cleanup_child(child_pid);

  return 0;
}