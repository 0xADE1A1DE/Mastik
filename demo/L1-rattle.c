#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

volatile char buffer[4096];

int main(int ac, char **av) {
  for (;;) {
    for (int i = 0; i < 64000; i++)
      buffer[800] += i;
    for (int i = 0; i < 64000; i++)
      buffer[1800] += i;
  }
}
