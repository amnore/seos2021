#ifndef SEOS2021_LAB3_KALLOC_H
#define SEOS2021_LAB3_KALLOC_H

#include <stddef.h>

void *kalloc(size_t size);

void kfree(void *ptr);

#endif // SEOS2021_LAB3_KALLOC_H
