#include <setjmp.h>
#include <stdlib.h>

#include "gc.h"
#include "lisp_types.h"
#include "lisp_globals.h"
#include "type_conv.h"

Value *lookup(Symbol s) {
  int i, j;
  for(i = env.frames.count; i-- > 0;) {
    for(j = 0; j < nth_frame(env.frames, i).bindings.count; j++) {
      if(s == nth_binding(nth_frame(env.frames, i).bindings, j).name)
        return nth_binding(nth_frame(env.frames, i).bindings, j).v;
    }
  }

  longjmp(*onErr, -4);
}

// consFn conses 2 values together and returns the pair
// as a lisp value. 
Value consFn(Value *l, Value *r) {
  Value result;
  result.type = pair;
  result.val.pair.left = l;
  result.val.pair.right = r;
  return result;
}

Value *carFn(Value v) {
  if(v.type != pair) {
    longjmp(*onErr, -1);
  }

  return v.val.pair.left;
}

Value *cdrFn(Value v) {
   if(v.type != pair) {
    longjmp(*onErr, -1);
  }

  return v.val.pair.right;
}

int valuesEqual(Value l, Value r) {
  if (l.type != r.type) {
    return 0;
  } 

  switch(l.type) {
  case null:
    return 1;

  case pair:
    if(l.val.pair.left != r.val.pair.left){
      if(l.val.pair.left == NULL || r.val.pair.left == NULL)
        return 0;

      if(!valuesEqual(*l.val.pair.left, *r.val.pair.left))
        return 0;
    }

    if(l.val.pair.right != r.val.pair.right){
      if(l.val.pair.right == NULL || r.val.pair.right == NULL)
        return 0;

      if(!valuesEqual(*l.val.pair.right, *r.val.pair.right))
        return 0;
    }

    return 1;


  case number:
    return l.val.number == r.val.number;

  case symbol:
    return l.val.symbol == r.val.symbol;

  case boolean:
    return l.val.boolean == r.val.boolean;

  case string:
    return l.val.string == r.val.string;

  case lambda:
    // todo fix
    return environmentsEqual(l.val.fn.env, r.val.fn.env) && l.val.fn.op == r.val.fn.op;

    // unknown type
  default:
    longjmp(*onErr, -2);
  }
}

Value atomFn(Value v) {
  int result = (v.type != pair && v.type != null);
  return box(&result, boolean);
}

// converts an argument to a value.
// this can either be an environment lookup, a reference to the previous computed value,
// or a literal value.
//
// environment lookups are O(1) if they're a lexical reference and O(N)
// if they're a symbolic reference, since the binding has to be dynamically located at runtime.
Value *getLiteralValue(Argument a) {
  Value *container;
  switch(a.type) {
    case lexRef:
      return nth_binding(nth_frame(env.frames, a.val.lex.frameI).bindings, a.val.lex.bindingI).v;

    case envRef:
      return lookup(a.val.env);

    case prevRef:
      return prev;

    case literal:
      container = reg_alloc(sizeof(Value));
      *container = a.val.literal;
      return container;

    default:
      longjmp(*onErr, -3);
  }
}

// allocates garbage collected memory
// and registers it with the wip table.
// this ensures that the memory won't be cleaned up until
// we move on to the next iteration
void *reg_alloc(size_t amount) {
  void *result = gc_alloc(amount);
  registerWip(result);
  return result;
}

// this takes an operation and optionally persists the value.
// the location of the persistance must be computed ahead of time.
// all references to it must be lexical since no name is compiled in.
//
// builds in amortized O(1).
Environment getNextEnv(Operation op) {
  EnvFrame updatedFrame;
  if(!op.persistance.save)
    return env;

  updatedFrame = nth_frame(env.frames, op.persistance.frame);
  if(op.persistance.binding == VIA_APPEND) {
    updatedFrame.bindings = append_binding(updatedFrame.bindings, (Binding){UNNAMED, prev});
  } else {
    set_binding(updatedFrame.bindings, op.persistance.binding, (Binding){UNNAMED, prev});
  }
  set_frame(env.frames, op.persistance.frame, updatedFrame);
  return env;
}

Lambda makeLambda(Operation *op, Environment env, int numArgs) {
  return (Lambda){env, 0, op, numArgs};
}

/*
 * loop back updates th`arguments for the function then jumps back to the start of the function.
 * the use of a macro eliminates the tedium of setting all the proper variables, and ensures they're
 * all set correctly.
 * 
 * This is also done via goto, so there's no function call overhead.
 * 
 * It's essentially a manual tail call to self.
 */
#define NEXT_STEP(p, e) opNum++; prev = p; env = e; continue
#define NEW_BLOCK(o, i, p, e) block = o; opNum = i; prev = p; env = e; continue
#define NEXT_STEP_NEW_VAL(v, e) prev = (Value *)gc_alloc(sizeof(Value)); *prev = v; NEXT_STEP(prev, e)

