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

static char advance(lexer_t *lexer)
{
    lexer->current++;
    lexer->position++;
    return lexer->current[-1];
}

static bool match(lexer_t *lexer, char expected)
{
    if (isAtEnd(lexer)) return false;
    if (*lexer->current != expected) return false;

    lexer->current++;
    lexer->position++;
    return true;
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

    char c = advance(lexer);

    switch (c) {
        case '(': return makeToken(lexer, TOKEN_LEFT_PAREN);
        case ')': return makeToken(lexer, TOKEN_RIGHT_PAREN);
        case '{': return makeToken(lexer, TOKEN_LEFT_BRACE);
        case '}': return makeToken(lexer, TOKEN_RIGHT_BRACE);
        case ';': return makeToken(lexer, TOKEN_SEMICOLON);
        case ',': return makeToken(lexer, TOKEN_COMMA);
        case '.': return makeToken(lexer, TOKEN_DOT);
        case '-': return makeToken(lexer, TOKEN_MINUS);
        case '+': return makeToken(lexer, TOKEN_PLUS);
        case '/': return makeToken(lexer, TOKEN_SLASH);
        case '*': return makeToken(lexer, TOKEN_STAR);

        case '!':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    }

    return errorToken(lexer, "Unexpected character.");
}
