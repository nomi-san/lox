#pragma once

#include "common.h"
#include "value.h"
#include "chunk.h"

struct _vm {
    val_t *top;
    val_t stack[STACK_MAX];
};

vm_t *vm_create();
void vm_close(vm_t *vm);

int do_string(vm_t *vm, const char *source);
int do_file(vm_t *vm, const char *fname);
