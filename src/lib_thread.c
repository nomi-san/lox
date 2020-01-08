#include <stdlib.h>

#include "libs.h"
#include "vm.h"
#include "value.h"

#ifdef _WIN32
__declspec(dllimport) void __stdcall Sleep(unsigned long);
#else
#include <unistd.h>
#endif

static val_t thread_sleep(vm_t *vm, int argc, val_t *args)
{
    int ms = AS_INT(args[0]);

#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif

    return VAL_NIL;
}

static val_t thread_create(vm_t *vm, int argc, val_t *args)
{

    return VAL_NIL;
}

static val_t thread_exit(vm_t *vm, int argc, val_t *args)
{

    return VAL_NIL;
}

static val_t thread_join(vm_t *vm, int argc, val_t *args)
{

    return VAL_NIL;
}

static val_t thread_cancel(vm_t *vm, int argc, val_t *args)
{

    return VAL_NIL;
}

void load_libthread(vm_t *vm)
{
    map_t *thread = map_new(vm, 0, 0);

    map_set(vm, thread, "sleep", VAL_CFN(thread_sleep));

    set_global(vm, "thread", VAL_OBJ(thread));
}
