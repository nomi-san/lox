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

#define PUSH(vm, v)    *((vm)->top++) = (v)
#define POP(vm)        *(--(vm)->top)

static int execute(vm_t *vm)
{

#define READ_BYTE()     *(vm->ip++)
#define READ_SHORT()    (vm->ip += 2, (uint16_t)((vm->ip[-2] << 8) | vm->ip[-1]))

#define READ_CONST()    (vm)->chunk->constants.values[READ_BYTE()]
#define READ_CONSTL()   (vm)->chunk->constants.values[READ_SHORT()]

#define INTERPRET       _loop: switch(READ_BYTE())
#define CODE(x)         case OP_##x:
#define NEXT            goto _loop

    INTERPRET
    {
        CODE(NIL) {
            PUSH(vm, VAL_NIL);
            NEXT;
        }
        CODE(TRUE) {
            PUSH(vm, VAL_TRUE);
            NEXT;
        }
        CODE(FALSE) {
            PUSH(vm, VAL_FALSE);
            NEXT;
        }
        CODE(CONST) {
            PUSH(vm, READ_CONST());
            NEXT;
        }
        CODE(CONSTL) {
            PUSH(vm, READ_CONSTL());
            NEXT;
        }
        CODE(RET) {
            
            return VM_OK;
        }

        CODE(NOT) {
            PUSH(vm, VAL_BOOL(IS_FALSEY(POP(vm))));
            NEXT;
        }

        CODE(EQ) {
            val_t b = POP(vm);
            val_t a = POP(vm);
            PUSH(vm, VAL_BOOL(val_equal(a, b)));
            NEXT;
        }
    }

    return VM_OK;
}

int do_string(vm_t *vm, const char *source)
{
    chunk_t chunk;
    chunk_init(&chunk);

    if (!compile(vm, NULL, source, &chunk)) {
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
