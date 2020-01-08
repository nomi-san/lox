#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "vm.h"
#include "gc.h"

#define ALLOC(gc, size) \
    gc_realloc(gc, NULL, 0, size)

#define FREE(gc, type, pointer) \
    gc_realloc(gc, pointer, sizeof(type), 0)

#define ALLOC_OBJ(vm, type, objectType) \
    (type *)allocateObject(vm, sizeof(type), objectType)

static obj_t *allocateObject(vm_t *vm, size_t size, otype_t type)
{
    gc_t *gc = vm->gc;

    obj_t *object = ALLOC(gc, size);
    object->type = type;

    object->next = gc->objects;
    gc->objects = object;
    return object;
}

static str_t *allocateString(vm_t *vm, char *chars, int length, uint32_t hash)
{
    str_t *string = ALLOC_OBJ(vm, str_t, OT_STR);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    tab_set(vm->strings, string, VAL_NIL);

    return string;
}

str_t *str_take(vm_t *vm, char *chars, int length)
{
    uint32_t hash = hash_bytes(chars, length);
    str_t *interned = tab_findstr(vm->strings, chars, length, hash);
    if (interned != NULL) {
        free(chars);
        return interned;
    }

    return allocateString(vm, chars, length, hash);
}

str_t *str_copy(vm_t *vm, const char *chars, int length)
{
    uint32_t hash = hash_bytes(chars, length);
    str_t *interned = tab_findstr(vm->strings, chars, length, hash);
    if (interned != NULL) return interned;

    char *heapChars = malloc((length + 1) * sizeof(char));
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(vm, heapChars, length, hash);
}

fun_t *fun_new(vm_t *vm, src_t *source)
{
    fun_t *function = ALLOC_OBJ(vm, fun_t, OT_FUN);

    function->arity = 0;
    function->name = NULL;
    chunk_init(&function->chunk, source);
    return function;
}

map_t *map_new(vm_t *vm, int arr_cap, int tab_cap)
{
    map_t *map = ALLOC_OBJ(vm, map_t, OT_MAP);

    hash_init(&map->hash);
    tab_init(&map->table);
    // todo
    return map;
}

void map_set(vm_t *vm, map_t *map, const char *key, val_t value)
{
    str_t *field = str_copy(vm, key, (int)strlen(key));

    vm_push(vm, value);
    vm_push(vm, VAL_OBJ(field));
    tab_set(&map->table, field, value);

    vm_pop(vm);
    vm_pop(vm);
}

const char *obj_typeof(obj_t *object)
{
    switch (object->type) {
        case OT_STR:
            return "str";
        case OT_FUN:
            return "fn";
        default:
            return "obj";
    }
}

void obj_print(obj_t *object)
{
    switch (object->type) {
        case OT_STR: {
            str_t *string = (str_t *)object;
            printf("%.*s", string->length, string->chars);
            break;
        }
        case OT_FUN: {
            fun_t *function = (fun_t *)object;
            if (function->name == NULL)
                printf("<script>");
            else
                printf("fn: %s", function->name->chars);
            break;
        }
        case OT_MAP:
            printf("map: %p", object);
            break;
        default:
            printf("obj: %p", object);
            break;
    }
}

void obj_free(gc_t *gc, obj_t *object)
{
    switch (object->type) {
        case OT_STR: {
            str_t *string = (str_t *)object;
            free(string->chars);
            FREE(gc, str_t, string);
            break;
        }
        case OT_FUN: {
            fun_t *function = (fun_t *)object;
            chunk_free(&function->chunk);
            FREE(gc, fun_t, function);
            break;
        }
        case OT_MAP: {
            map_t *map = (map_t *)object;
            hash_free(&map->hash);
            tab_free(&map->table);
            FREE(gc, map_t, map);
            break;
        }
    }
}
