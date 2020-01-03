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

static bool isDigit(char c)
{
    return (c >= '0')
        && (c <= '9');
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

static char peek(lexer_t *lexer)
{
    return *lexer->current;
}

static char peekNext(lexer_t *lexer)
{
    if (isAtEnd(lexer)) return '\0';
    return lexer->current[1];
}

static void newLine(lexer_t *lexer)
{
    lexer->line++;
    lexer->position = 0;
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

static void skipWhitespace(lexer_t *lexer)
{
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;

            case '\n':
                newLine(lexer);
                advance(lexer);
                break;

            case '/':
                if (peekNext(lexer) == '/') {
                    // A comment goes until the end of the line.   
                    while (peek(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);
                }
                else {
                    return;
                }
                break;

        default:
            return;
        }
    }
}

static tok_t number(lexer_t *lexer)
{
    while (isDigit(peek(lexer))) advance(lexer);

    // Look for a fractional part.             
    if (peek(lexer) == '.' && isDigit(peekNext(lexer))) {
        // Consume the ".".                      
        advance(lexer);

        while (isDigit(peek(lexer))) advance(lexer);
    }

    return makeToken(lexer, TOKEN_NUMBER);
}

static tok_t string(lexer_t *lexer, char start)
{
    while (peek(lexer) != start && !isAtEnd(lexer)) {
        if (peek(lexer) == '\n') newLine(lexer);
        advance(lexer);
    }

    if (isAtEnd(lexer)) return errorToken(lexer, "Unterminated string.");

    // The closing quote.                                    
    advance(lexer);
    return makeToken(lexer, TOKEN_STRING);
}

tok_t lexer_scan(lexer_t *lexer)
{
    skipWhitespace(lexer);

    lexer->start = lexer->current;

    if (isAtEnd(lexer)) return makeToken(lexer, TOKEN_EOF);

    char c = advance(lexer);

    if (isDigit(c)) return number(lexer);

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

        case '\'':
        case '\"': return string(lexer, c);
    }

    return errorToken(lexer, "Unexpected character.");
}
