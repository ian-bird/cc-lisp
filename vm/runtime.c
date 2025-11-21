#include "vm_types.h"
#include <stdlib.h>

Reg box(void *val, ValueType t){
  Value *v = (Value *)gc_alloc(sizeof(Value));
  switch(t){
  case number:
    v->number = *(int *)val;
    break;

  case proc:
  case pair:
    t = err;
    break;

  case symbol:
    v->symbol = *(Symbol *)val;
    break;

  case string:
    v->string = *(char **)val;
    break;

  case boolean:
    v->boolean = *(int *)val;
    break;

  case null:
  case err:
    break;  
  }

  return (Reg){t, v};
}

Reg *lookupSym(Symbol s, Env e) {
  int i = 0;
  for(; i < e.numBindings; i++) {
    if(e.bindings[i].name == s)
      return &e.bindings[i].val;
  }

  return NULL;
}

Reg exec(Machine m) {
  Value *addr;
  int boolVal;
  int expected;
  int numVal;
  Reg *lookedUp;

  while(!(m.pc->type == ret && m.regs[CONT_I].val->proc.code == NULL)) {
    switch(m.pc->type) {
      // jal rX
      //
      // jump to the procedure in rX and
      // save continuation in cont
    case jal:
      addr = (Value *)gc_alloc(sizeof(Value));
      addr->proc.env = m.regs;
      addr->proc.code = m.pc + 1;
      m.regs = m.regs[m.pc->val.jal].val->proc.env;
      m.regs[CONT_I] = (Reg){proc, addr};
      m.pc = m.regs[m.pc->val.jal].val->proc.code;
      break;

      // typChk rX, rY
      // determine if rX and rY have the same type and store
      // the value in the result register.
    case typChk:
      boolVal = m.regs[m.pc->val.typChk.l].type == m.regs[m.pc->val.typChk.r].type;
      m.regs[RESULT_I] = box(&boolVal, boolean);
      m.pc++;
      break;

      // assert rX
      // exit if rX is not true
    case assert:
      if(!m.regs[m.pc->val.assert].val->boolean)
	      return (Reg){err,NULL};
      m.pc++;
      break;

      // arityChk rX, rY
      // if rX is true, then operate in &rest mode. number of args must be >= rY.
      // if rY is false, then number of args must be == rY
      // store resulting boolean in result.
    case arityChk:
      expected = m.regs[m.pc->val.arityChk.r].val->number;
      boolVal = m.regs[m.pc->val.arityChk.l].val->boolean ? expected >= m.numArgs : expected == m.numArgs;
      m.regs[RESULT_I] = box(&boolVal, boolean);
      m.pc++;
      break;
	

      // lookup rX
      // lookup the value associated with the symbol in the global environment.
      //
      // local variables are always stored in registers and should be accessed directly.
      //
      // store the resulting symbol in result, and exit if none found.
    case lookup:
      lookedUp = lookupSym(m.regs[m.pc->val.lookup].val->symbol, *m.globalEnv);
      if(lookedUp == NULL)
      	return (Reg){err, NULL};

      m.regs[RESULT_I] = *lookedUp;
      m.pc++;
      break;

      // push rX
      // pushes the value stored in rX to the argument stack.
      // this is used to pass values to a procedure.
    case push:
      if(m.numArgs == m.argsCap) {
      	m.argsCap *= 2;
      	m.args = (Reg *)realloc(m.args, m.argsCap * sizeof(Reg));
      }
      m.args[m.numArgs] = m.regs[m.pc->val.push];
      m.numArgs++;
      m.pc++;
      break;

      // pop rX
      // pop the topmost value from the argument stack and store
      // the contents in rX.
      // this is used to extract values passed to a procedure.
    case pop:
      m.numArgs--;
      m.regs[m.pc->val.pop] = m.args[m.numArgs];
      m.pc++;
      break;

    case add:
    case sub:
    case mult:
    case divide:
      break;

      // cond rX, rY, rZ
      // if rX is true or not nil, then jump and link to rY, maintaining the current env.
      // if it's false or nil, then same but for rZ.
    case cond:
      if(m.regs[m.pc->val.cond.pred].type == boolean)
      	boolVal = m.regs[m.pc->val.cond.pred].val->boolean;
      else
      	boolVal = m.regs[m.pc->val.cond.pred].type != null;

      m.regs[CONT_I].val->proc.code = m.pc + 1;
      m.pc = boolVal ? m.regs[m.pc->val.cond.consq].val->proc.code : m.regs[m.pc->val.cond.alt].val->proc.code;
      break;

      // cons rX, rY
      // creates a new pair composed of the values pointed to by rX and rY.
      // stores the new value in result.
    case cons:
      addr = (Value *)gc_alloc(sizeof(Value));
      addr->pair.l = m.regs[m.pc->val.cons.l];
      addr->pair.r = m.regs[m.pc->val.cons.r];
      
      m.regs[RESULT_I] = (Reg){pair, addr};
      m.pc++;
      break;

      // car rX
      // takes the left value of the pair in rX, stores in result.
    case car:
      m.regs[RESULT_I] = m.regs[m.pc->val.car].val->pair.l;
      m.pc++;
      break;

      // cdr rX
      // takes the right value of the pair in rY, stores in result.
    case cdr:
      m.regs[RESULT_I] = m.regs[m.pc->val.cdr].val->pair.r;
      m.pc++;
      break;

      // load rX
      // take the value stored in the immediate of the instruction
      // and store in rX
    case load:
      m.regs[m.pc->val.load.l] = m.pc->val.load.v;
      m.pc++;
      break;

    case move:
      m.regs[m.pc->val.move.r] = m.regs[m.pc->val.move.l];
      m.pc++;
      break;

      // cmp rX, rY
      // return #f if types are different, #f if values
      // are different and type isn't number,
      // -1 if the value in rX is smaller than rY
      // 0 if equal,
      // 1 if rX is greater.
    case cmp:
      if (m.regs[m.pc->val.cmp.l].type != m.regs[m.pc->val.cmp.r].type) {
      	boolVal = 0;
	      m.regs[RESULT_I] = box(&boolVal, boolean);
      	m.pc++;
      	break;
      }

      if(m.regs[m.pc->val.cmp.l].type == number) {
      	if(m.regs[m.pc->val.cmp.l].val->number < m.regs[m.pc->val.cmp.r].val->number) {
      	  numVal = -1;
      	} else if(m.regs[m.pc->val.cmp.l].val->number == m.regs[m.pc->val.cmp.r].val->number) {
      	  numVal = 0;
      	} else {
      	  numVal = 1;
    	  }

      	m.regs[RESULT_I] = box(&numVal, number);
      	m.pc++;
      	break;
      }

      boolVal = m.regs[m.pc->val.cmp.l].val == m.regs[m.pc->val.cmp.r].val;
      m.regs[RESULT_I] = box(&boolVal, boolean);
      m.pc++;
      break;

      // set rX, rY
      // goes to the Nth element in the global environment and sets the value to rY.
      // N is the value stored in rX.
    case set:
      m.globalEnv->bindings[m.regs[m.pc->val.set.l].val->number].val = m.regs[m.pc->val.set.r];
      m.pc++;
      break;

      // ret
      // load the environment from the continuation register
      // and jump to its referenced address.
    case ret:
      m.regs = m.regs[CONT_I].val->proc.env;
      m.pc = m.regs[CONT_I].val->proc.code;
      break;
    }
  }
  return m.regs[RESULT_I];
}


