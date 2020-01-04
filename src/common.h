#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CMB_BYTES(l, r)	    (uint8_t)(((char)(l) & 0xF) | ((char)(r) & 0xF) << 4)
#define CMB_LEFT(c)	        (char)(((c) & 0xF))
#define CMB_RIGHT(c)	    (char)((((c) >> 4) & 0xF))

#define GROW_CAPACITY(x)    ((x) < 8 ? 8 : (x) * 2)

#define STACK_MAX           256

#define VM_INIT_ERROR       -1
#define VM_OK               0
#define VM_COMPILE_ERROR    1
#define VM_RUNTIME_ERROR    2

#define DEBUG_PRINT_CODE

typedef struct _val val_t;
typedef struct _vm  vm_t;
typedef struct _gc  gc_t;

uint32_t hash_bytes(const void *bytes, size_t size);
char *read_file(const char *path, size_t *size);
