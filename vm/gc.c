#include "gc.h"

void *gc_malloc(size_t s) 
{
    return malloc(s);
}

void *gc_realloc(void *prev, size_t s)
{
    return realloc(prev, s);
}