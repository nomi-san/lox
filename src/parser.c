#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

bool compile(vm_t *vm, const char *fname, const char *source, chunk_t *chunk)
{
    lexer_t lexer;
    parser_t parser;

    lexer_init(&lexer, vm, fname, source);

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");

    return !parser.hadError;
}
