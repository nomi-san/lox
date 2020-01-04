#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gc.h"
#include "vm.h"
#include "object.h"

void gc_init(gc_t *gc)
{
    gc->next = 0;
    gc->allocated = 0;
    gc->objects = NULL;
}

void gc_free(gc_t *gc)
{
    obj_t *object = gc->objects;

    while (object != NULL) {
        obj_t *next = object->next;
        obj_free(gc, object);
        object = next;
    }
}

void *gc_realloc(gc_t *gc, void *ptr, size_t old, size_t new)
{
    gc->allocated += new - old;

    if (new > old && gc->allocated > gc->next) {
        // todo
        //gc_collect(gc);
    }

    if (new == 0) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, new);
}