// evaluates a compiled lambda form.
// compiled lambdas exist as cps. 
// maybe there's something cool where we can pre-load all the nexts up to an "if", "funcall" or "pass".
// since all the other operations have a static next op.
Value EvaluateCps(Slice_op suppliedBlock, Environment suppliedEnv, jmp_buf handler) {
  int i, boolVal;
  Environment newEnv, contEnv;
  Value val, *lValue, *rValue;
  EnvFrame newFrame;
  Lambda f, cont;  
  block = suppliedBlock;
  env = suppliedEnv;
  onErr =  &handler;
  prev = NULL;
  int opNum = 0;
  Operation op;


  while(opNum < count(block)) {
    op = ((Operation *)block.p)[opNum];

    clearWip();

    switch(op.type) {
      // build out the new environment for the function call.
      // then evaluate the operation with the new environment.
      //
      // this needs to be optimized to have a seperate arity check and type check for the first arg.
    case funcall:
      f = toLambda(*getLiteralValue(op.val.funcall.f));
      newEnv.frames = f.env.frames;
      newFrame.bindings = new_slice_binding();
      for(i = 0; i < count(op.val.funcall.args) - 1; i++) {
        newFrame.bindings = append_binding(newFrame.bindings, (Binding) {
          UNNAMED,
          getLiteralValue(nth_arg(op.val.funcall.args, i))
        });
      }

      if(opNum + 1 < count(block)) {
        cont = (Lambda){ getNextEnv(op), opNum + 1, block, 1};
        newFrame.bindings = append_binding(newFrame.bindings, (Binding){
          UNNAMED,
          box(&cont, fn)
        });
      }

      newEnv.frames = append_frame(f.env.frames, newFrame);

      NEW_BLOCK(f.ops, f.entryOp, NULL, newEnv);

      

      // load the predicate value. if it's true, then get the consequent continuation,
      // if its false, get the alternate continuation, then 
    case ifOp:
      // an if statement has 2 potential continuations, and a predicate. the predicate is just a plain value,
      // and the consq and alt are operations that execute in the current environment.
      // a predicate is true if it isn't false or null.
      if(getLiteralValue(op.val.ifOp.predicate)->type == boolean)
        boolVal = toBoolean(*getLiteralValue(op.val.ifOp.predicate));
      else 
        boolVal = getLiteralValue(op.val.ifOp.predicate)->type != null;

      NEW_BLOCK(boolVal ? op.val.ifOp.consequent : op.val.ifOp.alternative, 0, prev, env);

      // load the predicate value. If its true, then execute the consequent lambda,
      // providing the next operation as the continuation.
      //
      // if its false, do the same but for the alternate lambda.
      case ifOp:
         if(getLiteralValue(op.val.ifOp.predicate)->type == boolean)
          boolVal = toBoolean(*getLiteralValue(op.val.ifOp.predicate));
        else 
          boolVal = getLiteralValue(op.val.ifOp.predicate)->type != null;

        
      

      // cons conses the two arguments and passes them through the prev into the next operation.
      // the env is updated as well.
      //
      // update so that type checking is done as a seperate step
    case cons:
      NEXT_STEP_NEW_VAL(consFn(getLiteralValue(op.val.cons.first), getLiteralValue(op.val.cons.second)), 
                        getNextEnv(op));

      // takes the car of the argument then passes it through the prev into the next op.
      // also updates the env.
      //
      // update so that type checking is done as a seperate step
    case car:
      NEXT_STEP(carFn(*getLiteralValue(op.val.car.arg)), getNextEnv(op));

      // takes the cdr of the argument then passes it through the prev into the next op.
      // also updates the env.
      //
      // update so that type checking is done as a seperate step
    case cdr:
      NEXT_STEP(cdrFn(*getLiteralValue(op.val.cdr.arg)), getNextEnv(op));

      // takes the argument then passes it through the prev into the next op.
      // also updates the env.
    case quote:
      NEXT_STEP_WITH_NEW_VAL(op.val.quote.val, getNextEnv(op));

      // eq determines if the two arguments are equal and passes the result through the prev into the next operation.
      // the env is updated as well.
    case eq:
      boolVal = structuralEquality(*getLiteralValue(op.val.eq.first), *getLiteralValue(op.val.eq.second));
      NEXT_STEP_WITH_NEW_VAL(box(&boolVal, boolean), getNextEnv(op));
      
      // atom determines if the value passed is not a pair, then passes the result through the prev into the next op.
      // also updates env.
    case atom:
      val = *getLiteralValue(op.val.atom.arg);
      NEXT_STEP_WITH_NEW_VAL(atomFn(val), getNextEnv(op));

      // ???
    case lambda:
      
      // easy
    case define:
      
      // maybe easy
    case set:

    case callcc:
      // similar to a funcall, except call/cc builds a continuation with next op and passes it
      // as the sole argument to the lambda in the first argument position.
      f = toLambda(*getLiteralValue(nth_arg(op.val.funcall.args, 0)));
      newEnv.frames = clone(f.env.frames);
      newFrame.bindings = new_slice_binding();

      if(opNum + 1 == count(block))
        longjmp(*onErr, -6);

      cont.env = getNextEnv(op);

      newFrame.bindings = append_binding(newFrame.bindings, (Binding){ UNNAMED, box(&cont, fn) });

      newEnv.frames = append_frame(newEnv.frames, newFrame);

      NEW_BLOCK(f.ops, f.entryOp, NULL, newEnv);

    case done:
      break;

    default:
      
    }
  }
  
  return *prev;
}
	    
