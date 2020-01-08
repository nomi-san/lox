#include <stdio.h>

#include "vm.h"
#include "libs.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: lox [file]\n");
        return 0;
    }

    vm_t *vm = vm_create();
    int ret = VM_INIT_ERROR;

    if (vm != NULL) {
        load_libmath(vm);
        ret = vm_dofile(vm, argv[argc - 1]);
        vm_close(vm);
    }

    return ret;
}
