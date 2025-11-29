#include "vm_types.h"
#include "gc.h"
#include "globals.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_ALLOC 8
#define SYMBOL_NOT_FOUND 1
#define FAILED_ARITY_CHECK 2
#define UNRECOGNIZED_INSTRUCTION 3
#define UNRECOGNIZED_FUNCALL 4


Value lookup(Environment env, Symbol s) 
{
  if(s.type == lexical) 
    return env.frames[s.val.lexical.frameNumber]->bindings[s.val.lexical.bindingNumber].value;

  // else its dynamic
  for(int i = env.numFrames; i-- > 0;)
  {
    for(int j = 0; j < env.frames[i]->numBindings; j++)
    {
      if(env.frames[i]->bindings[j].name.val.dynamic == s.val.dynamic)
        return env.frames[i]->bindings[j].value;
    }
  }

  longjmp(onErr, SYMBOL_NOT_FOUND);
}

#define REALLOC_FAM(old, structType, arrType, numMembers) \
  (structType *)gc_realloc(old, sizeof(structType) + sizeof(arrType) * ((numMembers) > 0 ? (numMembers) : 1 - 1))

#define ALLOC_FAM(structType, arrType, numMembers) \
  (structType *)gc_malloc(sizeof(structType) + sizeof(arrType) * ((numMembers) > 0 ? (numMembers) : 1 - 1))

Values *appendValue(Values *vs, Value v)
{
  Values *result = vs;
  if(result == NULL) 
  {
    result = ALLOC_FAM(Values, Value, INITIAL_ALLOC);
    result->valueCapacity = INITIAL_ALLOC;
    result->numValues = 0;
  } 
  else if(vs->numValues == vs->valueCapacity)
  {
    result = REALLOC_FAM(vs, Values, Value, vs->valueCapacity * 2);
    result->valueCapacity *= 2;
  }

  vs->values[vs->numValues++] = v;
  return vs;
}

Value toList(Value *values, int numValues)
{
  Value *memBlock = gc_malloc(sizeof(Value) * (numValues * 2 + 1));
  memBlock[0].type = null;
  for(int i = 0; i < numValues; i++)
  {
    memBlock[2 * i].type = pair;
    memBlock[2 * i].val.pair.car = memBlock + (2 * i + 1);
    memBlock[2 * i].val.pair.cdr = memBlock + (2 * i + 2);
    memBlock[2 * i + 1] = values[i];
  }
  memBlock[numValues * 2].type = null;
  return memBlock[0];
}

Function *extend(Function *f, Values *vs)
{
  Function *result = f;
  result->funEnv = ALLOC_FAM(Environment, Frame, result->funEnv->numFrames + 1);

  int capacity = f->numFixedArgs + f->hasFinalVarArg ? 1 : 0;
  Frame nextFrame = (Frame){capacity, capacity, gc_malloc(sizeof(Binding) * capacity)};

  int i = 0;
  for(; i < result->numFixedArgs; i++)
  {

    nextFrame.bindings[i].name.type = dynamic;
    nextFrame.bindings[i].name.val.dynamic = f->argNames[i];
    nextFrame.bindings[i].value = vs->values[i];
  }

  if(f->hasFinalVarArg)
  {
    nextFrame.bindings[i].name.type = dynamic;
    nextFrame.bindings[i].value = toList(vs->values + i, vs->numValues - i);
  }

  return result;
}

Values *setValues(Values *vs, int i, Value v)
{
  if(vs == NULL) 
  {
    Values *next = REALLOC_FAM(vs, Values, Value, INITIAL_ALLOC);
    next->valueCapacity = INITIAL_ALLOC;
    next->numValues = 0;
    return setValues(next, i, v);
  }
  
  if(i > vs->valueCapacity)
  {
    Values *next = REALLOC_FAM(vs, Values, Value, vs->valueCapacity * 2);
    next->valueCapacity *= 2;
    return setValues(next, i, v);
  }

  vs->values[i] = v;
  return vs;
}

