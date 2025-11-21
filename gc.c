#include "gc.h"
#include "lisp_globals.h"

#define POOL_SIZE 4096

#define UNINITED -1

typedef struct {
  size_t amount;
  void *addr;
} Record;

typedef struct {
  int numRecords;
  int recordTableSize;
  Record *records;
} RecordTable;

typedef struct {
  int bumpOffset;
  RecordTable bookKeeping;
  void *mem;
} Pool;

Pool poolA, poolB;
int activePool = UNINITED;

#define GET_POOL activePool == 0 ? &poolA : &poolB

// sets the initial config for a pool
Pool initPool() {
  Pool result ;
  result.mem = malloc(POOL_SIZE);
  result.bookKeeping.numRecords = 0;
  result.bumpOffset = 0;
  result.bookKeeping.records = (Record *)malloc(sizeof(Record) * 8);
  result.bookKeeping.recordTableSize = 8;
  result.bookKeeping.numRecords = 0;

  return result;
}

void free_all() {
  activePool = UNINITED;
  free(poolA.bookKeeping.records);
  free(poolB.bookKeeping.records);

  free(poolA.mem);
  free(poolB.mem);
}

// resets offsets for a pool so it can be filled up again
void resetPool(Pool *p) {
  p->bumpOffset = 0;
  p->bookKeeping.numRecords = 0;
}

// try to allocate memory in the current block.
// if we can't, then call garbage collection and attempt again.
void *gc_alloc(size_t s) {
  Pool *p;

  if(activePool == UNINITED) {
    activePool = 0;
    poolA = initPool();
    poolB = initPool();
  }

  p = GET_POOL;

  if(p->bumpOffset + s > POOL_SIZE) {
    gc_collect();
    return gc_alloc(s);
  }

  return bumpAlloc(s, p);
}

// given an amount of memory and a pool, bump allocate from the pool,
// returning the pointer, and doing the associated book-keeping.
void *bumpAlloc(size_t s, Pool *p) {
  void *result = (char *)p->mem + p->bumpOffset;
  
  // if record table is full, double its size
  if(p->bookKeeping.numRecords == p->bookKeeping.recordTableSize) {
    p->bookKeeping.recordTableSize *= 2;
    p->bookKeeping.records = (Record *)realloc(p->bookKeeping.records,
						   sizeof(Record) * p->bookKeeping.recordTableSize);
  }
  
  // update records and bump pointer
  p->bookKeeping.records[p->bookKeeping.numRecords] = (Record){ result, s };
  p->bookKeeping.numRecords += 1;
  p->bumpOffset += s;
  return result;
}

void collect() {
  Pool *oldPool = GET_POOL;
  Pool *newPool = activePool == 0 ? &poolB : &poolA;

  void **mapDict = (void **)calloc(oldPool->bookKeeping.numRecords, sizeof(void *));

  // move over wip variables
  int i = 0;
  for(; i < numWip; i++) {
    migratePointer(wip + i, *oldPool, newPool, mapDict);
  }
    
  // prev is saved
  migrateValue(&prev, *oldPool, newPool, mapDict);

  // anuthing in the env is saved
  migrateEnv(&env, *oldPool, newPool, mapDict);

  // also need to migrate block over

  //after moving over all the saved data, reset the pointers
  // on the old table, and switch which pool is active.
  resetPool(oldPool);
  activePool == 0 ? 1 : 0;
  free(mapDict);
}

// these are helper macros to make setting up the structure migration less tedious
#define WITH_ARGS(fn, what) fn(what, from, to, remapped)
#define WITH_ARGS_P(fn, what) \
 WITH_ARGS(migratePointer, &(what)); \
 WITH_ARGS(fn, what)

#define ENUM_FAIL \
  free(remapped); \
  longjmp(*onErr, -3);
// this needs to recursively walk the value and move all pointers to living data
// over to the new table.
void migrateValue(Value *v, Pool from, Pool *to, void **remapped) {
  int i;
  if(v == NULL)
    return;

  WITH_ARGS(migratePointer, v);
  
  switch(v->type) {
  case null:
  case number:
  case symbol:
  case boolean:
    return;
  
  case string:
    WITH_ARGS(migratePointer, &(v->val.string));
    return;

  case pair:
    WITH_ARGS_P(migrateValue, v->val.pair.left);
    WITH_ARGS_P(migrateValue, v->val.pair.right);
    return;
  
  // fix this
  case fn:
    WITH_ARGS_P(migrateOperation, v->val.fn.op);
    WITH_ARGS(migrateEnv, &(v->val.fn.env));
    WITH_ARGS(migratePointer, &(v->val.fn.argNames.p));

  default:
    ENUM_FAIL
  }
}

