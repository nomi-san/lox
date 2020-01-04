#pragma once

#include "common.h"
#include "object.h"

struct _gc {
    size_t allocated;
    size_t next;
    obj_t *objects;
};

void gc_init(gc_t *gc);
void gc_free(gc_t *gc);

void *gc_realloc(gc_t *gc, void *ptr, size_t old, size_t new);
