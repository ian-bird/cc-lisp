#ifndef SLICE_H
#define SLICE_H

typedef struct Slice {
    size_t capacity;
    size_t elemSize;
    unsigned int count;
    void *p;
} Slice;

// create a new slice
Slice newSlice(size_t elemSize);

// gets the nth element in the slice, with a bounds check.
void *nth(Slice s, int i);

// appends an element to the slice
Slice append(Slice s, void *v);

// gets the length of a slice
unsigned int len(Slice s);

// merges 2 slices
Slice concat(Slice a, Slice b);

Slice clone(Slice s);

void insert(Slice s, int i, void *v);

#define SLICE_TEMPLATE(n, T)                 \
                                             \
    typedef Slice Slice_##n;                 \
                                             \
    Slice_##n new_slice_##n() {              \
        Slice result = newSlice(sizeof(T));  \
        return *(Slice_##n *)(&result);      \
    }                                        \
                                             \
    T nth_##n(Slice_##n s, int i) {          \
        Slice input = *(Slice *)(&s);        \
        return *(T *)nth(input, i); }        \
                                             \
    Slice_##n append_##n(Slice_##n s, T v) { \
        Slice input = *(Slice *)(&s);        \
        Slice result = append(input, &v);    \
        return *(Slice_##n *)(&result);      \
    }                                        \
                                             \
    void set_##n(Slice_##n s, int i, T v) {  \
        Slice input = *(Slice *)(&s);        \
        insert(input, i, &v);                \
    }

#endif