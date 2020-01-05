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

char *read_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        goto _clean;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        goto _clean;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        goto _clean;
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    if (size) *size = bytesRead;
    return buffer;

_clean:
    if (file != NULL) fclose(file);
    free(buffer);
    return NULL;
}

src_t *src_new(const char *fname)
{
    src_t *source = malloc(sizeof(src_t));
    if (source == NULL) return NULL;

    source->fname = fname;
    source->buffer = read_file(fname, &source->size);

    if (source->buffer == NULL) {
        free(source);
        return NULL;
    }

    return source;
}

void src_free(src_t *source)
{
    if (source == NULL) return;

    free(source->buffer);
    free(source);
}
