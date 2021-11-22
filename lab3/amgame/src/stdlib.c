#include "am.h"
#include "klib.h"

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

static uint64_t randseed = 1;

int rand() {
  // xorshift
  randseed ^= randseed << 13;
  randseed ^= randseed >> 7;
  randseed ^= randseed << 17;
  int ret = randseed & INT_MAX;

  return ret;
}

void srand(unsigned int seed) {
  randseed = seed == 0 ? 1 : seed;
}

// klib-extra.h

int abs(int x) { return (x < 0 ? -x : x); }

int atoi(const char *nptr) {
  int x = 0;
  while (*nptr == ' ') {
    nptr++;
  }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr++;
  }
  return x;
}

void exit_safe(int code) {
  static int exit_flag;
  if (!atomic_xchg(&exit_flag, 1)) {
    printf("CPU #%d Halt (%x).\n", cpu_current(), code);
    // qemu will somehow panic if halt() is called, so we exit manually
    asm volatile("outw %%ax, %%dx" ::"a"(0x2000), "d"(0x604));
  }
  while (true)
    continue;
}
