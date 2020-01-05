#include <stdio.h>
#include <time.h>
#include "vm.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: lox [file]\n");
        return 0;
    }

    vm_t *vm = vm_create();
    int ret = VM_INIT_ERROR;

    if (vm != NULL) {
        clock_t clk = clock();
        ret = do_file(vm, argv[argc - 1]);
        printf("\ndone in %.3gs\n", (clock()-clk)/(double)CLOCKS_PER_SEC);
        vm_close(vm);
    }

    return ret;
}
