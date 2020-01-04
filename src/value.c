#include <stdio.h>
#include <stdlib.h>

#include "value.h"
#include "object.h"

void val_print(val_t value)
{
    switch (AS_TYPE(value)) {
        case VT_NIL:
            printf("nil");
            break;
        case VT_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VT_NUM:
            printf("%.14g", AS_NUM(value));
            break;
        case VT_OBJ:
            obj_print(AS_OBJ(value));
            break;
    }
}

bool val_equal(val_t a, val_t b)
{
    if (AS_TYPE(a) != AS_TYPE(b))
        return false;

    switch (AS_TYPE(a)) {
        case VT_NIL:
            return true;
        case VT_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VT_NUM:
            return AS_NUM(a) == AS_NUM(b);
        case VT_OBJ:
            return AS_OBJ(a) == AS_OBJ(b);
    }

    return false;
}

void arr_init(arr_t *array)
{
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void arr_free(arr_t *array)
{
    free(array->values);
    arr_init(array);
}

int arr_add(arr_t *array, val_t value, bool allowdup)
{
    if (!allowdup) {
        for (int i = 0; i < array->count; i++)
            if (val_equal(array->values[i], value))
                return i;
    }

    if (array->count >= array->capacity) {
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = realloc(array->values, array->capacity * sizeof(val_t));
    }

    array->values[array->count] = value;
    return array->count++;
}
