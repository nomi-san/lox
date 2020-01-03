#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GROW_CAPACITY(x)    ((x) < 8 ? 8 : (x) * 2)
