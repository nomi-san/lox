#pragma once

#include "value.h"
#include "chunk.h"

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

struct _fun {
    obj_t obj;
    int arity;
    chunk_t chunk;
    str_t *name;
};

#define AS_STR(v)       ((str_t *)AS_OBJ(v))
#define AS_CSTR(v)      (((str_t *)AS_OBJ(v))->chars)
#define AS_FUN(v)       ((fun_t *)AS_OBJ(v))

#define OBJ_TYPE(v)     (AS_OBJ(v)->type)

#define IS_STR(v)       (OBJ_TYPE(v) == OT_STR)
#define IS_FUN(v)       (OBJ_TYPE(v) == OT_FUN)

static inline bool obj_is(val_t value, otype_t type) {
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

str_t *str_take(vm_t *vm, char *chars, int length);
str_t *str_copy(vm_t *vm, const char *chars, int length);
fun_t *fun_new(vm_t *vm, src_t *source);

void obj_print(obj_t *object);
void obj_free(gc_t *gc, obj_t *object);
