#include "type_conv.h"
#include "lisp_globals.h"

int toBoolean(Value v) {
  if(v.type != boolean) {
    longjmp(*onErr, -1);
  }

  return v.val.boolean;
}

Value nullValue() {
  Value result;
  result.type = null;
  return result;
}

Lambda toLambda(Value v) {
  if(v.type != fn) {
    longjmp(*onErr, -1);
  }

  return v.val.fn;
}

Value box(void *val, ValueType t) {
  Value result;
  result.type = t;
  switch(t) {
  case null:
    break;

    // cannot box a pair. Only can be constructed via cons.
  case pair:
    longjmp(*onErr, -3);

  case number:
    result.val.number = *(double *)val;
    break;

  case symbol:
    result.val.number = *(int *)val;
    break;

  case boolean:
    result.val.boolean = *(int *)val;
    break;

  case string:
    result.val.string = *(char **)val;
    break;

  case lambda:
    result.val.fn = *(Lambda *)val;
    break;

  default:
    longjmp(*onErr, -2);
  }

  return result;
}