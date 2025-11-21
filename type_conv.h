#ifndef TYPE_CONV_H
#define TYPE_CONV_H

#include "lisp_types.h"

Value box(void *val, ValueType t);

Value nullValue();

Lambda toLambda(Value v);

int toBoolean(Value v);

#endif