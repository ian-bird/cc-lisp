#include "vm_types.h"
#include "runtime.h"
#include "globals.h"

#include <stdlib.h>
#include <stdio.h>


void impl() {
    Function *toRun = (Function *)malloc(2048);

    toRun->argNames = (int *)malloc(sizeof(int));
    toRun->argNames[0] = 1;
    toRun->numFixedArgs = 1;
    toRun->hasFinalVarArg = 0;

    toRun->funEnv = (Environment *)malloc(sizeof(Environment) + sizeof(Frame));
    toRun->funEnv->numFrames = 1;
    toRun->funEnv->frames[0] = (Frame *)malloc(sizeof(Frame));
    toRun->funEnv->frames[0]->bindings = (Binding *)malloc(sizeof(Binding) * 2);
    toRun->funEnv->frames[0]->capacity = 2;
    toRun->funEnv->frames[0]->numBindings = 0;

    toRun->varEnv = (Environment *)malloc(sizeof(Environment) + sizeof(Frame));
    toRun->varEnv->numFrames = 1;
    toRun->varEnv->frames[0] = (Frame *)malloc(sizeof(Frame));
    toRun->varEnv->frames[0]->bindings = (Binding *)malloc(sizeof(Binding) * 2);
    toRun->varEnv->frames[0]->capacity = 2;
    toRun->varEnv->frames[0]->numBindings = 0;

    toRun->instructions[0].type = ret;

    run(NULL, 0, toRun);
}

int main()
{
    if(setjmp(onErr) > 0)
    {
        fprintf(stderr, "encountered error on evaluation.\n");
        exit(1);
    }

    impl();
    return 0;
}
