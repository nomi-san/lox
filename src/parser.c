#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "object.h"

typedef struct _parser   parser_t;
typedef struct _compiler compiler_t;

struct _parser {
    vm_t *vm;
    chunk_t *compilingChunk;
    lexer_t *lexer;
    compiler_t *compiler;
    tok_t current;
    tok_t previous;
    bool hadError;
    bool panicMode;
};

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

typedef void (* parsefn_t)(parser_t *parser, bool canAssign);

typedef struct {
    parsefn_t prefix;
    parsefn_t infix;
    prec_t precedence;
} rule_t;

typedef struct {
    tok_t name;
    int depth;
} local_t;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT
} funtype_t;

struct _compiler
{
    compiler_t *enclosing;
    fun_t *function;
    funtype_t type;
    local_t locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
};

static chunk_t *currentChunk(parser_t *parser)
{
    return &parser->compiler->function->chunk;
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

static int emitJump(parser_t *parser, uint8_t instruction)
{
    emitByte(parser, instruction);
    emitBytes(parser, 0, 0);
    return currentChunk(parser)->count - 2;
}

static void emitReturn(parser_t *parser)
{
    emitByte(parser, OP_NIL);
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

static void emitBytesAndConstLong(parser_t *parser, uint8_t op, int arg)
{
#define CHANGE(x)   (op == x) op = x##L
    if (arg > UINT8_MAX) {
        if CHANGE(OP_GLD);
        if CHANGE(OP_GST);
        if CHANGE(OP_DEF);
        if CHANGE(OP_CONST);
        emitByte(parser, op);
        emitBytes(parser, (arg >> 8) & 0xFF, arg & 0xFF);
        return;
    }
#undef CHANGE
    if (op == OP_LD && arg <= 8) {
        emitByte(parser, (uint8_t)(OP_LD0 + arg));
        return;
    }
    emitBytes(parser, op, (uint8_t)arg);
}

static void emitConstant(parser_t *parser, val_t value)
{
    uint16_t constant = makeConstant(parser, value);
    emitBytesAndConstLong(parser, OP_CONST, constant);
}

static void patchJump(parser_t *parser, int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk(parser)->count - offset - 2;

    if (jump > UINT16_MAX) {
        error(parser, "Too much code to jump over.");
    }

    currentChunk(parser)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(parser)->code[offset + 1] = jump & 0xff;
}

static void initCompiler(parser_t *parser, compiler_t *compiler, funtype_t type)
{
    compiler->enclosing = parser->compiler;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = fun_new(parser->vm);

    if (type != TYPE_SCRIPT) {
        compiler->function->name = str_copy(parser->vm, parser->previous.start,
            parser->previous.length);
    }

    local_t *local = &compiler->locals[compiler->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;

    parser->compiler = compiler;
}

static fun_t *endCompiler(parser_t *parser)
{
    emitReturn(parser);
    fun_t *function = parser->compiler->function;

#ifdef DEBUG_PRINT_CODE                      
    if (!parser->hadError) {
        //disassembleChunk(currentChunk(parser), "code");
    }
#endif

    parser->compiler = parser->compiler->enclosing;
    return function;
}

static void beginScope(parser_t *parser)
{
    compiler_t *current = parser->compiler;
    current->scopeDepth++;
}

static void endScope(parser_t *parser)
{
    compiler_t *current = parser->compiler;
    current->scopeDepth--;

    while (current->localCount > 0 &&
        current->locals[current->localCount - 1].depth >
        current->scopeDepth) {
        emitByte(parser, OP_POP);
        current->localCount--;
    }
}

static void expression(parser_t *parser);
static void statement(parser_t *parser);
static void declaration(parser_t *parser);
static rule_t *getRule(toktype_t type);
static void parsePrecedence(parser_t *parser, prec_t precedence);

static uint16_t identifierConstant(parser_t *parser, tok_t *name)
{
    str_t *id = str_copy(parser->vm, name->start, name->length);
    return makeConstant(parser, VAL_OBJ(id));
}

static bool identifiersEqual(tok_t *a, tok_t *b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(parser_t *parser, compiler_t *compiler, tok_t *name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        local_t *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error(parser, "Cannot read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void addLocal(parser_t *parser, tok_t name)
{
    compiler_t *current = parser->compiler;

    if (current->localCount == UINT8_COUNT) {
        error(parser, "Too many local variables in function.");
        return;
    }

    local_t *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}

static void declareVariable(parser_t *parser)
{
    compiler_t *current = parser->compiler;

    // Global variables are implicitly declared.
    if (current->scopeDepth == 0) return;

    tok_t *name = &parser->previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        local_t *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error(parser, "Variable with this name already declared in this scope.");
        }
    }

    addLocal(parser, *name);
}

static uint16_t parseVariable(parser_t *parser, const char *errorMessage)
{
    consume(parser, TOKEN_IDENTIFIER, errorMessage);

    declareVariable(parser);
    if (parser->compiler->scopeDepth > 0) return 0;

    return identifierConstant(parser, &parser->previous);
}

static void markInitialized(parser_t *parser)
{
    compiler_t *current = parser->compiler;

    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth =
        current->scopeDepth;
}

static void defineVariable(parser_t *parser, uint16_t global)
{
    if (parser->compiler->scopeDepth > 0) {
        markInitialized(parser);
        return;
    }

    emitBytesAndConstLong(parser, OP_DEF, global);
}

static uint8_t argumentList(parser_t *parser)
{
    uint8_t argCount = 0;
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            expression(parser);
            argCount++;
            if (argCount == 32) {
                error(parser, "Cannot have more than 32 arguments.");
            }
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void and_(parser_t *parser, bool canAssign)
{
    int endJump = emitJump(parser, OP_JMPF);

    emitByte(parser, OP_POP);
    parsePrecedence(parser, PREC_AND);

    patchJump(parser, endJump);
}

static void binary(parser_t *parser, bool canAssign)
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

static void call(parser_t *parser, bool canAssign)
{
    uint8_t argCount = argumentList(parser);
    emitBytes(parser, OP_CALL, argCount);
}

static void literal(parser_t *parser, bool canAssign)
{
    switch (parser->previous.type) {
        case TOKEN_FALSE:   emitByte(parser, OP_FALSE); break;
        case TOKEN_NIL:     emitByte(parser, OP_NIL); break;
        case TOKEN_TRUE:    emitByte(parser, OP_TRUE); break;
        case TOKEN_FUN:     emitByte(parser, OP_LD0); break;
        default:
            return; // Unreachable.                   
    }
}

static void grouping(parser_t *parser, bool canAssign)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(parser_t *parser, bool canAssign)
{
    double n = strtod(parser->previous.start, NULL);
    emitConstant(parser, VAL_NUM(n));
}

static void string(parser_t *parser, bool canAssign)
{
    str_t *s = str_copy(parser->vm,
        parser->previous.start + 1, parser->previous.length - 2);

    emitConstant(parser, VAL_OBJ(s));
}

static void namedVariable(parser_t *parser, tok_t name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(parser, parser->compiler, &name);

    if (arg != -1) {
        getOp = OP_LD;
        setOp = OP_ST;
    }
    else {
        arg = identifierConstant(parser, &name);
        getOp = OP_GLD;
        setOp = OP_GST;
    }

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitBytesAndConstLong(parser, setOp, arg);
    }
    else {
        emitBytesAndConstLong(parser, getOp, arg);
    }
}

static void variable(parser_t *parser, bool canAssign)
{
    namedVariable(parser, parser->previous, canAssign);
}

static void or_(parser_t *parser, bool canAssign)
{
    int elseJump = emitJump(parser, OP_JMPF);
    int endJump = emitJump(parser, OP_JMP);

    patchJump(parser, elseJump);
    emitByte(parser, OP_POP);

    parsePrecedence(parser, PREC_OR);
    patchJump(parser, endJump);
}

static void unary(parser_t *parser, bool canAssign)
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
    { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN 
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

    { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
    { string,   NULL,    PREC_NONE },       // TOKEN_STRING          
    { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER

    { NULL,     and_,    PREC_AND },        // TOKEN_AND   
    { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE            
    { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR             
    { literal,  NULL,    PREC_NONE },       // TOKEN_FUN             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_IF              
    { literal,  NULL,    PREC_NONE },       // TOKEN_NIL             
    { NULL,     or_,     PREC_OR },         // TOKEN_OR   
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

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(parser, canAssign);

    while (precedence <= getRule(parser->current.type)->precedence) {
        advance(parser);
        parsefn_t infixRule = getRule(parser->previous.type)->infix;
        infixRule(parser, canAssign);
    }

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        error(parser, "Invalid assignment target.");
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

static void block(parser_t *parser)
{
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        declaration(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(parser_t *parser, funtype_t type)
{
    compiler_t compiler;
    initCompiler(parser, &compiler, type);
    beginScope(parser);

    // Compile the parameter list.                                
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            int arity = ++parser->compiler->function->arity;
            if (arity > 32) {
                errorAtCurrent(parser, "Cannot have more than 32 parameters.");
            }
            uint16_t paramConstant = parseVariable(parser, "Expect parameter name.");
            defineVariable(parser, paramConstant);
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    // The body.                                                  
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block(parser);

    // Create the function object.                                
    fun_t *function = endCompiler(parser);
    uint16_t constant = makeConstant(parser, VAL_OBJ(function));

    emitBytesAndConstLong(parser, OP_CONST, constant);
}

static void funDeclaration(parser_t *parser)
{
    uint16_t global = parseVariable(parser, "Expect function name.");
    markInitialized(parser);
    function(parser, TYPE_FUNCTION);
    defineVariable(parser, global);
}

static void varDeclaration(parser_t *parser)
{
    uint16_t global = parseVariable(parser, "Expect variable name.");

    if (match(parser, TOKEN_EQUAL)) {
        expression(parser);
    }
    else {
        emitByte(parser, OP_NIL);
    }
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(parser, global);
}

static void expressionStatement(parser_t *parser)
{
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");

    emitByte(parser, OP_POP);
}

static void ifStatement(parser_t *parser)
{
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(parser, OP_JMPF);
    emitByte(parser, OP_POP);
    statement(parser);

    int elseJump = emitJump(parser, OP_JMP);

    patchJump(parser, thenJump);
    emitByte(parser, OP_POP);

    if (match(parser, TOKEN_ELSE)) statement(parser);
    patchJump(parser, elseJump);
}

static void printStatement(parser_t *parser)
{
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");

    emitByte(parser, OP_PRINT);
}

static void returnStatement(parser_t *parser)
{
    if (parser->compiler->type == TYPE_SCRIPT) {
        error(parser, "Cannot return from top-level code.");
    }

    if (match(parser, TOKEN_SEMICOLON)) {
        emitReturn(parser);
    }
    else {
        expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(parser, OP_RET);
    }
}

static void synchronize(parser_t *parser)
{
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        switch (parser->current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:; // Do nothing.
        }

        advance(parser);
    }
}

static void declaration(parser_t *parser)
{
    if (match(parser, TOKEN_FUN)) {
        funDeclaration(parser);
    }
    else if (match(parser, TOKEN_VAR)) {
        varDeclaration(parser);
    }
    else {
        statement(parser);
    }

    if (parser->panicMode) synchronize(parser);
}

static void statement(parser_t *parser)
{
    if (match(parser, TOKEN_PRINT)) {
        printStatement(parser);
    }
    else if (match(parser, TOKEN_IF)) {
        ifStatement(parser);
    }
    else if (match(parser, TOKEN_RETURN)) {
        returnStatement(parser);
    }
    else if (match(parser, TOKEN_LEFT_BRACE)) {
        beginScope(parser);
        block(parser);
        endScope(parser);
    }
    else {
        expressionStatement(parser);
    }
}

fun_t *compile(vm_t *vm, const char *fname, const char *source)
{
    lexer_t lexer;
    parser_t parser;
    compiler_t compiler;

    parser.vm = vm;
    parser.lexer = &lexer;
    parser.compiler = NULL;
    parser.hadError = false;
    parser.panicMode = false;

    lexer_init(&lexer, vm, fname, source);
    initCompiler(&parser, &compiler, TYPE_SCRIPT);
    
    advance(&parser);
    while (!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    fun_t *function = endCompiler(&parser);
    return parser.hadError ? NULL : function;
}
