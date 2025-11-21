#ifndef LISP_H
#define LISP_H

#include "lisp_types.h"
#include <setjmp.h>

// evaluates a compiled lambda form.
// compiled lambdas exist as cps. 
// maybe there's something cool where we can pre-load all the nexts up to an "if", "funcall" or "pass".
// since all the other operations have a static next op.
Value EvaluateCps(Operation op, Environment suppliedEnv, jmp_buf handler);

#endif