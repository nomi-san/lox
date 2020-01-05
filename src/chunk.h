#pragma once

#include "common.h"
#include "value.h"

#define OPCODES() \
/*        opcodes      args     stack       description */ \
    _CODE(PRINT)   	/* []       [-1, +0]    pop a value from stack */ \
    _CODE(POP)     	/* []       [-1, +0]    pop a value from stack and print it */ \
    _CODE(CALL)    	/* [n]      [-n, +1]    */ \
    _CODE(RET)     	/* []       [-1, +0]    */ \
    _CODE(NIL)     	/* []       [-0, +1]    push nil to stack */ \
    _CODE(TRUE)    	/* []       [-0, +1]    push true to stack */ \
    _CODE(FALSE)   	/* []       [-0, +1]    push false to stack */ \
    _CODE(CONST)   	/* [k]      [-0, +1]    push a constant from (k) to stack */ \
    _CODE(CONSTL)  	/* [k, k]   [-0, +1]    */ \
    _CODE(NEG)     	/* []       [-1, +1]    */ \
    _CODE(NOT)     	/* []       [-1, +1]    */ \
    _CODE(LT)      	/* []       [-1, +1]    */ \
    _CODE(LE)      	/* []       [-1, +1]    */ \
    _CODE(EQ)      	/* []       [-1, +1]    */ \
    _CODE(ADD)     	/* []       [-2, +1]    */ \
    _CODE(SUB)     	/* []       [-2, +1]    */ \
    _CODE(MUL)     	/* []       [-2, +1]    */ \
    _CODE(DIV)     	/* []       [-2, +1]    */ \
    _CODE(DEF)     	/* [k]      [-1, +0]    pop a value from stack and define as (k) in global */ \
    _CODE(GLD)     	/* [k]      [-0, +1]    push a from (k) in global to stack */ \
    _CODE(GST)     	/* [k]      [-0, +0]    set a value from stack as (k) in global */ \
    _CODE(DEFL)    	/* [k, k]   [-1, +0]    */ \
    _CODE(GLDL)    	/* [k, k]   [-0, +1]    */ \
    _CODE(GSTL)    	/* [k, k]   [-0, +0]    */ \
    _CODE(JMP)     	/* [s, s]   [-0, +0]    */ \
    _CODE(JMPF)    	/* [s, s]   [-1, +0]    */ \
    _CODE(LD)      	/* [s]      [-0, +1]    */ \
    _CODE(LD0)     	/* []       [-0, +1]    */ \
    _CODE(LD1)     	/* []       [-0, +1]    */ \
    _CODE(LD2)     	/* []       [-0, +1]    */ \
    _CODE(LD3)     	/* []       [-0, +1]    */ \
    _CODE(LD4)     	/* []       [-0, +1]    */ \
    _CODE(LD5)     	/* []       [-0, +1]    */ \
    _CODE(LD6)     	/* []       [-0, +1]    */ \
    _CODE(LD7)     	/* []       [-0, +1]    */ \
    _CODE(LD8)     	/* []       [-0, +1]    */ \
    _CODE(ST)      	/* [s]      [-0, +0]    */

#define _CODE(x)    OP_##x,
typedef enum { OPCODES() } opcode_t;
#undef _CODE

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    uint32_t *lines;
    arr_t constants;
} chunk_t;

void chunk_init(chunk_t *chunk);
void chunk_free(chunk_t *chunk);
void chunk_emit(chunk_t *chunk, uint8_t byte, int ln, int col);

#define CHUNK_CODEPAGE      256
#define CHUNK_GETLN(c, i)   (((c)->lines)[i] >> 16 & 0xFFFF)
#define CHUNK_GETCOL(c, i)  (((c)->lines)[i] & 0xFFFF)

static const char *opcode_tostr(opcode_t opcode) {
#define _CODE(x) #x,
    static const char *tab[] = { OPCODES() };
    return tab[opcode];
#undef _CODE
}