void migrateEnv(Environment *env, Pool from, Pool *to, void **remapped) {
  int i, j;
  if(env == NULL)
    return;

  for(i = 0; i < count(env->frames); i++) {
    for(j = 0; j < count((nth_frame(env->frames, i)).bindings); j++) {
      WITH_ARGS_P(migrateValue, ((Binding *)((EnvFrame *)env->frames.p)[i].bindings.p)[j].v);
    }
    WITH_ARGS(migratePointer, &(((EnvFrame *)env->frames.p)[i].bindings.p));
  }

  WITH_ARGS(migratePointer, &(env->frames.p));
}

// more helper macros to reduce tediousness
#define MIGRATE_2_ARGS(field) \
  case field: \
   WITH_ARGS(migrateArgument, &(op->val.field.first)); \
   WITH_ARGS(migrateArgument, &(op->val.field.second)); \
   return

#define MIGRATE_1_ARG(field) \
case field: \
  WITH_ARGS(migrateArgument, &(op->val.field.arg)); \
  return

// migrates an operation over to the new pool
void migrateOperation(Operation *op, Pool from, Pool *to, void **remapped) {
  int i;
  if(op == NULL)
    return;

  switch(op->type) {
  case funcall:
    WITH_ARGS(migratePointer, &(op->val.funcall.args.p));
    for(i = 0; i < count(op->val.funcall.args); i++) {
      WITH_ARGS(migrateArgument, ((Argument *)op->val.funcall.args.p) + i);
    }
    return;

  case ifOp:
    WITH_ARGS(migrateArgument, &(op->val.ifOp.predicate));
    // fix these
    WITH_ARGS_P(migrateOperation, op->val.ifOp.consequent);
    WITH_ARGS_P(migrateOperation, op->val.ifOp.alternative);
    return;

  MIGRATE_2_ARGS(cons);
  MIGRATE_2_ARGS(eq);
  MIGRATE_1_ARG(car);
  MIGRATE_1_ARG(cdr);
  MIGRATE_1_ARG(atom);

  case quote:
    WITH_ARGS(migrateValue, &(op->val.quote.val));
    return;

  default:
    ENUM_FAIL
  }

  WITH_ARGS(migratePointer, &(op->indicesToDrop));
}

// migrates an argument's pointer data over from the old pool to the new one.
void migrateArgument(Argument *arg, Pool from, Pool *to, void **remapped) {
  if(arg == NULL)
    return;

  switch(arg->type) {
  case envRef:
  case lexRef:
  case prevRef:
    return;

  case literal:
    WITH_ARGS(migrateValue, &(arg->val.literal));
    return;

  default:
    ENUM_FAIL
  }
}

// we receive the new table to update into.
// the one we're moving from is the global table;
void migratePointer(void **p, Pool from, Pool *to, void **remapped) {
  int i;
  if(*p == NULL)
    return;

  for(i = 0; i < from.bookKeeping.numRecords; i++) {
    // found the record for the address we're migrating
    if(from.bookKeeping.records[i].addr == *p) {
      // if it's already been moved, update pointer to use the new address and done.
      if(remapped[i] != NULL) {
	      *p = remapped[i];
        return;
      }

      // otherwise, allocate space in the new pool for it, set the pointer to use the
      // newly allocated address, copy the data over, store the new address in the remapped
      // table, and then done.
      *p = bump_alloc(from.bookKeeping.records[i].amount, to);
      memcpy(p, from.bookKeeping.records[i].addr, from.bookKeeping.records[i].amount);
      remapped[i] = *p;
      return;
    }
  }
}

void registerWip(void *p) {
  if(wip == NULL) {
    wip = (void **)malloc(sizeof(void *) * 8);
    wipCapacity = 8;
  }
  if(numWip == wipCapacity) {
    wipCapacity *= 2;
    wip = (void **)realloc(wip, sizeof(void *) * wipCapacity);
  }

  wip[numWip] = p;
  numWip++;
}

void clearWip() {
  free(wip);
  wip = NULL;
  wipCapacity = 0;
  numWip = 0;
}