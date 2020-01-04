#include <stdlib.h>

#include "table.h"
#include "object.h"

#define TABLE_MAX_LOAD  0.75

void tab_init(tab_t *table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void tab_free(tab_t *table)
{
    free(table->entries);
    tab_init(table);
}

static ent_t *findEntry(ent_t *entries, int capacity, str_t *key)
{
    uint32_t index = key->hash % capacity;
    ent_t *tombstone = NULL;

    for (;;) {
        ent_t *entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // Empty entry.                              
                return tombstone != NULL ? tombstone : entry;
            }
            else {
                // We found a tombstone.                     
                if (tombstone == NULL) tombstone = entry;
            }
        }
        else if (entry->key == key) {
            // We found the key.                           
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

bool tab_get(tab_t *table, str_t *key, val_t *value)
{
    if (table->count == 0) return false;

    ent_t *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

static void adjustCapacity(tab_t *table, int capacity)
{
    ent_t *entries = malloc(capacity * sizeof(ent_t));

    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = VAL_NIL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        ent_t *entry = &table->entries[i];
        if (entry->key == NULL) continue;

        ent_t *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

bool tab_set(tab_t *table, str_t *key, val_t value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    ent_t *entry = findEntry(table->entries, table->capacity, key);

    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tab_remove(tab_t *table, str_t *key)
{
    if (table->count == 0) return false;

    // Find the entry.                                             
    ent_t *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    // Place a tombstone in the entry.                             
    entry->key = NULL;
    entry->value = VAL_TRUE;

    return true;
}

void tab_add(tab_t *from, tab_t *to)
{
    for (int i = 0; i < from->capacity; i++) {
        ent_t *entry = &from->entries[i];
        if (entry->key != NULL) {
            tab_set(to, entry->key, entry->value);
        }
    }
}

str_t *tab_findstr(tab_t *table, const char *chars, int length, uint32_t hash)
{
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;

    for (;;) {
        ent_t *entry = &table->entries[index];
        str_t *key = entry->key;

        if (key == NULL) {
            // Stop if we find an empty non-tombstone entry.                 
            if (IS_NIL(entry->value)) return NULL;
        }
        else if (key->length == length && key->hash == hash) {
            // We found it.                                                  
            return key;
        }

        index = (index + 1) % table->capacity;
    }
}