Value run(Value *initialArgs, int numInitialArgs, Function *toExecute) 
{
  // set up the state for evaluation
  State s;
  s.current = NULL;
  for(int i = 0; i < numInitialArgs; i++) {
    s.current = appendValue(s.current, initialArgs[i]);
  }
  s.future = NULL;

  Values *vs = ALLOC_FAM(Values, Value, numInitialArgs);
  vs->numValues = numInitialArgs;
  vs->valueCapacity = numInitialArgs;
  memcpy(vs->values, initialArgs, numInitialArgs * sizeof(Value));

  s.fn = extend(toExecute, vs);
  s.instruction = 0;

  Continuation *k = gc_malloc(sizeof(Continuation));
   *k = (Continuation){(State){}, NULL};

  // start evaluation
  while(!(s.fn->instructions[s.instruction].type == ret && k->next == NULL))
  {
    Instruction thisInstr = s.fn->instructions[s.instruction];
    switch(thisInstr.type)
    {
      // var lookup does a dynamic or lexical lookup for a symbol inside
      // the current variable environment.
    case varLookup:
      s.result = lookup(*s.fn->varEnv, s.current->values[thisInstr.val.varLookup].val.sym);
      break;

      // fn lookup does a dynamic or lexical lookup for a symbol inside
      // the current fn environment.
    case fnLookup:
      s.result = lookup(*s.fn->funEnv, s.current->values[thisInstr.val.varLookup].val.sym);
      break;

      // we've reached the end of this function.
      // all we can pass on is the result value.
    case ret:
      {
        Value result = s.result;
        s = k->nextState;
        s.result = result;
        k = k->next;
      }
      break;

      // load takes a literal value (the argument in the instruction)
      // and moves it into the result register.
    case load:
      s.result = thisInstr.val.load;
      break;

      // move transfers the value in result into another register.
    case move:
      s.current = setValues(s.current, thisInstr.val.move, s.result);
      break;

      // add arg takes a register and appends it into the future state.
    case addArg:
      s.future = appendValue(s.future, s.current->values[thisInstr.val.addArg]);
      break;

      // arity checks the arity of a function call that is soon to be made.
      // it looks at the fucntion in the register and checks if the number of arguments
      // in the nextFrame is sufficient.
    case arity:
      {
        Procedure proc = s.current->values[thisInstr.val.arity].val.procedure;
        int numExpected = proc.type == continuation ? 1 : proc.val.function->numFixedArgs;
        int isVarArg = proc.type == continuation ? 0 : proc.val.function->hasFinalVarArg;
        if(!(s.future->numValues == numExpected || (s.future->numValues > numExpected && isVarArg)))
          longjmp(onErr, FAILED_ARITY_CHECK);
      }
      break;

      // goSub does all the work of going into a function or into a continuation.
      //
      // this is probably the single most complex instruction present.
      // it does the job of building the "toExecute" function, which contains both environments,
      // as well as saving all the stateful parts of the machine and loading the new values for that.
    case gosub:
      {
        Procedure proc = s.current->values[thisInstr.val.goSub].val.procedure;
        switch(proc.type) 
        {
          // we extend the continuation with everything left to do in this function,
          // and then we load the new state and apply the function.
        case funcall:
          {
            // creating the future state to return to
            s.current = s.future;
            s.future = NULL;
            s.instruction++;

            // saving it in the continuation
            Continuation *next = (Continuation *)gc_malloc(sizeof(Continuation));
            *next = (Continuation){s, k};
            k = next;

            // now go eval the function
            s.fn = extend(proc.val.function, s.current);
            s.instruction = 0;
          }
          break;

          // follow the cont in the procedure. Current stack is discarded.
        case continuation:
          {
            Value result = s.future->values[0];
            k = proc.val.cont->next;
            s = proc.val.cont->nextState;
            s.result = result;
          }
          break;

          // we need to load the new state and apply the function, but we keep the current cont.
        case tailcall:
          s = (State){s.future, NULL, (Value){}, extend(proc.val.function, s.current), 0};
          break;

          default:
          longjmp(onErr, UNRECOGNIZED_FUNCALL);
        }
      }
      continue;

    case unless:
      if(!s.current->values[thisInstr.val.unless.pred].val.boolean)
      {
        s.instruction = thisInstr.val.unless.where;
        continue;
      }
      break;

    case jmp:
      s.instruction = thisInstr.val.jmp;
      continue;

    case makeClosure:
      {
        Procedure proc = s.current->values[thisInstr.val.makeClosure].val.procedure;
        if(proc.type == continuation)
          break;

        proc.val.function->funEnv = s.fn->funEnv;
        proc.val.function->varEnv = s.fn->varEnv;
      }
      break;

    default:
      longjmp(onErr, UNRECOGNIZED_INSTRUCTION);
    }

    s.instruction++;
  }

  return s.result;
}