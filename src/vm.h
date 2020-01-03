#pragma once

#include "common.h"
#include "value.h"
#include "chunk.h"

struct _vm {
    val_t *top;
    val_t stack[STACK_MAX];
};
