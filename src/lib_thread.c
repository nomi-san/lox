#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "libs.h"
#include "vm.h"
#include "value.h"

typedef struct {
    vm_t *vm;
    vm_t *main;
    fun_t *routine;
#ifdef _WIN32
    HANDLE handle;
#else
#endif
    bool running;
} thread_t;

#ifdef _WIN32
static DWORD WINAPI thread_routine(void *data)
#else
#endif
{
    thread_t *thread = data;

    vm_call(thread->vm, VAL_OBJ(thread->routine), 0);
    vm_execute(thread->vm);

    return 0;
}

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
    thread_t *thread = malloc(sizeof(thread_t));
    thread->main = vm;
    thread->vm = vm_clone(vm);
    thread->running = false;
    thread->routine = AS_FUN(args[0]);

    vm_push(thread->vm, args[0]);

#ifdef _WIN32
    thread->handle = CreateThread(NULL, 0, thread_routine, thread, CREATE_SUSPENDED, NULL);
#else
#endif

    return VAL_PTR(thread);
}

static val_t thread_exit(vm_t *vm, int argc, val_t *args)
{
#ifdef _WIN32
    ExitThread(0);
#else
#endif

    return VAL_NIL;
}

static val_t thread_start(vm_t *vm, int argc, val_t *args)
{
    thread_t *thread = AS_PTR(args[0]);

    if (!thread->running) {
        for (int i = 0; i < argc; i++) {
            //vm_push(thread->vm, args[i]);
        }

        thread->running = true;

#ifdef _WIN32
        ResumeThread(thread->handle);
#else
#endif
    }

    return VAL_NIL;
}

static val_t thread_join(vm_t *vm, int argc, val_t *args)
{
    thread_t *thread = AS_PTR(args[0]);

    if (thread->running) {
#ifdef _WIN32
        WaitForSingleObject(thread->handle, INFINITE);
#else
#endif
    }

    return VAL_NIL;
}

static val_t thread_cancel(vm_t *vm, int argc, val_t *args)
{
    thread_t *thread = AS_PTR(args[0]);

    if (!thread->running) {
#ifdef _WIN32
        TerminateThread(thread->handle, 0);
#else
#endif
    }

    return VAL_NIL;
}

static val_t thread_close(vm_t *vm, int argc, val_t *args)
{
    thread_t *thread = AS_PTR(args[0]);

    if (thread->running) {
#ifdef _WIN32
        TerminateThread(thread->handle, 0);
#else
#endif
    }

#ifdef _WIN32
    CloseHandle(thread->handle);
#else
#endif

    free(thread->vm);
    free(thread);
    return VAL_NIL;
}

void load_libthread(vm_t *vm)
{
    map_t *thread = map_new(vm, 0, 0);

    map_set(vm, thread, "sleep", VAL_CFN(thread_sleep));
    map_set(vm, thread, "create", VAL_CFN(thread_create));
    map_set(vm, thread, "exit", VAL_CFN(thread_exit));
    map_set(vm, thread, "start", VAL_CFN(thread_start));
    map_set(vm, thread, "join", VAL_CFN(thread_join));
    map_set(vm, thread, "cancel", VAL_CFN(thread_cancel));
    map_set(vm, thread, "close", VAL_CFN(thread_close));

    set_global(vm, "thread", VAL_OBJ(thread));
}
