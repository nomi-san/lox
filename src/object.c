#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "object.h"

#define ALLOCATE(vm, type, count) \
    (type *)reallocate(vm, NULL, 0, sizeof(type) * (count))

#define ALLOCATE_OBJ(vm, type, objectType) \
    (type *)allocateObject(vm, sizeof(type), objectType)

static obj_t *allocateObject(vm_t *vm, size_t size, otype_t type)
{
    obj_t *object = (obj_t *)reallocate(vm, NULL, 0, size);
    object->type = type;
    return object;
}

static str_t *allocateString(vm_t *vm, char *chars, int length)
{
    str_t *string = ALLOCATE_OBJ(vm, str_t, OT_STR);
    string->length = length;
    string->chars = chars;

    return string;
}

str_t *str_copy(vm_t *vm, const char *chars, int length)
{
    char *heapChars = ALLOCATE(vm, char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(vm, heapChars, length);
}

void obj_print(val_t value)
{
    switch (OBJ_TYPE(value)) {
        case OT_STR:
            printf("%s", AS_CSTR(value));
            break;
    }
}
