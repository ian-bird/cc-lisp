#ifndef VM_TYPES_H
#define VM_TYPES_H

typedef enum ProcedureType {tailcall, funcall, continuation} ProcedureType;

typedef struct Procedure {
  ProcedureType type;
  union {
    Function function;
    Continuation cont;
  } val;
} Procedure;

typedef enum ValueType {number, symbol, pair, boolean, character, procedure, promise, null} ValueType;

typedef struct Value {
  ValueType type;
  union {
    Symbol sym;
    Procedure procedure;
    int boolean;
    struct {
      Value *car;
      Value *cdr;
    } pair;
  } val;
} Value;

typedef enum SymbolType {lexical, dynamic} SymbolType;

typedef struct Symbol {
  SymbolType type;
  union {
    int dynamic;
    struct {
      int frameNumber, bindingNumber;
    } lexical;
  } val;
} Symbol;

typedef struct Binding {
  Symbol name;
  Value value;
} Binding;

typedef struct Frame {
  int numBindings;
  int capacity;
  Binding *bindings;
} Frame;

typedef struct Environment {
  int numFrames;
  Frame *frames[];
} Environment;

typedef struct Function {
  Environment *funEnv; // using deep binding.
  Environment *varEnv;
  int *argNames; // length is num fixed args (+1 if hasFinalVarArg is 1) 
  int numFixedArgs;
  int hasFinalVarArg;
  Instruction instructions[];
} Function;

typedef enum InstructionType {varLookup, fnLookup, ret, load, move, addArg, arity, type, gosub, unless, jmp, makeClosure} InstructionType;

typedef int Register;

typedef struct Instruction {
  InstructionType type;
  union {
    Register varLookup, fnLookup, move, addArg, arity, goSub, makeClosure;
    Value load;
    struct {
      Register pred;
      int where;
    } unless;
    struct {
      Register what;
      ValueType expected;
    } type;
    int jmp;
  } val;
} Instruction;


typedef struct Values {
  int numValues;
  int valueCapacity;
  Value values[];
} Values;

typedef struct State {
  Values *current;
  Values *future;
  Value result;
  Function *fn;
  int instruction;
} State;

typedef struct Continuation {
  State nextState;
  Continuation *next;
} Continuation;

#endif