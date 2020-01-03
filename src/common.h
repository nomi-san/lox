#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GROW_CAPACITY(x)    ((x) < 8 ? 8 : (x) * 2)

#define STACK_MAX   256

#define VM_OK               0
#define VM_COMPILE_ERROR    1
#define VM_RUNTIME_ERROR    2

typedef struct _val val_t;
typedef struct _vm  vm_t;
