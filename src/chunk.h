#pragma once

#include "common.h"

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    uint32_t *lines;
} chunk_t;

void chunk_init(chunk_t *chunk);
void chunk_free(chunk_t *chunk);
void chunk_emit(chunk_t *chunk, uint8_t byte, int line, int column);
