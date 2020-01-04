#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

// fnv1a_32
uint32_t hash_bytes(const void *bytes, size_t size)
{
    uint32_t hash = 2166136261u;
    const uint8_t *bs = (uint8_t *)bytes;

    for (size_t i = 0; i < size; i++) {
        hash ^= bs[i];
        hash *= 16777619;
    }

    return hash;
}
