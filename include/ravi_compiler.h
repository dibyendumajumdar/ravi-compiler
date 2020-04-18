/*
A compiler for Ravi and Lua 5.3. This is work in progress.
Once ready it will be used to create a new byte code generator for Ravi.

This header file defines the public api

Copyright 2018-2020 Dibyendu Majumdar
*/

#ifndef ravicomp_COMPILER_H
#define ravicomp_COMPILER_H

#include "ravicomp_export.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct compiler_state;
struct lexer_state;
struct linearizer_state;

typedef long long lua_Integer;
typedef double lua_Number;

/* Initialize the compiler state */
RAVICOMP_EXPORT struct compiler_state *raviX_init_compiler(void);
/* Destroy the compiler state */
RAVICOMP_EXPORT void raviX_destroy_compiler(struct compiler_state *compiler);

/* ------------------------ LEXICAL ANALYZER -------------------------------*/
/* This is derived from PuC Lua implementation                              */
enum TokenType {
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

/*
 * Lua strings can have embedded 0 bytes therefore we
 * need a string type that has a length associated with it.
 */
struct string_object {
	uint32_t len;	  /* length of the string */
	int32_t reserved; /* if is this a keyword then token id else -1 */
	uint32_t hash;	  /* hash value of the string */
	const char *str;  /* string data */
};

/*
 * Lua literals
 */
typedef union {
	lua_Number r;
	lua_Integer i;
	const struct string_object *ts;
} SemInfo; /* semantic information */

typedef struct Token {
	int token; /* Token value or character value */
	SemInfo seminfo;
} Token;

/*
 * Everything below should be treated as readonly
 */
typedef struct {
	int current;	 /* current character (char as int) */
	int linenumber;	 /* input line counter */
	int lastline;	 /* line of last token 'consumed' */
	Token t;	 /* current token */
	Token lookahead; /* look ahead token */
} LexState;

/* all strings are interned and stored in a hash set, strings may have embedded
 * 0 bytes therefore explicit length is necessary
 */
RAVICOMP_EXPORT const struct string_object *raviX_create_string(struct compiler_state *container, const char *s,
								uint32_t len);

/* Initialize lexical analyser. Takes as input a buffer containing Lua/Ravi source and the source name*/
RAVICOMP_EXPORT struct lexer_state *raviX_init_lexer(struct compiler_state *compiler_state, const char *buf,
						     size_t buflen, const char *source_name);
/* Gets the lexer data structure that can be used to access the current token */
RAVICOMP_EXPORT LexState *raviX_get_lexer_info(struct lexer_state *ls);
/* Retrieves the next token and saves it is LexState structure. If a lookahead was set then that is retrieved,
 else the next token is retrieved */
RAVICOMP_EXPORT void raviX_next(struct lexer_state *ls);
/* Retrieves the next token and sets it as the lookahead. This means that a next call will get the lookahead */
RAVICOMP_EXPORT int raviX_lookahead(struct lexer_state *ls);
/* Release all data structures used by the lexer */
RAVICOMP_EXPORT void raviX_destroy_lexer(struct lexer_state *);

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

/* Binary operators */
typedef enum BinaryOperatorType {
	BINOPR_ADD,
	BINOPR_SUB,
	BINOPR_MUL,
	BINOPR_MOD,
	BINOPR_POW,
	BINOPR_DIV,
	BINOPR_IDIV,
	BINOPR_BAND,
	BINOPR_BOR,
	BINOPR_BXOR,
	BINOPR_SHL,
	BINOPR_SHR,
	BINOPR_CONCAT,
	BINOPR_EQ,
	BINOPR_LT,
	BINOPR_LE,
	BINOPR_NE,
	BINOPR_GT,
	BINOPR_GE,
	BINOPR_AND,
	BINOPR_OR,
	BINOPR_NOBINOPR
} BinaryOperatorType;

/* Unary operators */
typedef enum UnaryOperatorType {
	UNOPR_MINUS,
	UNOPR_BNOT,
	UNOPR_NOT,
	UNOPR_LEN,
	UNOPR_TO_INTEGER,
	UNOPR_TO_NUMBER,
	UNOPR_TO_INTARRAY,
	UNOPR_TO_NUMARRAY,
	UNOPR_TO_TABLE,
	UNOPR_TO_STRING,
	UNOPR_TO_CLOSURE,
	UNOPR_TO_TYPE,
	UNOPR_NOUNOPR
} UnaryOperatorType;

enum ast_node_type {
	AST_NONE, /* Used when the node doesn't represent an AST such as test_then_block. */
	AST_RETURN_STMT,
	AST_GOTO_STMT,
	AST_LABEL_STMT,
	AST_DO_STMT,
	AST_LOCAL_STMT,
	AST_FUNCTION_STMT,
	AST_IF_STMT,
	AST_TEST_THEN_STMT,
	AST_WHILE_STMT,
	AST_FORIN_STMT,
	AST_FORNUM_STMT,
	AST_REPEAT_STMT,
	AST_EXPR_STMT, /* Also used for assignment statements */
	AST_LITERAL_EXPR,
	AST_SYMBOL_EXPR,
	AST_Y_INDEX_EXPR,	 /* [] operator */
	AST_FIELD_SELECTOR_EXPR, /* table field access - '.' or ':' operator */
	AST_INDEXED_ASSIGN_EXPR, /* table value assign in table constructor */
	AST_SUFFIXED_EXPR,
	AST_UNARY_EXPR,
	AST_BINARY_EXPR,
	AST_FUNCTION_EXPR, /* function literal */
	AST_TABLE_EXPR,	   /* table constructor */
	AST_FUNCTION_CALL_EXPR
};

struct statement;
struct return_statement;
struct label_statement;
struct goto_statement;
struct local_statement;
struct expression_statement;
struct function_statement;
struct do_statement;
struct test_then_statement;
struct if_statement;
struct while_or_repeat_statement;
struct for_statement;

struct expression;
struct literal_expression;
struct symbol_expression;
struct index_expression;
struct unary_expression;
struct binary_expression;
struct function_expression;
struct table_element_assignment_expression;
struct table_literal_expression;
struct suffixed_expression;
struct function_call_expression;

struct block_scope;

/* Types of symbols */
enum symbol_type {
	SYM_LOCAL,
	SYM_UPVALUE,
	SYM_GLOBAL, /* Global symbols are never added to a scope so they are always looked up */
	SYM_LABEL
};
struct lua_symbol;
struct lua_upvalue_symbol;
struct lua_local_symbol;
struct lua_label_symbol;

RAVICOMP_EXPORT enum ast_node_type raviX_statement_type(struct statement *statement);

RAVICOMP_EXPORT const struct function_expression *
raviX_ast_get_main_function(const struct compiler_state *compiler_state);

RAVICOMP_EXPORT const struct var_type *raviX_function_type(const struct function_expression *function_expression);
RAVICOMP_EXPORT bool raviX_function_is_vararg(const struct function_expression *function_expression);
RAVICOMP_EXPORT bool raviX_function_is_method(const struct function_expression *function_expression);
RAVICOMP_EXPORT const struct function_expression *
raviX_function_parent(const struct function_expression *function_expression);
RAVICOMP_EXPORT void
raviX_function_foreach_child(const struct function_expression *function_expression, void *userdata,
			     void (*callback)(void *userdata, const struct function_expression *function_expression));
RAVICOMP_EXPORT struct block_scope *raviX_function_scope(const struct function_expression *function_expression);
RAVICOMP_EXPORT void
raviX_function_foreach_statement(const struct function_expression *function_expression, void *userdata,
				 void (*callback)(void *userdata, const struct statement *statement));
RAVICOMP_EXPORT void raviX_function_foreach_argument(const struct function_expression *function_expression, void *userdata,
						     void (*callback)(void *userdata, const struct lua_local_symbol *symbol));
RAVICOMP_EXPORT void raviX_function_foreach_local(const struct function_expression *function_expression, void *userdata,
				  void (*callback)(void *userdata, const struct lua_local_symbol *lua_local_symbol));
RAVICOMP_EXPORT void raviX_function_foreach_upvalue(const struct function_expression *function_expression, void *userdata,
				    void (*callback)(void *userdata, const struct lua_upvalue_symbol *symbol));

RAVICOMP_EXPORT const struct string_object *raviX_local_symbol_name(const struct lua_local_symbol *lua_local_symbol);
RAVICOMP_EXPORT const struct var_type *raviX_local_symbol_type(const struct lua_local_symbol *lua_local_symbol);
RAVICOMP_EXPORT const struct block_scope *raviX_local_symbol_scope(const struct lua_local_symbol *lua_local_symbol);

RAVICOMP_EXPORT const struct return_statement * raviX_return_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct label_statement * raviX_label_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct goto_statement * raviX_goto_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct local_statement * raviX_local_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct expression_statement * raviX_expression_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct function_statement * raviX_function_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct do_statement * raviX_do_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct test_then_statement * raviX_test_then_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct if_statement * raviX_if_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct while_or_repeat_statement * raviX_while_or_repeat_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct for_statement * raviX_for_statement(const struct statement *stmt);


#endif
