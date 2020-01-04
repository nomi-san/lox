#pragma once

#include "value.h"

struct _obj {
    otype_t type;
    struct _obj *next;
};

struct _str {
    obj_t obj;
    char *chars;
    int length;
    uint32_t hash;
};

#define AS_STR(v)       ((str_t *)AS_OBJ(v))         
#define AS_CSTR(v)      (((str_t *)AS_OBJ(v))->chars)

#define OBJ_TYPE(v)     (AS_OBJ(v)->type)

static inline bool obj_is(val_t value, otype_t type) {
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

str_t *str_copy(vm_t *vm, const char *chars, int length);
