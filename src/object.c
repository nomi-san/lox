#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "vm.h"
#include "gc.h"

#define ALLOC(gc, size) \
    gc_realloc(gc, NULL, 0, size)

#define FREE(gc, type, pointer) \
    gc_realloc(gc, pointer, sizeof(type), 0)

#define ALLOC_OBJ(vm, type, objectType) \
    (type *)allocateObject(vm, sizeof(type), objectType)

static obj_t *allocateObject(vm_t *vm, size_t size, otype_t type)
{
    obj_t *object = ALLOC(vm->gc, size);
    object->type = type;

    object->next = vm->gc->objects;
    vm->gc->objects = object;
    return object;
}

static str_t *allocateString(vm_t *vm, char *chars, int length, uint32_t hash)
{
    str_t *string = ALLOC_OBJ(vm, str_t, OT_STR);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    return string;
}

str_t *str_take(vm_t *vm, char *chars, int length)
{
    uint32_t hash = hash_bytes(chars, length);
    return allocateString(vm, chars, length, hash);
}

str_t *str_copy(vm_t *vm, const char *chars, int length)
{
    uint32_t hash = hash_bytes(chars, length);

    char *heapChars = malloc((length + 1) * sizeof(char));
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(vm, heapChars, length, hash);
}

void obj_print(obj_t *object)
{
    switch (object->type) {
        case OT_STR: {
            str_t *string = (str_t *)object;
            printf("%.*s", string->length, string->chars);
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
            FREE(gc, str_t, object);
            break;
        }
    }
}
