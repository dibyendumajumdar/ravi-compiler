/*
A compiler for Ravi and Lua 5.3. This is work in progress.
Once ready it will be used to create a new byte code generator for Ravi.

Copyright 2018-2020 Dibyendu Majumdar
*/

#ifndef _RAVI_COMPILER_H
#define _RAVI_COMPILER_H

#include <stdio.h>

#include "ravicomp_export.h"

struct compiler_state;
struct lexer_state;
struct linearizer_state;

typedef long long lua_Integer;
typedef double lua_Number;


/* Initialize the compiler state */
RAVICOMP_EXPORT struct compiler_state *raviX_init_compiler();
/* Destroy the compiler state */
RAVICOMP_EXPORT void raviX_destroy_compiler(struct compiler_state *compiler);

/* ------------------------ LEXICAL ANALYZER -------------------------------*/
/* This is derived from PuC Lua implementation                              */
enum RESERVED {
	/* terminal symbols denoted by reserved words */
	FIRST_RESERVED = 257,
	TK_AND = FIRST_RESERVED,
	TK_BREAK,
	TK_DO,
	TK_ELSE,
	TK_ELSEIF,
	TK_END,
	TK_FALSE,
	TK_FOR,
	TK_FUNCTION,
	TK_GOTO,
	TK_IF,
	TK_IN,
	TK_LOCAL,
	TK_DEFER,
	TK_NIL,
	TK_NOT,
	TK_OR,
	TK_REPEAT,
	TK_RETURN,
	TK_THEN,
	TK_TRUE,
	TK_UNTIL,
	TK_WHILE,
	/* other terminal symbols */
	TK_IDIV,
	TK_CONCAT,
	TK_DOTS,
	TK_EQ,
	TK_GE,
	TK_LE,
	TK_NE,
	TK_SHL,
	TK_SHR,
	TK_DBCOLON,
	TK_EOS,
	TK_FLT,
	TK_INT,
	TK_NAME,
	TK_STRING,
	/** RAVI extensions */
	TK_TO_INTEGER,
	TK_TO_NUMBER,
	TK_TO_INTARRAY,
	TK_TO_NUMARRAY,
	TK_TO_TABLE,
	TK_TO_STRING,
	TK_TO_CLOSURE
};

typedef union {
	lua_Number r;
	lua_Integer i;
	const char* ts;
} SemInfo; /* semantic information */

typedef struct Token {
	int token;
	SemInfo seminfo;
} Token;

typedef struct {
	int current;	 /* current character (charint) */
	int linenumber;	 /* input line counter */
	int lastline;	 /* line of last token 'consumed' */
	Token t;	 /* current token */
	Token lookahead; /* look ahead token */
} LexState;

/* Initialize lexical analyser. Takes as input a buffer containing Lua/Ravi source and the source name*/
RAVICOMP_EXPORT struct lexer_state *raviX_init_lexer(struct compiler_state *compiler_state, const char *buf, size_t buflen,
						     const char *source_name);
/* Gets the lexer data structure that can be used to access the current token */
RAVICOMP_EXPORT LexState *raviX_get_lexer_info(struct lexer_state *ls);
/* Retrieves the next token and saves it is LexState structure. If a lookahead was set then that is retrieved,
 else the next token is retrieved */
RAVICOMP_EXPORT void raviX_next(struct lexer_state *ls);
/* Retrieves the next token and sets it as the lookahead. This means that a next call will get the lookahead */
RAVICOMP_EXPORT int raviX_lookahead(struct lexer_state *ls);
/* Release all data structures used by the lexer */
RAVICOMP_EXPORT void raviX_destroy_lexer(struct lexer_state *);

/* parser and ast builder */
RAVICOMP_EXPORT const char *raviX_create_string(struct compiler_state *container, const char *s, size_t len);
RAVICOMP_EXPORT int raviX_parse(struct compiler_state *container, const char *buffer, size_t buflen, const char *name);
RAVICOMP_EXPORT void raviX_output_ast(struct compiler_state *container, FILE *fp);
RAVICOMP_EXPORT int
raviX_ast_typecheck(struct compiler_state *container); /* Perform type checks and assign types to AST */

/* linear IR generator */
RAVICOMP_EXPORT struct linearizer_state *raviX_init_linearizer(struct compiler_state *container);
RAVICOMP_EXPORT void raviX_destroy_linearizer(struct linearizer_state *linearizer);
RAVICOMP_EXPORT int raviX_ast_linearize(struct linearizer_state *linearizer);
RAVICOMP_EXPORT void raviX_output_linearizer(struct linearizer_state *linearizer, FILE *fp);

/* utilies */
RAVICOMP_EXPORT const char *raviX_get_last_error(struct compiler_state *container);

#endif
