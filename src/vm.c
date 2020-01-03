#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "value.h"

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
