#ifndef KERNEL_KLIB_EXTRA_H
#define KERNEL_KLIB_EXTRA_H

#include <stddef.h>
#include <stdarg.h>
#include <stdnoreturn.h>

#ifndef __cplusplus
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

int sscanf(const char *str, const char *format, ...);

int vsscanf(const char *str, const char *format, va_list ap);

int isspace(int ch);

int isdigit(int ch);

char *strchr(char *str, int ch);

char *strrchr(char *str, int ch);

char *strnrchr(char *str, int len, int ch);

char *strdup(const char *str);

char *strndup(const char *str, size_t n);

#define RAND_MAX INT_MAX

noreturn void exit_safe(int code);

#define panic_fmt(cond, fmt, ...)                                              \
  do {                                                                         \
    if (cond) {                                                                \
      printf(fmt, ##__VA_ARGS__);                                              \
      exit_safe(1);                                                            \
    }                                                                          \
  } while (0)

#endif
