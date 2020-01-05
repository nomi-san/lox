#pragma once

#include "common.h"
#include "value.h"

typedef enum {
//  opcodes        args      stack      description //
    OP_PRINT,   /* []       [-1, +0]    pop a value from stack */
    OP_POP,     /* []       [-1, +0]    pop a value from stack and print it */
    OP_CALL,    /* not implemented yet */
    OP_RET,     /* not implemented yet */
    OP_NIL,     /* []       [-0, +1]    push nil to stack */
    OP_TRUE,    /* []       [-0, +1]    push true to stack */
    OP_FALSE,   /* []       [-0, +1]    push false value to stack */
    OP_CONST,   /* [k]      [-0, +1]    push a constant from (k) to stack */
    OP_CONSTL,  /* [k, k]   [-0, +1]    */
    OP_NEG,     /* []       [-1, +1]    */
    OP_NOT,     /* []       [-1, +1]    */
    OP_LT,      /* []       [-1, +1]    */
    OP_LE,      /* []       [-1, +1]    */
    OP_EQ,      /* []       [-1, +1]    */
    OP_ADD,     /* []       [-2, +1]    */
    OP_SUB,     /* []       [-2, +1]    */
    OP_MUL,     /* []       [-2, +1]    */
    OP_DIV,     /* []       [-2, +1]    */
    OP_DEF,     /* [k]      [-1, +0]    pop a value from stack and define as (k) in global */
    OP_GLD,     /* [k]      [-0, +1]    push a from (k) in global to stack */
    OP_GST,     /* [k]      [-0, +0]    set a value from stack as (k) in global */
    OP_DEFL,    /* [k, k]   [-1, +0]    */
    OP_GLDL,    /* [k, k]   [-0, +1]    */
    OP_GSTL,    /* [k, k]   [-0, +0]    */
    OP_JMP,     /* [s, s]   [-0, +0]    */
    OP_JMPF,    /* [s, s]   [-1, +0]    */
    OP_LD,      /* [s]      [-0, +1]    */
    OP_LD0,     /* []       [-0, +1]    */
    OP_LD1,     /* []       [-0, +1]    */
    OP_LD2,     /* []       [-0, +1]    */
    OP_LD3,     /* []       [-0, +1]    */
    OP_LD4,     /* []       [-0, +1]    */
    OP_LD5,     /* []       [-0, +1]    */
    OP_LD6,     /* []       [-0, +1]    */
    OP_LD7,     /* []       [-0, +1]    */
    OP_LD8,     /* []       [-0, +1]    */
    OP_ST,      /* [s]      [-0, +0]    */
} opcode_t;

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
