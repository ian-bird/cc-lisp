#ifndef RUNTIME_H
#define RUNTIME_H

// evaluates a compiled function, returning the result of evaluation.
//
// implements lexical binding, proper tail-recursion, reified continuations
// (via spaghetti stack)
Value run(Value *initialArgs, int numInitialArgs, Function *toExecute);

#endif
