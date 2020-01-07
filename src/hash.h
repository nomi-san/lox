#pragma once

#include "common.h"
#include "value.h"

typedef struct {
    uint64_t key;
    val_t value;
} index_t;

typedef struct {
    int count;
    int capacity;
    index_t *indexes;
} hash_t;

void hash_init(hash_t *hash);
void hash_free(hash_t *hash);

bool hash_get(hash_t *hash, uint64_t key, val_t *value);
bool hash_set(hash_t *hash, uint64_t key, val_t value);
