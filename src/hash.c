#include <stdlib.h>
#include <string.h>

#include "hash.h"

#define HASH_MAX_LOAD   0.75
#define UNUSED_INDEX    UINT64_MAX

void hash_init(hash_t *hash)
{
    hash->count = 0;
    hash->capacity = 0;
    hash->indexes = NULL;
}

void hash_free(hash_t *hash)
{
    free(hash->indexes);
    hash_init(hash);
}

static index_t *hash_find(index_t *indexes, int capacity, uint64_t key)
{
    uint32_t i = key % capacity;
    index_t *tombstone = NULL;

    for (;;) {
        index_t *index = &indexes[i];

        if (index->key == UNUSED_INDEX) {
            if (IS_NIL(index->value)) {
                // Empty entry.                              
                return tombstone != NULL ? tombstone : index;
            }
            else {
                // We found a tombstone.                     
                if (tombstone == NULL) tombstone = index;
            }
        }
        else if (index->key == key) {
            // We found the key.
            return index;
        }

        i = (i + 1) % capacity;
    }
}

static void hash_resize(hash_t *hash, int capacity)
{
    index_t *indexes = malloc(capacity * sizeof(index_t));

    for (int i = 0; i < capacity; i++) {
        indexes[i].key = UNUSED_INDEX;
        indexes[i].value = VAL_NIL;
    }
 
    hash->count = 0;
    for (int i = 0; i < hash->capacity; i++) {
        index_t *index = &hash->indexes[i];
        if (index->key == UNUSED_INDEX) continue;

        index_t *dest = hash_find(indexes, capacity, index->key);
        dest->key = index->key;
        dest->value = index->value;
        hash->count++;
    }

    free(hash->indexes);
    hash->indexes = indexes;
    hash->capacity = capacity;
}

bool hash_get(hash_t *hash, uint64_t key, val_t *value)
{
    if (hash->count == 0) return false;

    index_t *index = hash_find(hash->indexes, hash->capacity, key);
    if (index->key == UNUSED_INDEX) return false;

    (*value) = index->value;
    return true;
}

bool hash_set(hash_t *hash, uint64_t key, val_t value)
{
    if (hash->count + 1 > hash->capacity * HASH_MAX_LOAD) {
        int capacity = GROW_CAPACITY(hash->capacity);
        hash_resize(hash, capacity);
    }

    index_t *index = hash_find(hash->indexes, hash->capacity, key);

    bool isNewKey = (index->key == UNUSED_INDEX);
    if (isNewKey && IS_NIL(index->value)) hash->count++;

    index->key = key;
    index->value = value;
    return isNewKey;
}
