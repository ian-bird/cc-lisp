#ifndef GC_H
#define GC_H

#include <stdlib.h>

void *gc_alloc(size_t);

void free_all();

void registerWip(void *);

void clearWip();

#endif
