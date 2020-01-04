#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "object.h"

typedef struct {
    vm_t *vm;
    chunk_t *compilingChunk;
    lexer_t *lexer;
    tok_t current;
    tok_t previous;
    bool hadError;
    bool panicMode;
} parser_t;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =        
    PREC_OR,          // or       
    PREC_AND,         // and      
    PREC_EQUALITY,    // == !=    
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -      
    PREC_FACTOR,      // * /      
    PREC_UNARY,       // ! -      
    PREC_CALL,        // . ()     
    PREC_PRIMARY
} prec_t;

typedef void (* parsefn_t)(parser_t *parser);

typedef struct {
    parsefn_t prefix;
    parsefn_t infix;
    prec_t precedence;
} rule_t;

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

static bool check(parser_t *parser, toktype_t type)
{
    return parser->current.type == type;
}

static bool match(parser_t *parser, toktype_t type)
{
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
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

static uint16_t makeConstant(parser_t *parser, val_t value)
{
    int constant = arr_add(&currentChunk(parser)->constants, value, false);
    if (constant > UINT16_MAX) {
        error(parser, "Too many constants in one chunk.");
        return 0;
    }

    return (uint16_t)constant;
}

static void emitConstant(parser_t *parser, val_t value)
{
    uint16_t constant = makeConstant(parser, value);

    if (constant > UINT8_MAX) {
        emitByte(parser, OP_CONSTL);
        emitBytes(parser, (constant >> 8) & 0xFF, constant & 0xFF);
        return;
    }

    emitBytes(parser, OP_CONST, (uint8_t)constant);
}

static void endCompiler(parser_t *parser)
{
    emitReturn(parser);

#ifdef DEBUG_PRINT_CODE                      
    if (!parser->hadError) {
        //disassembleChunk(currentChunk(parser), "code");
    }
#endif
}

static void expression(parser_t *parser);
static void statement(parser_t *parser);
static void declaration(parser_t *parser);
static rule_t *getRule(toktype_t type);
static void parsePrecedence(parser_t *parser, prec_t precedence);

static void binary(parser_t *parser)
{
    // Remember the operator.                                
    toktype_t operatorType = parser->previous.type;

    // Compile the right operand.                            
    rule_t *rule = getRule(operatorType);
    parsePrecedence(parser, (prec_t)(rule->precedence + 1));

    // Emit the operator instruction.                        
    switch (operatorType) {
        case TOKEN_EQUAL_EQUAL:   emitByte(parser, OP_EQ); break;
        case TOKEN_LESS:          emitByte(parser, OP_LT); break;
        case TOKEN_LESS_EQUAL:    emitByte(parser, OP_LE); break;

        case TOKEN_BANG_EQUAL:    emitBytes(parser, OP_EQ, OP_NOT); break;
        case TOKEN_GREATER:       emitBytes(parser, OP_LE, OP_NOT); break;
        case TOKEN_GREATER_EQUAL: emitBytes(parser, OP_LT, OP_NOT); break;

        case TOKEN_PLUS:          emitByte(parser, OP_ADD); break;
        case TOKEN_MINUS:         emitByte(parser, OP_SUB); break;
        case TOKEN_STAR:          emitByte(parser, OP_MUL); break;
        case TOKEN_SLASH:         emitByte(parser, OP_DIV); break;
        default:
            return; // Unreachable.                              
    }
}

static void literal(parser_t *parser)
{
    switch (parser->previous.type) {
        case TOKEN_FALSE:   emitByte(parser, OP_FALSE); break;
        case TOKEN_NIL:     emitByte(parser, OP_NIL); break;
        case TOKEN_TRUE:    emitByte(parser, OP_TRUE); break;
        default:
            return; // Unreachable.                   
    }
}

static void grouping(parser_t *parser)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(parser_t *parser)
{
    double n = strtod(parser->previous.start, NULL);
    emitConstant(parser, VAL_NUM(n));
}

static void string(parser_t *parser)
{
    str_t *s = str_copy(parser->vm,
        parser->previous.start + 1, parser->previous.length - 2);

    emitConstant(parser, VAL_OBJ(s));
}

static void unary(parser_t *parser)
{
    toktype_t operatorType = parser->previous.type;

    // Compile the operand.                        
    parsePrecedence(parser, PREC_UNARY);

    // Emit the operator instruction.              
    switch (operatorType) {
        case TOKEN_BANG:    emitByte(parser, OP_NOT); break;
        case TOKEN_MINUS:   emitByte(parser, OP_NEG); break;
        default:
            return; // Unreachable.                    
    }
}

static rule_t rules[] = {
    { grouping, NULL,    PREC_NONE },       // TOKEN_LEFT_PAREN      
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN     
    { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE

    { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_DOT

    { unary,    binary,  PREC_TERM },       // TOKEN_MINUS           
    { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS            
    { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON       
    { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH           
    { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR

    { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
    { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
    { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL           
    { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL  
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER      
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS         
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL 

    { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER      
    { string,   NULL,    PREC_NONE },       // TOKEN_STRING          
    { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER

    { NULL,     NULL,    PREC_NONE },       // TOKEN_AND             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE            
    { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_IF              
    { literal,  NULL,    PREC_NONE },       // TOKEN_NIL             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_OR              
    { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN          
    { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS            
    { literal,  NULL,    PREC_NONE },       // TOKEN_TRUE            
    { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE

    { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF             
};

static void parsePrecedence(parser_t *parser, prec_t precedence)
{
    advance(parser);
    parsefn_t prefixRule = getRule(parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        error(parser, "Expect expression.");
        return;
    }

    prefixRule(parser);

    while (precedence <= getRule(parser->current.type)->precedence) {
        advance(parser);
        parsefn_t infixRule = getRule(parser->previous.type)->infix;
        infixRule(parser);
    }
}

static rule_t *getRule(toktype_t type)
{
    return &rules[type];
}

static void expression(parser_t *parser)
{
    parsePrecedence(parser, PREC_ASSIGNMENT);
}

static void printStatement(parser_t *parser)
{
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");

    emitByte(parser, OP_PRINT);
}

static void declaration(parser_t *parser)
{
    statement(parser);
}

static void statement(parser_t *parser)
{
    if (match(parser, TOKEN_PRINT)) {
        printStatement(parser);
    }
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
    while (!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    endCompiler(&parser);
    return !parser.hadError;
}
