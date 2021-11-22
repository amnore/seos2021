#ifndef SEOS2021_LAB3_KVECTOR_H
#define SEOS2021_LAB3_KVECTOR_H

#include "kalloc.h"
#include "klib.h"

#include <stdbool.h>

#define KVECTOR_DEFINE_TYPE(name, type)                                        \
  typedef struct name {                                                        \
    type *arr;                                                                 \
    int size;                                                                  \
    int cap;                                                                   \
  } name;

#define KVECTOR_DEFINE_OPS(name, type)                                         \
  _Pragma("GCC diagnostic push");                                              \
  _Pragma("GCC diagnostic ignored \"-Wunused-function\"");                     \
                                                                               \
  static inline void name##_init(name *v) {                                    \
    assert(v);                                                                 \
    v->arr = kalloc(sizeof(type) * 8);                                         \
    assert(v->arr);                                                            \
    v->size = 0;                                                               \
    v->cap = 8;                                                                \
  }                                                                            \
                                                                               \
  static inline void name##_drop(name *v) {                                    \
    assert(v);                                                                 \
    kfree(v->arr);                                                             \
    v->arr = NULL;                                                             \
  }                                                                            \
                                                                               \
  static inline bool name##_empty(name *v) {                                   \
    assert(v);                                                                 \
    return v->size == 0;                                                       \
  }                                                                            \
                                                                               \
  static inline int name##_size(name *v) {                                     \
    assert(v);                                                                 \
    return v->size;                                                            \
  }                                                                            \
                                                                               \
  static inline void __##name##_grow(name *v) {                                \
    assert(v);                                                                 \
    type *newarr = kalloc(sizeof(type) * 2 * v->cap);                          \
    assert(newarr);                                                            \
                                                                               \
    /*copy elements to the new array*/                                         \
    memcpy(newarr, v->arr, v->size * sizeof(type));                            \
    v->cap *= 2;                                                               \
    kfree(v->arr);                                                             \
    v->arr = newarr;                                                           \
  }                                                                            \
                                                                               \
  static inline void name##_push_back(name *v, type elem) {                    \
    if (v->size == v->cap) {                                                   \
      __##name##_grow(v);                                                      \
    }                                                                          \
    v->arr[v->size++] = elem;                                                  \
  }                                                                            \
                                                                               \
  static inline void name##_pop_back(name *v) {                                \
    assert(v && v->size > 0);                                                  \
    v->size--;                                                                 \
  }                                                                            \
                                                                               \
  static inline type name##_at(name *v, int index) {                           \
    assert(0 <= index && index < v->size);                                     \
    return v->arr[index];                                                      \
  }                                                                            \
                                                                               \
  static inline type name##_back(name *v) {                                    \
    assert(v && v->size > 0);                                                  \
    return v->arr[v->size - 1];                                                \
  }                                                                            \
                                                                               \
  static inline int name##_insert(name *v, int pos, type value) {              \
    assert(v && 0 <= pos && pos <= v->size);                                   \
                                                                               \
    if (v->size == v->cap) {                                                   \
      __##name##_grow(v);                                                      \
    }                                                                          \
                                                                               \
    memmove(v->arr + pos + 1, v->arr + pos, (v->size - pos) * sizeof(type));   \
    v->arr[pos] = value;                                                       \
    v->size++;                                                                 \
    return pos;                                                                \
  }                                                                            \
                                                                               \
  static inline int name##_erase(name *v, int pos) {                           \
    assert(v && 0 <= pos && pos < v->size);                                    \
                                                                               \
    memmove(v->arr + pos, v->arr + pos + 1,                                    \
            (v->size - pos - 1) * sizeof(type));                               \
    v->size--;                                                                 \
    return pos;                                                                \
  }                                                                            \
                                                                               \
  static inline void name##_clear(name *v) {                                   \
    assert(v);                                                                 \
    v->size = 0;                                                               \
  }                                                                            \
  _Pragma("GCC diagnostic pop")

#define KVECTOR_DEFINE(name, type)                                             \
  KVECTOR_DEFINE_TYPE(name, type);                                             \
  KVECTOR_DEFINE_OPS(name, type);

#endif // SEOS2021_LAB3_KVECTOR_H
