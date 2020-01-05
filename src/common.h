#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UINT8_COUNT         (UINT8_MAX + 1)

#define CMB_BYTES(l, r)	    (uint8_t)(((char)(l) & 0xF) | ((char)(r) & 0xF) << 4)
#define CMB_LEFT(c)	        (char)(((c) & 0xF))
#define CMB_RIGHT(c)	    (char)((((c) >> 4) & 0xF))

#define GROW_CAPACITY(x)    ((x) < 8 ? 8 : (x) * 2)

#define FRAMES_MAX          64
#define STACK_MAX           (FRAMES_MAX * UINT8_COUNT)

#define VM_INIT_ERROR       -1
#define VM_OK               0
#define VM_COMPILE_ERROR    1
#define VM_RUNTIME_ERROR    2

#define DEBUG_PRINT_CODE

typedef struct _val val_t;
typedef struct _vm  vm_t;
typedef struct _gc  gc_t;

typedef struct {
    char *buffer;
    size_t size;
    const char *fname;
} src_t;

uint32_t hash_bytes(const void *bytes, size_t size);
char *read_file(const char *path, size_t *size);

src_t *src_new(const char *fname);
void src_free(src_t *source);
