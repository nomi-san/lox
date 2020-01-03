#pragma once

#include "common.h"

typedef struct _obj obj_t;

typedef enum {
    VT_NIL,
    VT_BOOL,
    VT_NUM,
    VT_OBJ
} vtype_t;

typedef struct {
    vtype_t type;
    union {
        bool Bool : 1;
        double Num;
        obj_t *Obj;
        uint64_t raw;
    };
} val_t;

void val_print(val_t *value);

static const val_t VAL_NIL = { .type = VT_NIL };
static const val_t VAL_TRUE = { .type = VT_BOOL, .Bool = true }
static const val_t VAL_FALSE = { .type = VT_BOOL, .Bool = false }

#define VAL_BOOL(b)     ((val_t){ .type = VT_BOOL, .Bool = (b) })
#define VAL_NUM(n)      ((val_t){ .type = VT_NUM, .Num = (n) })
#define VAL_OBJ(o)      ((val_t){ .type = VT_OBJ, .o = (obj_t *)(n) })
