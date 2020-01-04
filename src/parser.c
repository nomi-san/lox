#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

typedef struct {
    vm_t *vm;
    chunk_t *compilingChunk;
    lexer_t *lexer;
    tok_t current;
    tok_t previous;
    bool hadError;
    bool panicMode;
} parser_t;

static chunk_t *currentChunk(parser_t *parser)
{
    return parser->compilingChunk;
}

static void errorAt(parser_t *parser, tok_t *token, const char *message)
{
    if (parser->panicMode) return;
    parser->panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR) {
        // Nothing.                                                
    }
    else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->hadError = true;
}

static void error(parser_t *parser, const char *message)
{
    errorAt(parser, &parser->previous, message);
}

static void errorAtCurrent(parser_t *parser, const char *message)
{
    errorAt(parser, &parser->current, message);
}

static void advance(parser_t *parser)
{
    parser->previous = parser->current;

    for (;;) {
        parser->current = lexer_scan(parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser, parser->current.start);
    }
}

static void consume(parser_t *parser, toktype_t type, const char* message)
{
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    errorAtCurrent(parser, message);
}

static void emitByte(parser_t *parser, uint8_t byte)
{
    chunk_emit(currentChunk(parser), byte,
        parser->previous.line, parser->previous.column);
}

static void emitBytes(parser_t *parser, uint8_t byte1, uint8_t byte2)
{
    emitByte(parser, byte1);
    emitByte(parser, byte2);
}

static void emitNBytes(parser_t *parser, void *bytes, size_t size)
{
    const uint8_t *bs = bytes;
    for (size_t i = 0; i < size; i++) {
        emitByte(parser, bytes == NULL ? 0 : bs[i]);
    }
}

static void emitReturn(parser_t *parser)
{
    emitByte(parser, OP_RET);
}

static uint8_t makeConstant(parser_t *parser, val_t value)
{
    int constant = arr_add(&currentChunk(parser)->constants, value, false);
    if (constant > UINT8_MAX) {
        error(parser, "Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(parser_t *parser, val_t value)
{
    uint8_t constant = makeConstant(parser, value);
    emitBytes(parser, OP_CONST, constant);
}

static void endCompiler(parser_t *parser)
{
    emitReturn(parser);
}

static void number(parser_t *parser)
{
    double n = strtod(parser->previous.start, NULL);
    emitConstant(parser, VAL_NUM(n));
}

static void expression(parser_t *parser)
{
    // What goes here?      
}

bool compile(vm_t *vm, const char *fname, const char *source, chunk_t *chunk)
{
    lexer_t lexer;
    parser_t parser;

    lexer_init(&lexer, vm, fname, source);
    parser.vm = vm;
    parser.compilingChunk = chunk;
    parser.lexer = &lexer;
    parser.hadError = false;
    parser.panicMode = false;
    
    advance(&parser);
    expression(&parser);
    consume(&parser, TOKEN_EOF, "Expect end of expression.");

    endCompiler(&parser);
    return !parser.hadError;
}
