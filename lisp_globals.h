#ifndef LISP_GLOBALS_H
#define LISP_GLOBALS_H

#include "lisp_types.h"
#include <setjmp.h>

// global state. I would've preferered to have used closures but. It's C.
Environment env;
jmp_buf *onErr;
Value *prev;

Slice_op block;

void **wip;
int numWip;
int wipCapacity;

#endif