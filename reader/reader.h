#ifndef READER_H
#define READER_H

#include "../lisp_types.h"
#include "../lisp_globals.h"

// reader function. Converts string into lisp expression.
Value read(char *input);

#endif
