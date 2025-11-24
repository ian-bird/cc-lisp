#ifndef VM_TYPES_H
#define VM_TYPES_H

typedef struct StackFrame {
  Instruction *returnAddr;
  int numArgs;
  Value data[]; // the first numArgs values are set by the caller and constitute arguments pased to the function.
                // the remaining values are local arguments. This is a fixed number and the fn binding should contain
                // the info.
} StackFrame;

typedef struct Stack {
  int numFrames;
  int capacity;
  StackFrame *frames[]; // array of pointers to frames
} Stack;

typedef enum ValueType {number, symbol, pair, boolean, character, procedure, promise, null} ValueType;

typedef struct Value {
  ValueType type;
} Value;

typedef int Symbol;

typedef struct Binding {
  Symbol name;
  Value value;
} Binding;

typedef struct EnvFrame {
  int numBindings;
  int capacity;
  Binding bindings[];
} EnvFrame;

typedef struct Environment {
  int numFrames;
  int capacity;
  EnvFrame *frames[];
} Environment;

typedef struct Function {
  Environment funEnv; // using deep binding.
  Environment varEnv; 
  int numFixedArgs;
  int hasFinalVarArg;
  Instruction *returnAddress;
  Instruction entryPoint[];
} Function;

typedef struct Continuation {
  Stack s;
} Continuation;

typedef enum InstructionType {varLookup, fnLookup, ret, load, move, addArg, arity, type, gosub, unless, jmp} InstructionType;

typedef int Register;

typedef struct Instruction {
  InstructionType type;
  union {
    Register varLookup, fnLookup, move, addArg, arity, goSub;
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

typedef struct Machine {
  Stack s;
  Value result;
  Instruction *ip;
  int numRegisters;
  int registerCapacity;
  Value registers[];
} Machine;

#endif