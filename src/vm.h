#pragma once

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "gc.h"
#include "table.h"

typedef struct {
    fun_t *function;
    uint8_t *ip;
    val_t *slots;
} frame_t;

struct _vm {   
    val_t stack[STACK_MAX];
    frame_t frames[FRAMES_MAX];
    val_t *top;
    int frameCount;

    gc_t gc;
    tab_t strings;
    tab_t globals;
};

vm_t *vm_create();
void vm_close(vm_t *vm);

int do_string(vm_t *vm, const char *source);
int do_file(vm_t *vm, const char *fname);
