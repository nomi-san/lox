#pragma once

#include "common.h"
#include "object.h"

typedef struct {
    size_t allocated;
    obj_t *objects;
} gc_t;

void *gc_realloc();
