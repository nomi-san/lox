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

static bool isAtEnd(lexer_t *lexer)
{
    return *lexer->current == '\0';
}

static tok_t makeToken(lexer_t *lexer, toktype_t type)
{
    tok_t token;
    token.type = type;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->position - token.length;

    return token;
}

static tok_t errorToken(lexer_t *lexer, const char *message)
{
    tok_t token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer->line;
    token.column = lexer->position - 1;

    return token;
}

tok_t lexer_scan(lexer_t *lexer)
{
    lexer->start = lexer->current;

    if (isAtEnd(lexer)) return makeToken(lexer, TOKEN_EOF);

    return errorToken(lexer, "Unexpected character.");
}
