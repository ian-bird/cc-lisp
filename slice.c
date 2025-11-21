#include "slice.h"
#include "gc.h"
#include "lisp_globals.h"

typedef struct {
    size_t capacity;
    size_t elemSize;
    unsigned int count;
    void *p;
} Slice;

Slice newSlice(size_t size) {
    Slice result = (Slice){size * 8, size, 0, gc_alloc(size * 8)};
    registerWip(result.p);
    return result;
}

void *nth(Slice s, int i) {
    // dynamic bounds check
    if(i < 0 || i > s.count)
        longjmp(*onErr, -5);

    return  uncheckedNth(s, i);
}

void set(Slice s, int i, void *v) {
    if(i < 0 || i > s.count)
        longjmp(*onErr, -5);

    memcpy((char *)s.p + i * s.elemSize, v, s.elemSize);
}

void *uncheckedNth(Slice s, int i) {
    return (char *)(s.p) + s.elemSize * i;
}

Slice append(Slice s, void *v) {
    Slice result = s;

    if(s.count * s.elemSize < s.capacity) {
        result.count++;
        memcpy(s.p + s.elemSize * s.count, v, s.elemSize);
        return result;
    }

    result.capacity = s.capacity * 2;
    result.p = gc_alloc(s.capacity * 2);
    registerWip(result.p);

    memcpy(result.p, s.p, s.count * s.elemSize);
    return append(result, v);
}

// gets the length of the slice passed to it
unsigned int len(Slice s) {
    return s.count;
}

// this walks the underlying array and appends all the elements
Slice concat(Slice a, Slice b) {
    int i = 0;
    Slice result;

    for(; i < len(b); i++)
        result = append(a, uncheckedNth(b, i));

    return result;
}

Slice clone(Slice s) {
    Slice result = s;
    result.p  = gc_alloc(s.capacity);
    registerWip(result.p);

    memcpy(result.p, s.p, s.count * s.elemSize);
    return result;
}