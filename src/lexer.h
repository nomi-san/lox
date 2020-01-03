#pragma once

#include "common.h"

typedef struct {
    vm_t *vm;
    const char *fname;
    const char *source;
    const char *start;
    const char *current;
    int line;
    int position;
} lexer_t;

void lexer_init(lexer_t *lexer, vm_t *vm, const char *fname, const char *source);
