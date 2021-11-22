#include "klib.h"
#include "klib-extra.h"

#include <stdint.h>

size_t strlen(const char *s) {
  size_t sz = 0;
  while (s[sz]) {
    sz++;
  }
  return sz;
}

char *strcpy(char *dst, const char *src) {
  size_t i = 0;
  while (src[i]) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = src[i];
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  while (i < n && src[i]) {
    dst[i] = src[i];
    i++;
  }
  while (i < n) {
    dst[i] = '\0';
    i++;
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t dst_len = strlen(dst);
  strcpy(dst + dst_len, src);
  return dst;
}

int strcmp(const char *s1, const char *s2) { return strncmp(s1, s2, SIZE_MAX); }

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i = 0;
  const unsigned char *u1 = (const unsigned char *)s1,
                      *u2 = (const unsigned char *)s2;
  while (i < n && u1[i] && u2[i]) {
    if (u1[i] != u2[i])
      return (int)u1[i] - (int)u2[i];
    i++;
  }
  return i == n ? 0 : (int)u1[i] - (int)u2[i];
}

void *memset(void *s, int c, size_t n) {
  for (size_t i = 0; i < n; i++) {
    *((unsigned char *)s + i) = c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *u1 = dst;
  const unsigned char *u2 = src;
  if (u1 <= u2) {
    for (size_t i = 0; i < n; i++)
      u1[i] = u2[i];
  } else {
    for (size_t i = n - 1; i != SIZE_MAX; i--)
      u1[i] = u2[i];
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  return memmove(out, in, n);
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *c1 = s1, *c2 = s2;
  for (size_t i = 0; i < n; i++) {
    if (c1[i] != c2[i])
      return (int)c1[i] - (int)c2[i];
  }
  return 0;
}

char *strchr(char *str, int ch) {
  char c = (char)ch;

  while (*str && *str != c)
    str++;

  return *str ? str : NULL;
}

char *strrchr(char *str, int ch) { return strnrchr(str, strlen(str), ch); }

char *strnrchr(char *str, int len, int ch) {
  assert(str);

  for (int i = len - 1; i >= 0; i--) {
    if (str[i] == (char)ch)
      return str + i;
  }
  return NULL;
}

