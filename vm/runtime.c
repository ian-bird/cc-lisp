

// supported instructions:
// check arity, check type, funcall, lambda, set! 


(define (any? proc seq)
  (call/cc (lambda (exit)
             (for-each (lambda (e) (when (proc e) (exit #t))) seq)
             #f)))

(define (any? proc seq k)
  (call/cc (lambda (exit k) ; exit and k are the same thing, it jsut is easier to parse this way.
             (make-closure (lambda (e k) 
                             (proc e (lambda (p)
                                       (if p
                                            (lambda (k) (funcall exit #t k))
                                            (lambda (k) (identity '()' k))
                                            k))))
                            (lambda (iter)
                              (for-each iter seq (lambda (_) (identity #f k))))))))

                              
(function){
  globalFns,
  globalVars,
  2,
  FALSE,
  NULL,
  {
    r0 = load(l1)
    call-cc(r0)
    ret
  }
}

(function){
  {globalFns, {}},
  {globalVars, {proc seq}},
  1,
  FALSE,
  NULL,
  {
    r-1 = load(for-each)
    r0 = fn-env-lookup(r-1) ; each rX = Y expands to [Y; move(result, rX)]
    r0.5 = load(l2)
    r1 = make-closure(r0.5)
    r1.5 = load(seq)
    r2 = var-lookup(r1.5)
    funcall(r0, r1, r2) ; this expands out to add-arging r1 and r2, making an arity check, then jump and linking to r0.
    r4 = load(identity)
    r5 = fn-lookup(r4)
    r6 = load(#f)
    funcall(r4, r5) ; this will be detected as a tail call.
    ret
  }
}

(function){
  {globalFns, {}, {}},
  {globalVars, {proc seq} {exit}},
  1,
  FALSE,
  NULL,
  {
    r0 = var-lookup(proc)
    unless r0 b
    r1 = load(funcall)
    r2 = fn-lookup(funcall)
    r3 = load(exit)
    r4 = var-lookup(r1)
    r5 = load(#f)
    funcall(r2, r4, r5)
    goto c
    b: load('()')
       goto c
    c: ret
  }
}

Value run(Machine m, Value *initialArgs, int numInitialArgs, Function toExecute) {
  while(!(m.s.numFrames == 0 && m.ip->type == ret)) {
    switch(m.ip->type) {
    case varLookup:
      // get the current frame
      // get the variable environment
      // get the value out of the register
      // lookup the variable
    case fnLookup:
      // get the current frame
      // get the function environment
      // get the value of the register
      // lookup the function
    case ret:
      // load the result value, pop the stack frame, load the return address.
    case load:
      m.result = m->ip.val.load;
    case move:
      m.registers[m->ip.val.move] = m.result;
    case addArg:
      // add the register value to the argument stack to push to a function.
    case arity:
      // check the length of the arg stack versus the function, error out if invalid.
    case gosub:
      // load the function
    case unless:
      // if the predicate is false, move the instruction pointer to the new address
    case jmp:
      // load the current frame
      // get the pointer for the entry point
      // set the instruction pointer to the entry point + the value in the jump instruction.
    default:
      longjmp(*onErr, UNRECOGNIZED_INSTRUCTION);
    }
  }

  return m.result;
}