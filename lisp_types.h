#ifndef LISP_TYPES_H
#define LISP_TYPES_H
#include "slice.h"

typedef enum {funcall, ifOp, cons, car, cdr, quote, eq, atom, lambda, define, set, callcc, done} OperationType;

typedef int Symbol;

SLICE_TEMPLATE(arg, Argument)

SLICE_TEMPLATE(int, int)

typedef struct Operation {
  OperationType type;
  union {
    struct {
      Argument f;
      Slice_arg args;
    } funcall;
    struct {
      Argument predicate;
      Slice_op consequent, alternative;
    } ifOp;
    struct {
      Argument first, second;
    } cons, eq;
    struct {
      Argument arg;
    } car, cdr, atom, callcc;
    struct {
      Value val;
    } quote;
  } val;
  Persistance persistance;
} Operation;

#define VIA_APPEND -1

typedef struct Persistance {
  int save;
  int frame;
  int binding;
} Persistance;

typedef enum {envRef, lexRef, prevRef, literal} ArgumentType;

typedef struct{
  ArgumentType type;
  union {
    Symbol env;
    struct {
      int frameI;
      int bindingI;
    } lex;
    Value literal;
  } val;
} Argument;

typedef struct {
  Symbol name;
  Value *v;
} Binding;

SLICE_TEMPLATE(binding, Binding)

typedef struct {
  Slice_binding bindings;
} EnvFrame;

SLICE_TEMPLATE(frame, EnvFrame)

typedef struct{
  Slice_frame frames;
} Environment;

typedef enum {null, pair, number, symbol, boolean, string, fn} ValueType;

typedef struct{
  ValueType type;
  union {
    double number;
    char *string;
    int symbol;
    int boolean;
    Lambda fn;
    struct {
      Value *left;
      Value *right;
    } pair;
  } val;
} Value;

SLICE_TEMPLATE(symbol, Symbol)

SLICE_TEMPLATE(op, Operation)

typedef struct Lambda {
  Environment env;
  unsigned int entryOp;
  Slice_op ops;
  int numArgs;
  Lambda *k;
} Lambda;

#define UNNAMED -1

#endif
