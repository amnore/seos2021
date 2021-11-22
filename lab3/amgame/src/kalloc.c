#include "kalloc.h"
#include "am.h"

void *kalloc(size_t size) {
  static void *p = NULL;
  if (!p) {
    p = heap.start;
  }

  if (p + size <= heap.end) {
    void *ptr = p;
    p += size;
    return ptr;
  }

  return NULL;
}

void kfree(void *ptr) {}
