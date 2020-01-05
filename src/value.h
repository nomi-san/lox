#pragma once

#include "common.h"

typedef struct _obj obj_t;
typedef struct _str str_t;
typedef struct _fun fun_t;

typedef enum {
    VT_NIL,
    VT_BOOL,
    VT_NUM,
    VT_OBJ
} vtype_t;

typedef enum {
    OT_STR,
    OT_FUN,
} otype_t;

enum {
    VT_NIL_NIL      = CMB_BYTES(VT_NIL, VT_NIL),
    VT_NIL_BOOL     = CMB_BYTES(VT_NIL, VT_BOOL),
    VT_NIL_NUM      = CMB_BYTES(VT_NIL, VT_NUM),
    VT_NIL_OBJ      = CMB_BYTES(VT_NIL, VT_OBJ),

    VT_BOOL_NIL     = CMB_BYTES(VT_BOOL, VT_NIL),
    VT_BOOL_BOOL    = CMB_BYTES(VT_BOOL, VT_BOOL),
    VT_BOOL_NUM     = CMB_BYTES(VT_BOOL, VT_NUM),
    VT_BOOL_OBJ     = CMB_BYTES(VT_BOOL, VT_OBJ),

    VT_NUM_NIL      = CMB_BYTES(VT_NUM, VT_NIL),
    VT_NUM_BOOL     = CMB_BYTES(VT_NUM, VT_BOOL),
    VT_NUM_NUM      = CMB_BYTES(VT_NUM, VT_NUM),
    VT_NUM_OBJ      = CMB_BYTES(VT_NUM, VT_OBJ),

    VT_OBJ_NIL      = CMB_BYTES(VT_OBJ, VT_NIL),
    VT_OBJ_BOOL     = CMB_BYTES(VT_OBJ, VT_BOOL),
    VT_OBJ_NUM      = CMB_BYTES(VT_OBJ, VT_NUM),
    VT_OBJ_OBJ      = CMB_BYTES(VT_OBJ, VT_OBJ),
};

struct _val {
    vtype_t type;
    union {
        bool Bool : 1;
        double Num;
        obj_t *Obj;
        uint64_t raw;
    };
};

typedef struct {
    int count;
    int capacity;
    val_t *values;
} arr_t;

static const val_t VAL_NIL = { .type = VT_NIL };
static const val_t VAL_TRUE = { .type = VT_BOOL, .Bool = true };
static const val_t VAL_FALSE = { .type = VT_BOOL, .Bool = false };

#define VAL_BOOL(b)     ((val_t){ .type = VT_BOOL, .Bool = (b) })
#define VAL_NUM(n)      ((val_t){ .type = VT_NUM, .Num = (n) })
#define VAL_OBJ(o)      ((val_t){ .type = VT_OBJ, .Obj = (obj_t *)(o) })

#define AS_BOOL(v)      ((v).Bool)
#define AS_NUM(v)       ((v).Num)
#define AS_OBJ(v)       ((v).Obj)
#define AS_TYPE(v)      ((v).type)

#define IS_NIL(v)       (AS_TYPE(v) == VT_NIL)
#define IS_BOOL(v)      (AS_TYPE(v) == VT_BOOL)
#define IS_NUM(v)       (AS_TYPE(v) == VT_NUM)
#define IS_OBJ(v)       (AS_TYPE(v) == VT_OBJ)

#define IS_FALSEY(v)    (!(bool)(v).raw)

void val_print(val_t value);
bool val_equal(val_t a, val_t b);

void arr_init(arr_t *array);
void arr_free(arr_t *array);
int arr_add(arr_t *array, val_t value, bool allowdup);
