#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "value.h"
#include "parser.h"
#include "object.h"

static void resetStack(vm_t *vm)
{
    vm->top = vm->stack;
}

static void runtimeError(vm_t *vm, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->ip - vm->chunk->code;
    int line = vm->chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    resetStack(vm);
}

vm_t *vm_create()
{
    vm_t *vm = malloc(sizeof(vm_t));

    if (vm != NULL) {
        memset(vm, '\0', sizeof(vm_t));

        gc_init(&vm->gc);
        tab_init(&vm->strings);

        resetStack(vm);
    }

    return vm;
}

void vm_close(vm_t *vm)
{
    if (vm == NULL) return;

    tab_free(&vm->strings);
    gc_free(&vm->gc);

    free(vm);
}

#define PUSH(v)     *((vm)->top++) = (v)
#define POP()       *(--(vm)->top)
#define PEEK(i)     ((vm)->top[-1 - (i)])

static void concatenate(vm_t *vm)
{
    str_t *b = AS_STR(POP());
    str_t *a = AS_STR(POP());

    int length = a->length + b->length;
    char *chars = malloc((length + 1) * sizeof(char));
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    str_t *result = str_take(vm, chars, length);
    PUSH(VAL_OBJ(result));
}

static int execute(vm_t *vm)
{

#define PREV_BYTE()     (vm->ip[-1])
#define READ_BYTE()     *(vm->ip++)
#define READ_SHORT()    (vm->ip += 2, (uint16_t)((vm->ip[-2] << 8) | vm->ip[-1]))

#define READ_CONST()    (vm)->chunk->constants.values[READ_BYTE()]
#define READ_CONSTL()   (vm)->chunk->constants.values[READ_SHORT()]

#define ERROR(fmt, ...) do { runtimeError(vm, fmt, ##__VA_ARGS__); return VM_RUNTIME_ERROR; } while (0)

#define INTERPRET       _loop: switch(READ_BYTE())
#define CODE(x)         case OP_##x:
#define CODE_ERR()      default:
#define NEXT            goto _loop

    INTERPRET
    {
        CODE(NIL) {
            PUSH(VAL_NIL);
            NEXT;
        }
        CODE(TRUE) {
            PUSH(VAL_TRUE);
            NEXT;
        }
        CODE(FALSE) {
            PUSH(VAL_FALSE);
            NEXT;
        }
        CODE(CONST) {
            PUSH(READ_CONST());
            NEXT;
        }
        CODE(CONSTL) {
            PUSH(READ_CONSTL());
            NEXT;
        }
        CODE(RET) {
            
            return VM_OK;
        }

        CODE(NOT) {
            PUSH(VAL_BOOL(IS_FALSEY(POP())));
            NEXT;
        }

        CODE(NEG) {
            switch (AS_TYPE(PEEK(0))) {
                case VT_BOOL:
                    PUSH(VAL_NUM((double)AS_BOOL(POP())));
                    break;
                case VT_NUM:
                    PUSH(VAL_NUM(-AS_NUM(POP())));
                    break;
                default:
                    ERROR("Operands must be a number or boolean.");
            }
            NEXT;
        }

        CODE(EQ) {
            val_t b = POP();
            val_t a = POP();
            PUSH(VAL_BOOL(val_equal(a, b)));
            NEXT;
        }

        CODE(LT) {
            if (IS_NUM(PEEK(0)) && IS_NUM(PEEK(1))) {
                double b = AS_NUM(POP());
                double a = AS_NUM(POP());
                PUSH(VAL_NUM(a < b));
            } else {
                ERROR("Operands must be two numbers.");                             
            }
            NEXT;
        }

        CODE(LE) {
            if (IS_NUM(PEEK(0)) && IS_NUM(PEEK(1))) {
                double b = AS_NUM(POP());
                double a = AS_NUM(POP());
                PUSH(VAL_NUM(a <= b));
            } else {
                ERROR("Operands must be two numbers.");                             
            }
            NEXT;
        }

        CODE(ADD) {
            if (IS_STR(PEEK(0)) && IS_STR(PEEK(1))) {                
                concatenate(vm);                                               
            } else if (IS_NUM(PEEK(0)) && IS_NUM(PEEK(1))) {
                double b = AS_NUM(POP());                                 
                double a = AS_NUM(POP());                                 
                PUSH(VAL_NUM(a + b));                                     
            } else {
                ERROR("Operands must be two numbers or two strings.");                             
            }
            NEXT;
        }

        CODE(SUB) {
            if (IS_NUM(PEEK(0)) && IS_NUM(PEEK(1))) {
                double b = AS_NUM(POP());
                double a = AS_NUM(POP());
                PUSH(VAL_NUM(a - b));
            } else {
                ERROR("Operands must be two numbers.");                             
            }
            NEXT;
        }

        CODE(MUL) {
            if (IS_NUM(PEEK(0)) && IS_NUM(PEEK(1))) {
                double b = AS_NUM(POP());
                double a = AS_NUM(POP());
                PUSH(VAL_NUM(a * b));
            } else {
                ERROR("Operands must be two numbers.");                             
            }
            NEXT;
        }

        CODE(DIV) {
            if (IS_NUM(PEEK(0)) && IS_NUM(PEEK(1))) {
                double b = AS_NUM(POP());
                double a = AS_NUM(POP());
                PUSH(VAL_NUM(a / b));
            } else {
                ERROR("Operands must be two numbers.");                             
            }
            NEXT;
        }

        CODE_ERR() {
            ERROR("Bad opcode, got %d!", PREV_BYTE());
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

    return VM_OK;
}
