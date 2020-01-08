#include <stdlib.h>
#include <math.h>

#include "libs.h"
#include "vm.h"
#include "object.h"

static val_t math_abs(vm_t *vm, int argc, val_t *args)
{
    double number = AS_NUM(args[0]);
    double result = (number < 0) ? (-number) : number;

    return VAL_NUM(result);
}

static val_t math_ceil(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double result = ceil(x);

    return VAL_NUM(result);
}

static val_t math_cos(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double result = cos(x);

    return VAL_NUM(result);
}

static val_t math_floor(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double result = floor(x);

    return VAL_NUM(result);
}

static val_t math_log(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double result = log(x);

    return VAL_NUM(result);
}

static val_t math_log10(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double result = log10(x);

    return VAL_NUM(result);
}

static val_t math_pow(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double y = AS_NUM(args[1]);
    double result = pow(x, y);

    return VAL_NUM(result);
}

static val_t math_sin(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double result = sin(x);

    return VAL_NUM(result);
}

static val_t math_sqrt(vm_t *vm, int argc, val_t *args)
{
    double x = AS_NUM(args[0]);
    double result = sqrt(x);

    return VAL_NUM(result);
}

void load_libmath(vm_t *vm)
{
    map_t *math = map_new(vm, 0, 0);

    map_set(vm, math, "abs", VAL_CFN(math_abs));
    map_set(vm, math, "ceil", VAL_CFN(math_ceil));
    map_set(vm, math, "cos", VAL_CFN(math_cos));
    map_set(vm, math, "floor", VAL_CFN(math_floor));
    map_set(vm, math, "log", VAL_CFN(math_log));
    map_set(vm, math, "log10", VAL_CFN(math_log10));
    map_set(vm, math, "pow", VAL_CFN(math_pow));
    map_set(vm, math, "sin", VAL_CFN(math_sin));
    map_set(vm, math, "sqrt", VAL_CFN(math_sqrt));

    set_global(vm, "math", VAL_OBJ(math));
}
