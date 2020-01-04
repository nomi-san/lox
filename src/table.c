#include <stdlib.h>

#include "table.h"
#include "object.h"

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
