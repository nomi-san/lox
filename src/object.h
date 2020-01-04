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
