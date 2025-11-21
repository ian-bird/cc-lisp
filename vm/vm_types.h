typedef int Symbol;

typedef struct Binding {
  Symbol name;
  Reg val;
} Binding;

typedef struct Env {
  Binding *bindings;
  int numBindings;
  int capacity;
} Env;

typedef enum InstructionType {
  jal,
  typChk,
  assert,
  arityChk,
  lookup,
  push,
  pop,
  add,
  sub,
  mult,
  divide,
  cond,
  cons,
  car,
  load,
  move,
  cmp,
  set,
  cdr,
  ret
} InstructionType;

#define RESULT_I 0
#define CONT_I 1

typedef int RegI;

typedef struct Instruction {
  InstructionType type;
  union {
    RegI jal, push, pop, assert, lookup, car, cdr;
    struct {
      RegI l;
      Reg v;
    } load;
    struct {
      RegI l;
      RegI r;
    } arityChk, typChk, move, add, sub, mult, div, cons, cmp, set;
    struct {
      RegI pred;
      RegI consq;
      RegI alt;
    } cond;
  } val;
} Instruction;

typedef enum ValueType {
  number,
  proc,
  symbol,
  string,
  pair,
  boolean,
  null,
  err
} ValueType;

typedef struct Reg {
  ValueType type;
  Value *val;
} Reg;

typedef union Value {
  int number;
  struct {
    Reg *env;
    Instruction *code;
  } proc;
  Symbol symbol;
  int boolean;
  char *string;
  struct {
    Reg l;
    Reg r;
  } pair;
} Value;

typedef struct Machine {
  int numRegs;
  int regCap;
  Reg *regs;
  Instruction *pc;
  Env *globalEnv;
  int numArgs;
  int argsCap;
  Reg *args;
} Machine;
  
  
