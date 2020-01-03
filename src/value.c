#include <stdio.h>

#include "value.h"

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
            // todo
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
            // todo
            ;
    }

    return false;
}
