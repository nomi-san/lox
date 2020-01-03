#include <stdio.h>
#include <string.h>

#include "lexer.h"

void lexer_init(lexer_t *lexer, vm_t *vm, const char *fname, const char *source)
{
    lexer->vm = vm;
    lexer->fname = fname;
    lexer->source = source;

    lexer->start = lexer->source;
    lexer->start = lexer->source;
    lexer->line = 1;
    lexer->position = 1;
}

