#include "am.h"
#include "klib.h"
#include "klib-extra.h"
#include "klib-macros.h"

#include <limits.h>
#include <stdarg.h>
#define PRINTF_BUF_SZ 1024

static void _print_serial(const char *str) {
  for (; *str; str++) {
    if (*str != '\n') {
      putch(*str);
    } else {
      // hack to make serial display correctly
      putch('\r');
      putch('\n');
    }
  }
}

int printf(const char *fmt, ...) {
  va_list vl;
  char buf[PRINTF_BUF_SZ];
  va_start(vl, fmt);
  int sz = vsnprintf(buf, PRINTF_BUF_SZ, fmt, vl);
  va_end(vl);

  _print_serial(buf);

  return MIN(sz, PRINTF_BUF_SZ - 1);
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, SIZE_MAX, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  int sz = vsprintf(out, fmt, vl);
  va_end(vl);
  return sz;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  int sz = vsnprintf(out, SIZE_MAX, fmt, vl);
  va_end(vl);
  return sz;
}

/**
 * vsnprintf supports %%, %c, %s, %d, %x and %p
 */
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  size_t outi = 0, fmti = 0;
#define _append(ch) ((outi < n ? (out[outi] = (ch)) : '\0'), outi++)
  while (fmt[fmti]) {
    if (fmt[fmti] != '%') {
      _append(fmt[fmti]);
      fmti++;
      continue;
    }

    switch (fmt[fmti + 1]) {
    case '%':
      _append(fmt[fmti]);
      fmti += 2;
      break;
    case 'c': {
      char ch = (char)va_arg(ap, int);
      _append(ch);
      fmti += 2;
      break;
    }
    case 's': {
      char *s = va_arg(ap, char *);
      for (int i = 0; s[i]; i++) {
        _append(s[i]);
      }
      fmti += 2;
      break;
    }
    case 'd': {
      int d = va_arg(ap, int);
      unsigned u;
      if (d < 0) {
        _append('-');
        u = -d;
      } else {
        u = d;
      }

      unsigned k = 1;
      while (k <= UINT_MAX / 10 && k * 10 <= u)
        k *= 10;
      while (k) {
        _append(u / k + '0');
        u %= k;
        k /= 10;
      }

      fmti += 2;
      break;
    }
#define _fmt_hex(type)                                                         \
  {                                                                            \
    bool prefix = true;                                                        \
    type u = va_arg(ap, type);                                                 \
    if (u == 0) {                                                              \
      _append('0');                                                            \
    } else {                                                                   \
      for (int shift_n = sizeof(type) * 8 - 4; shift_n >= 0; shift_n -= 4) {   \
        unsigned bits = (u >> shift_n) & 0b1111;                               \
        prefix = prefix && !bits;                                              \
        if (!prefix)                                                           \
          _append(bits < 10 ? bits + '0' : bits - 10 + 'a');                   \
      }                                                                        \
    }                                                                          \
  }
    case 'x':
      _fmt_hex(unsigned);
      fmti += 2;
      break;
    case 'p': {
      _append('0');
      _append('x');
      _fmt_hex(uintptr_t);
      fmti += 2;
      break;
    }
#undef _fmt_hex
    default:
      panic("Not implemented");
    }
  }

  if (outi < n)
    out[outi] = '\0';
  else if (n)
    out[n - 1] = '\0';
  return outi;
}

// klib-extra.h

int sscanf(const char *str, const char *format, ...) {
  va_list args;
  va_start(args, format);
  return vsscanf(str, format, args);
}

/**
 * vsscanf supports %s and %d.
 */
int vsscanf(const char *str, const char *format, va_list ap) {
  assert(str && format);

  int read_args = 0;
  while (*str && *format) {
    if (isspace(*format)) {
      while (isspace(*str))
        str++;
      format++;
      continue;
    }

    if (*format != '%') {
      if (*str != *format)
        break;
      str++;
      format++;
      continue;
    }

    switch (*(format + 1)) {
    case '%':
      if (*str != '%')
        goto end;
      str++;
      format += 2;
      break;
    case 's': {
      char *p = va_arg(ap, char *);
      while (*str && !isspace(*str))
        *p++ = *str++;
      *p = '\0';
      format += 2;
      read_args++;
      break;
    }
    case 'd': {
      int *p = va_arg(ap, int *);
      bool negative = false;

      if (*str == '-') {
        negative = true;
        str++;
      } else if (*str == '+') {
        str++;
      }

      if (!isdigit(*str)) {
        goto end;
      }

      *p = 0;
      while (isdigit(*str)) {
        *p *= 10;
        *p += *str - '0';
        str++;
      }

      if (negative) {
        *p = -*p;
      }
      read_args++;
      break;
    }
    default:
      panic("Not implemented");
    }
  }

end:
  return read_args;
}
