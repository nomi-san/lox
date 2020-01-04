#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gc.h"
#include "vm.h"

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
