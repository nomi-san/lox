#pragma once

#include "common.h"
#include "value.h"

typedef enum {

    OP_CALL,
    OP_RET,
    OP_PRINT,

    OP_POP,

    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_CONST,
    OP_CONSTL,

    OP_NEG,
    OP_NOT,

    OP_LT,
    OP_LE,
    OP_EQ,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

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
#define CHUNK_GETCOL(c, i)  ((c)->lines)[i] & 0xFFFF)
