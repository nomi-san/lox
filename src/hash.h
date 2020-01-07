#pragma once

#include "common.h"
#include "value.h"

typedef struct {
    double key;
    val_t value;
} index_t;

typedef struct {
    int count;
    int capacity;
    index_t *indexes;
} hash_t;
