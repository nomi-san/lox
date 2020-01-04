#pragma once

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "gc.h"
#include "table.h"

struct _vm {
    uint8_t *ip;
    chunk_t *chunk;
    val_t *top;
    val_t stack[STACK_MAX];

    gc_t gc;
    tab_t strings;
};

vm_t *vm_create();
void vm_close(vm_t *vm);

int do_string(vm_t *vm, const char *source);
int do_file(vm_t *vm, const char *fname);
