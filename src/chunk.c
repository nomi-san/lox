#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"

void chunk_init(chunk_t *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
}

void chunk_free(chunk_t *chunk)
{
    free(chunk->code);
    free(chunk->lines);

    chunk_init(chunk);
}

void chunk_emit(chunk_t *chunk, uint8_t byte, int ln, int col)
{
    if (chunk->count >= chunk->capacity) {
        chunk->capacity += CHUNK_CODEPAGE;
        chunk->code = realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
        chunk->lines = realloc(chunk->lines, chunk->capacity * sizeof(uint32_t));
    }

    uint32_t line = ((ln & 0xFFFF) << 16) | (col & 0xFFFF);
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}
