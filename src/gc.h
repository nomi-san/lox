#pragma once

#include "common.h"
#include "object.h"

typedef struct {
    size_t allocated;
    obj_t *objects;
} gc_t;

void *gc_realloc(gc_t *gc, void *ptr, size_t old, size_t new);
