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
    vm->frameCount = 0;
}

static void runtimeError(vm_t *vm, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm->frameCount - 1; i >= 0; i--) {
        frame_t *frame = &vm->frames[i];
        fun_t *function = frame->function;
        // -1 because the IP is sitting on the next instruction to be
        // executed.                                                 
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = CHUNK_GETLN(&frame->function->chunk, instruction);
        int column = CHUNK_GETCOL(&frame->function->chunk, instruction);
        fprintf(stderr, "[%d:%d] in ", line, column);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        }
        else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack(vm);
}

vm_t *vm_create()
{
    vm_t *vm = malloc(sizeof(vm_t));
    if (vm == NULL) return NULL;

    memset(vm, '\0', sizeof(vm_t));

    gc_init(&vm->gc);
    tab_init(&vm->globals);
    tab_init(&vm->strings);

    resetStack(vm);
    return vm;
}

void vm_close(vm_t *vm)
{
    if (vm == NULL) return;

    tab_free(&vm->globals);
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

static bool call(vm_t *vm, fun_t *function, int argCount)
{
    if (argCount != function->arity) {
        runtimeError(vm, "Expected %d arguments but got %d.",
            function->arity, argCount);
        return false;
    }

    if (vm->frameCount == FRAMES_MAX) {
        runtimeError(vm, "Stack overflow.");
        return false;
    }

    frame_t *frame = &vm->frames[vm->frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;

    frame->slots = vm->top - argCount - 1;
    return true;
}

static bool callValue(vm_t *vm, val_t callee, int argCount)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OT_FUN:
                return call(vm, AS_FUN(callee), argCount);

            default:
                // Non-callable object type.                   
                break;
        }
    }

    runtimeError(vm, "Can only call functions and classes.");
    return false;
}

