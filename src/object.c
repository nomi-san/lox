#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "vm.h"
#include "gc.h"
#include "table.h"

#define ALLOC(gc, size) \
    gc_realloc(gc, NULL, 0, size)

#define FREE(gc, type, pointer) \
    gc_realloc(gc, pointer, sizeof(type), 0)

#define ALLOC_OBJ(vm, type, objectType) \
    (type *)allocateObject(vm, sizeof(type), objectType)

static obj_t *allocateObject(vm_t *vm, size_t size, otype_t type)
{
    gc_t *gc = &vm->gc;

    obj_t *object = ALLOC(gc, size);
    object->type = type;

    object->next = gc->objects;
    gc->objects = object;
    return object;
}

static str_t *allocateString(vm_t *vm, char *chars, int length, uint32_t hash)
{
    str_t *string = ALLOC_OBJ(vm, str_t, OT_STR);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    tab_set(&vm->strings, string, VAL_NIL);

    return string;
}

str_t *str_take(vm_t *vm, char *chars, int length)
{
    uint32_t hash = hash_bytes(chars, length);
    str_t *interned = tab_findstr(&vm->strings, chars, length, hash);
    if (interned != NULL) {
        free(chars);
        return interned;
    }

    return allocateString(vm, chars, length, hash);
}

str_t *str_copy(vm_t *vm, const char *chars, int length)
{
    uint32_t hash = hash_bytes(chars, length);
    str_t *interned = tab_findstr(&vm->strings, chars, length, hash);
    if (interned != NULL) return interned;

    char *heapChars = malloc((length + 1) * sizeof(char));
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(vm, heapChars, length, hash);
}

fun_t *fun_new(vm_t *vm, src_t *source)
{
    fun_t *function = ALLOC_OBJ(vm, fun_t, OT_FUN);

    function->arity = 0;
    function->name = NULL;
    chunk_init(&function->chunk, source);
    return function;
}

void obj_print(obj_t *object)
{
    switch (object->type) {
        case OT_STR: {
            str_t *string = (str_t *)object;
            printf("%.*s", string->length, string->chars);
            break;
        }
        case OT_FUN: {
            fun_t *function = (fun_t *)object;
            if (function->name == NULL)
                printf("<script>");
            else
                printf("fn: %s", function->name->chars);
            break;
        }
    }
}

void obj_free(gc_t *gc, obj_t *object)
{
    switch (object->type) {
        case OT_STR: {
            str_t *string = (str_t *)object;
            free(string->chars);
            FREE(gc, str_t, string);
            break;
        }
        case OT_FUN: {
            fun_t *function = (fun_t *)object;
            chunk_free(&function->chunk);
            FREE(gc, fun_t, function);
            break;
        }
    }
}
