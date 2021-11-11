#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
uint8_t *p;

typedef void (*fptr)(void);

#define RET_OPCODE 0xC3
int main(int c, char **v) {
  p = mmap(0, 4096, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  for (int i = 0; i < 4096; i++)
    p[i] = RET_OPCODE;

  while (1) {
    for (int i = 0; i < 32000; i++)
      (*((fptr)(p + 1880)))();
    for (int i = 0; i < 32000; i++)
      (*((fptr)(p + 880)))();
    for (int i = 0; i < 32000; i++)
      (*((fptr)(p + 2880)))();
    for (int i = 0; i < 64000; i++)
      (*((fptr)(p + 880)))();
  }
}