static int execute(vm_t *vm)
{
    register uint8_t *ip;
    register frame_t *frame;

#define STORE_FRAME() \
    frame->ip = ip

#define LOAD_FRAME() \
    frame = &vm->frames[vm->frameCount - 1]; \
	ip = frame->ip

#define STACK           (frame->slots)

#define PREV_BYTE()     (ip[-1])
#define READ_BYTE()     *(ip++)
#define READ_SHORT()    (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

#define READ_CONST()    frame->function->chunk.constants.values[READ_BYTE()]
#define READ_CONSTL()   frame->function->chunk.constants.values[READ_SHORT()]
#define READ_STR()      AS_STR(READ_CONST())
#define READ_STRL()     AS_STR(READ_CONSTL())

#define ERROR(fmt, ...) \
    do { \
        STORE_FRAME(); \
        runtimeError(vm, fmt, ##__VA_ARGS__); \
        return VM_RUNTIME_ERROR; \
    } while (0)

#define INTERPRET       _loop: switch(READ_BYTE())
#define CODE(x)         case OP_##x:
#define CODE_ERR()      default:
#define NEXT            goto _loop

    LOAD_FRAME();

    INTERPRET
    {
        CODE(PRINT) {
            val_print(POP());
            printf("\n");
            NEXT;
        }

        CODE(POP) {
            POP();
            NEXT;
        }

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

        CODE(CALL) {
            int argCount = READ_BYTE();
            STORE_FRAME();
            if (!callValue(vm, PEEK(argCount), argCount)) {
                return VM_RUNTIME_ERROR;
            }
            LOAD_FRAME();
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
                    ERROR("Operands must be a number/boolean.");
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
            switch (CMB_BYTES(AS_TYPE(PEEK(1)), AS_TYPE(PEEK(0)))) {
                case VT_NUM_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_BOOL(a < b));
                    break;
                }
                case VT_BOOL_BOOL: {
                    char b = AS_BOOL(POP());
                    char a = AS_BOOL(POP());
                    PUSH(VAL_BOOL(a < b));
                    break;
                }
                case VT_BOOL_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_BOOL(POP());
                    PUSH(VAL_BOOL(a < b));
                    break;
                }
                case VT_NUM_BOOL: {
                    double b = AS_BOOL(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_BOOL(a < b));
                    break;
                }
                default:
                    ERROR("Operands must be two numbers/booleans.");
            }
            NEXT;
        }

        CODE(LE) {
            switch (CMB_BYTES(AS_TYPE(PEEK(1)), AS_TYPE(PEEK(0)))) {
                case VT_NUM_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_BOOL(a <= b));
                    break;
                }
                case VT_BOOL_BOOL: {
                    char b = AS_BOOL(POP());
                    char a = AS_BOOL(POP());
                    PUSH(VAL_BOOL(a <= b));
                    break;
                }
                case VT_BOOL_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_BOOL(POP());
                    PUSH(VAL_BOOL(a <= b));
                    break;
                }
                case VT_NUM_BOOL: {
                    double b = AS_BOOL(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_BOOL(a <= b));
                    break;
                }
                default:
                    ERROR("Operands must be two numbers/booleans.");
            }
            NEXT;
        }

        CODE(ADD) {
            switch (CMB_BYTES(AS_TYPE(PEEK(1)), AS_TYPE(PEEK(0)))) {
                case VT_NUM_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a + b));
                    break;
                }
                case VT_BOOL_BOOL: {
                    char b = AS_BOOL(POP());
                    char a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a + b));
                    break;
                }
                case VT_BOOL_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a + b));
                    break;
                }
                case VT_NUM_BOOL: {
                    double b = AS_BOOL(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a + b));
                    break;
                }
                case VT_OBJ_OBJ:
                    if (IS_STR(PEEK(0)) && IS_STR(PEEK(1))) {
                        concatenate(vm);
                        break;
                    }
                default:
                    ERROR("Operands must be two numbers/booleans/strings.");
            }
            NEXT;
        }

        CODE(SUB) {
            switch (CMB_BYTES(AS_TYPE(PEEK(1)), AS_TYPE(PEEK(0)))) {
                case VT_NUM_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a - b));
                    break;
                }
                case VT_BOOL_BOOL: {
                    char b = AS_BOOL(POP());
                    char a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a - b));
                    break;
                }
                case VT_BOOL_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a - b));
                    break;
                }
                case VT_NUM_BOOL: {
                    double b = AS_BOOL(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a - b));
                    break;
                }
                default:
                    ERROR("Operands must be two numbers/booleans.");
            }
            NEXT;
        }

        CODE(MUL) {
            switch (CMB_BYTES(AS_TYPE(PEEK(1)), AS_TYPE(PEEK(0)))) {
                case VT_NUM_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a * b));
                    break;
                }
                case VT_BOOL_BOOL: {
                    char b = AS_BOOL(POP());
                    char a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a * b));
                    break;
                }
                case VT_BOOL_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a * b));
                    break;
                }
                case VT_NUM_BOOL: {
                    double b = AS_BOOL(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a * b));
                    break;
                }
                default:
                    ERROR("Operands must be two numbers/booleans.");
            }
            NEXT;
        }

        CODE(DIV) {
            switch (CMB_BYTES(AS_TYPE(PEEK(1)), AS_TYPE(PEEK(0)))) {
                case VT_NUM_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a / b));
                    break;
                }
                case VT_BOOL_BOOL: {
                    double b = AS_BOOL(POP());
                    double a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a / b));
                    break;
                }
                case VT_BOOL_NUM: {
                    double b = AS_NUM(POP());
                    double a = AS_BOOL(POP());
                    PUSH(VAL_NUM(a / b));
                    break;
                }
                case VT_NUM_BOOL: {
                    double b = AS_BOOL(POP());
                    double a = AS_NUM(POP());
                    PUSH(VAL_NUM(a / b));
                    break;
                }
                default:
                    ERROR("Operands must be two numbers/booleans.");
            }
            NEXT;
        }

        CODE(DEFL) {
            str_t *name = READ_STRL();
            goto _def;
        CODE(DEF)
            name = READ_STR();
        _def:
            tab_set(&vm->globals, name, PEEK(0));
            POP();
            NEXT;
        }

        CODE(GLDL) {
            str_t *name = READ_STRL();
            val_t value;
            goto _gld;
        CODE(GLD)
            name = READ_STR();
        _gld:            
            if (!tab_get(&vm->globals, name, &value)) {
                ERROR("Undefined variable '%s'.", name->chars);
            }
            PUSH(value);
            NEXT;
        }

        CODE(GSTL) {
            str_t *name = READ_STRL();
            goto _gst;
        CODE(GST)
            name = READ_STR();
        _gst:
            if (tab_set(&vm->globals, name, PEEK(0))) {
                tab_remove(&vm->globals, name);
                ERROR("Undefined variable '%s'.", name->chars);
            }
            NEXT;
        }

        CODE(LD) {
            PUSH(STACK[READ_BYTE()]);
            NEXT;
        }

        CODE(ST) {
            STACK[READ_BYTE()] = PEEK(0);
            NEXT;
        }

        CODE(JMP) {
            uint16_t offset = READ_SHORT();
            ip += offset;
            NEXT;
        }

        CODE(JMPF) {
            uint16_t offset = READ_SHORT();
            if (IS_FALSEY(PEEK(0))) ip += offset;
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
    fun_t *function = compile(vm, NULL, source);
    if (function == NULL) return VM_COMPILE_ERROR;

    val_t script = VAL_OBJ(function);

    PUSH(script);
    callValue(vm, script, 0);

    return execute(vm);
}

int do_file(vm_t *vm, const char *fname)
{
    char *source = read_file(fname, NULL);
    if (source == NULL) return VM_COMPILE_ERROR;

    int result = do_string(vm, source);

    free(source);
    return result;
}
