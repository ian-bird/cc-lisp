#ifndef GC_H
#define GC_H

#include <stdlib.h>

void *gc_malloc(size_t);

void *gc_realloc(void *, size_t);

#endif
