#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "value.h"
#include "parser.h"

vm_t *vm_create()
{
    vm_t *vm = malloc(sizeof(vm_t));

    if (vm != NULL) {
        memset(vm, '\0', sizeof(vm_t));

    }

    return vm;
}

void vm_close(vm_t *vm)
{
    if (vm == NULL) return;

    free(vm);
}

static int execute(vm_t *vm)
{

    return VM_OK;
}

int do_string(vm_t *vm, const char *source)
{
    chunk_t chunk;
    chunk_init(&chunk);

    if (!compile(vm, source, &chunk)) {
        chunk_free(&chunk);
        return VM_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    int result = execute(vm);

    chunk_free(&chunk);
    return result;
}

int do_file(vm_t *file, const char *fname)
{

}
