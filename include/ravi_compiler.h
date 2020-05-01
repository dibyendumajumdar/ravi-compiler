/*
A compiler for Ravi and Lua 5.3. This is work in progress.
Once ready it will be used to create a JIT compiler for Ravi.

This header file defines the public api

Copyright 2018-2020 Dibyendu Majumdar
*/

#ifndef ravicomp_COMPILER_H
#define ravicomp_COMPILER_H

#include "port.h"
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
/* During compilation all data structures are stored in the compiler state */
RAVICOMP_EXPORT struct compiler_state *raviX_init_compiler(void);
/* Destroy the compiler state */
RAVICOMP_EXPORT void raviX_destroy_compiler(struct compiler_state *compiler);

/* ------------------------ LEXICAL ANALYZER -------------------------------*/
/* This is derived from PuC Lua implementation                              */
enum TokenType {
	/* The reserved word tokens start from 257 because code 1 to 256 are used for standard character tokens */
	FIRST_RESERVED = 257,
	TOK_AND = FIRST_RESERVED,
	TOK_BREAK,
	TOK_DO,
	TOK_ELSE,
	TOK_ELSEIF,
	TOK_END,
	TOK_FALSE,
	TOK_FOR,
	TOK_FUNCTION,
	TOK_GOTO,
	TOK_IF,
	TOK_IN,
	TOK_LOCAL,
	TOK_DEFER, /* Ravi extension */
	TOK_NIL,
	TOK_NOT,
	TOK_OR,
	TOK_REPEAT,
	TOK_RETURN,
	TOK_THEN,
	TOK_TRUE,
	TOK_UNTIL,
	TOK_WHILE,
	/* other terminal symbols */
	TOK_IDIV,
	TOK_CONCAT,
	TOK_DOTS,
	TOK_EQ,
	TOK_GE,
	TOK_LE,
	TOK_NE,
	TOK_SHL,
	TOK_SHR,
	TOK_DBCOLON,
	TOK_EOS,
	TOK_FLT,
	TOK_INT,
	TOK_NAME,
	TOK_STRING,
	/** RAVI extensions */
	TOK_TO_INTEGER,
	TOK_TO_NUMBER,
	TOK_TO_INTARRAY,
	TOK_TO_NUMARRAY,
	TOK_TO_TABLE,
	TOK_TO_STRING,
	TOK_TO_CLOSURE
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
	int token; /* Token value or character value; token values start from FIRST_RESERVED which is 257 */
	SemInfo seminfo; /* Literal associated with the token */
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

typedef struct {
	char *buf;
	size_t allocated_size;
	size_t pos;
} membuff_t;

RAVICOMP_EXPORT void raviX_buffer_init(membuff_t *mb, size_t initial_size);
RAVICOMP_EXPORT void raviX_buffer_resize(membuff_t *mb, size_t new_size);
RAVICOMP_EXPORT void raviX_buffer_reserve(membuff_t *mb, size_t n);
RAVICOMP_EXPORT void raviX_buffer_free(membuff_t *mb);
static inline char *raviX_buffer_data(membuff_t *mb) { return mb->buf; }
static inline size_t raviX_buffer_size(membuff_t *mb) { return mb->allocated_size; }
static inline size_t raviX_buffer_len(membuff_t *mb) { return mb->pos; }
static inline void raviX_buffer_reset(membuff_t *mb) { mb->pos = 0; }

/* following convert input to string before adding */
RAVICOMP_EXPORT void raviX_buffer_add_string(membuff_t *mb, const char *str);
RAVICOMP_EXPORT void raviX_buffer_add_fstring(membuff_t *mb, const char *str, ...) FORMAT_ATTR(2);

/* strncpy() replacement with guaranteed 0 termination */
RAVICOMP_EXPORT void raviX_string_copy(char *buf, const char *src, size_t buflen);

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
/* Convert a token to text format */
RAVICOMP_EXPORT void raviX_token2str(int token, membuff_t *mb);
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
	STMT_RETURN,
	STMT_GOTO,
	STMT_LABEL,
	STMT_DO,
	STMT_LOCAL,
	STMT_FUNCTION,
	STMT_IF,
	STMT_TEST_THEN,
	STMT_WHILE,
	STMT_FOR_IN,
	STMT_FOR_NUM,
	STMT_REPEAT,
	STMT_EXPR, /* Also used for assignment statements */
	EXPR_LITERAL,
	EXPR_SYMBOL,
	EXPR_Y_INDEX,	 /* [] operator */
	EXPR_FIELD_SELECTOR, /* table field access - '.' or ':' operator */
	EXPR_TABLE_ELEMENT_ASSIGN, /* table value assign in table constructor */
	EXPR_SUFFIXED,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_FUNCTION, /* function literal */
	EXPR_TABLE_LITERAL,	   /* table constructor */
	EXPR_FUNCTION_CALL
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
	SYM_LOCAL,   /* lua_variable_symbol */
	SYM_UPVALUE, /* lua_upvalue_symbol */
	SYM_GLOBAL,  /* lua_variable_symbol, Global symbols are never added to a scope so they are always looked up */
	SYM_LABEL    /* lua_label_symbol */
};
struct lua_symbol;
struct lua_upvalue_symbol;
struct lua_variable_symbol;
struct lua_label_symbol;

RAVICOMP_EXPORT const struct function_expression *
raviX_ast_get_main_function(const struct compiler_state *compiler_state);

/* return statement walking */
RAVICOMP_EXPORT void raviX_return_statement_foreach_expression(const struct return_statement *statement, void *userdata,
							       void (*callback)(void *, const struct expression *expr));

/* label statement walking */
RAVICOMP_EXPORT const struct string_object *raviX_label_statement_label_name(const struct label_statement *statement);
RAVICOMP_EXPORT const struct block_scope *raviX_label_statement_label_scope(const struct label_statement *statement);

/* goto statement walking */
RAVICOMP_EXPORT const struct string_object *raviX_goto_statement_label_name(const struct goto_statement *statement);
RAVICOMP_EXPORT const struct block_scope *raviX_goto_statement_scope(const struct goto_statement *statement);
RAVICOMP_EXPORT bool raviX_goto_statement_is_break(const struct goto_statement *statement);

/* local statement walking */
RAVICOMP_EXPORT void raviX_local_statement_foreach_expression(const struct local_statement *statement, void *userdata,
							      void (*callback)(void *, const struct expression *expr));
RAVICOMP_EXPORT void raviX_local_statement_foreach_symbol(const struct local_statement *statement, void *userdata,
							  void (*callback)(void *,
									   const struct lua_variable_symbol *expr));

/* expression or assignment statement walking */
RAVICOMP_EXPORT void
raviX_expression_statement_foreach_lhs_expression(const struct expression_statement *statement, void *userdata,
						  void (*callback)(void *, const struct expression *expr));
RAVICOMP_EXPORT void
raviX_expression_statement_foreach_rhs_expression(const struct expression_statement *statement, void *userdata,
						  void (*callback)(void *, const struct expression *expr));

/* function statement walking */
RAVICOMP_EXPORT const struct symbol_expression *
raviX_function_statement_name(const struct function_statement *statement);
RAVICOMP_EXPORT bool raviX_function_statement_is_method(const struct function_statement *statement);
RAVICOMP_EXPORT const struct index_expression *
raviX_function_statement_method_name(const struct function_statement *statement);
RAVICOMP_EXPORT bool raviX_function_statement_has_selectors(const struct function_statement *statement);
RAVICOMP_EXPORT void
raviX_function_statement_foreach_selector(const struct function_statement *statement, void *userdata,
					  void (*callback)(void *, const struct index_expression *expr));
RAVICOMP_EXPORT const struct function_expression *raviX_function_ast(const struct function_statement *statement);

/* do statement walking */
RAVICOMP_EXPORT const struct block_scope *raviX_do_statement_scope(const struct do_statement *statement);
RAVICOMP_EXPORT void raviX_do_statement_foreach_statement(const struct do_statement *statement, void *userdata,
							  void (*callback)(void *userdata,
									   const struct statement *statement));
/* if statement walking */
RAVICOMP_EXPORT void
raviX_if_statement_foreach_test_then_statement(const struct if_statement *statement, void *userdata,
					       void (*callback)(void *, const struct test_then_statement *stmt));
RAVICOMP_EXPORT const struct block_scope *raviX_if_then_statement_else_scope(const struct if_statement *statement);
RAVICOMP_EXPORT void raviX_if_statement_foreach_else_statement(const struct if_statement *statement, void *userdata,
							       void (*callback)(void *userdata,
										const struct statement *statement));
RAVICOMP_EXPORT const struct block_scope *raviX_test_then_statement_scope(const struct test_then_statement *statement);
RAVICOMP_EXPORT void
raviX_test_then_statement_foreach_statement(const struct test_then_statement *statement, void *userdata,
					   void (*callback)(void *userdata, const struct statement *statement));
RAVICOMP_EXPORT const struct expression *
raviX_test_then_statement_condition(const struct test_then_statement *statement);

/* while or repeat statement walking */
RAVICOMP_EXPORT const struct expression *
raviX_while_or_repeat_statement_condition(const struct while_or_repeat_statement *statement);
RAVICOMP_EXPORT const struct block_scope *
raviX_while_or_repeat_statement_scope(const struct while_or_repeat_statement *statement);
RAVICOMP_EXPORT void
raviX_while_or_repeat_statement_foreach_statement(const struct while_or_repeat_statement *statement, void *userdata,
						  void (*callback)(void *userdata, const struct statement *statement));

/* for statement walking */
RAVICOMP_EXPORT const struct block_scope *raviX_for_statement_scope(const struct for_statement *statement);
RAVICOMP_EXPORT void raviX_for_statement_foreach_symbol(const struct for_statement *statement, void *userdata,
							void (*callback)(void *,
									 const struct lua_variable_symbol *expr));
RAVICOMP_EXPORT void raviX_for_statement_foreach_expression(const struct for_statement *statement, void *userdata,
							    void (*callback)(void *, const struct expression *expr));
RAVICOMP_EXPORT const struct block_scope *raviX_for_statement_body_scope(const struct for_statement *statement);
RAVICOMP_EXPORT void raviX_for_statement_body_foreach_statement(const struct for_statement *statement, void *userdata,
								void (*callback)(void *userdata,
										 const struct statement *statement));

/* literal expression */
RAVICOMP_EXPORT const struct var_type *raviX_literal_expression_type(const struct literal_expression *expression);
RAVICOMP_EXPORT const SemInfo *raviX_literal_expression_literal(const struct literal_expression *expression);

/* symbol expression */
RAVICOMP_EXPORT const struct var_type *raviX_symbol_expression_type(const struct symbol_expression *expression);
RAVICOMP_EXPORT const struct lua_symbol *raviX_symbol_expression_symbol(const struct symbol_expression *expression);

/* index expression */
RAVICOMP_EXPORT const struct var_type *raviX_index_expression_type(const struct index_expression *expression);
RAVICOMP_EXPORT const struct expression *raviX_index_expression_expression(const struct index_expression *expression);

/* unary expression */
RAVICOMP_EXPORT const struct var_type *raviX_unary_expression_type(const struct unary_expression *expression);
RAVICOMP_EXPORT const struct expression *raviX_unary_expression_expression(const struct unary_expression *expression);
RAVICOMP_EXPORT UnaryOperatorType raviX_unary_expression_operator(const struct unary_expression *expression);

/* binary expression */
RAVICOMP_EXPORT const struct var_type *raviX_binary_expression_type(const struct binary_expression *expression);
RAVICOMP_EXPORT const struct expression *
raviX_binary_expression_left_expression(const struct binary_expression *expression);
RAVICOMP_EXPORT const struct expression *
raviX_binary_expression_right_expression(const struct binary_expression *expression);
RAVICOMP_EXPORT BinaryOperatorType raviX_binary_expression_operator(const struct binary_expression *expression);

/* function expression */
RAVICOMP_EXPORT const struct var_type *raviX_function_type(const struct function_expression *function_expression);
RAVICOMP_EXPORT bool raviX_function_is_vararg(const struct function_expression *function_expression);
RAVICOMP_EXPORT bool raviX_function_is_method(const struct function_expression *function_expression);
RAVICOMP_EXPORT const struct function_expression *
raviX_function_parent(const struct function_expression *function_expression);
RAVICOMP_EXPORT void
raviX_function_foreach_child(const struct function_expression *function_expression, void *userdata,
			     void (*callback)(void *userdata, const struct function_expression *function_expression));
RAVICOMP_EXPORT const struct block_scope *raviX_function_scope(const struct function_expression *function_expression);
RAVICOMP_EXPORT void
raviX_function_foreach_statement(const struct function_expression *function_expression, void *userdata,
				 void (*callback)(void *userdata, const struct statement *statement));
RAVICOMP_EXPORT void
raviX_function_foreach_argument(const struct function_expression *function_expression, void *userdata,
				void (*callback)(void *userdata, const struct lua_variable_symbol *symbol));
RAVICOMP_EXPORT void raviX_function_foreach_local(const struct function_expression *function_expression, void *userdata,
						  void (*callback)(void *userdata,
								   const struct lua_variable_symbol *lua_local_symbol));
RAVICOMP_EXPORT void
raviX_function_foreach_upvalue(const struct function_expression *function_expression, void *userdata,
			       void (*callback)(void *userdata, const struct lua_upvalue_symbol *symbol));

/* table element assignment expression */
RAVICOMP_EXPORT const struct var_type *
raviX_table_element_assignment_expression_type(const struct table_element_assignment_expression *expression);
RAVICOMP_EXPORT const struct expression *
raviX_table_element_assignment_expression_key(const struct table_element_assignment_expression *expression);
RAVICOMP_EXPORT const struct expression *
raviX_table_element_assignment_expression_value(const struct table_element_assignment_expression *expression);

/* table_literal_expression */
RAVICOMP_EXPORT const struct var_type *
raviX_table_literal_expression_type(const struct table_literal_expression *expression);
RAVICOMP_EXPORT void raviX_table_literal_expression_foreach_element(
    const struct table_literal_expression *expression, void *userdata,
    void (*callback)(void *, const struct table_element_assignment_expression *expr));

/* suffixed_expression */
RAVICOMP_EXPORT const struct var_type *raviX_suffixed_expression_type(const struct suffixed_expression *expression);
RAVICOMP_EXPORT const struct expression *
raviX_suffixed_expression_primary(const struct suffixed_expression *expression);
RAVICOMP_EXPORT void raviX_suffixed_expression_foreach_suffix(const struct suffixed_expression *expression,
							      void *userdata,
							      void (*callback)(void *, const struct expression *expr));

/* function call expression */
RAVICOMP_EXPORT const struct var_type *
raviX_function_call_expression_type(const struct function_call_expression *expression);
// can return NULL
RAVICOMP_EXPORT const struct string_object *
raviX_function_call_expression_method_name(const struct function_call_expression *expression);
RAVICOMP_EXPORT void
raviX_function_call_expression_foreach_argument(const struct function_call_expression *expression, void *userdata,
						void (*callback)(void *, const struct expression *expr));

/* Convert a statement to the correct type */
RAVICOMP_EXPORT enum ast_node_type raviX_statement_type(const struct statement *statement);
RAVICOMP_EXPORT const struct return_statement *raviX_return_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct label_statement *raviX_label_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct goto_statement *raviX_goto_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct local_statement *raviX_local_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct expression_statement *raviX_expression_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct function_statement *raviX_function_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct do_statement *raviX_do_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct test_then_statement *raviX_test_then_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct if_statement *raviX_if_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct while_or_repeat_statement *raviX_while_or_repeat_statement(const struct statement *stmt);
RAVICOMP_EXPORT const struct for_statement *raviX_for_statement(const struct statement *stmt);

/* Convert an expression to the correct type */
RAVICOMP_EXPORT enum ast_node_type raviX_expression_type(const struct expression *expression);
RAVICOMP_EXPORT const struct literal_expression *raviX_literal_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct symbol_expression *raviX_symbol_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct index_expression *raviX_index_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct unary_expression *raviX_unary_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct binary_expression *raviX_binary_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct function_expression *raviX_function_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct table_element_assignment_expression *
raviX_table_element_assignment_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct table_literal_expression *raviX_table_literal_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct suffixed_expression *raviX_suffixed_expression(const struct expression *expr);
RAVICOMP_EXPORT const struct function_call_expression *raviX_function_call_expression(const struct expression *expr);

RAVICOMP_EXPORT const struct function_expression *raviX_scope_owning_function(const struct block_scope *scope);
RAVICOMP_EXPORT const struct block_scope *raviX_scope_parent_scope(const struct block_scope *scope);
RAVICOMP_EXPORT void raviX_scope_foreach_symbol(const struct block_scope *scope, void *userdata,
						void (*callback)(void *userdata, const struct lua_symbol *symbol));

RAVICOMP_EXPORT enum symbol_type raviX_symbol_type(const struct lua_symbol *symbol);
/* symbol downcast */
RAVICOMP_EXPORT const struct lua_variable_symbol *raviX_symbol_variable(const struct lua_symbol *symbol);
RAVICOMP_EXPORT const struct lua_upvalue_symbol *raviX_symbol_upvalue(const struct lua_symbol *symbol);
RAVICOMP_EXPORT const struct lua_label_symbol *raviX_symbol_label(const struct lua_symbol *symbol);

/* variable symbol - local and global variables */
RAVICOMP_EXPORT const struct string_object *
raviX_variable_symbol_name(const struct lua_variable_symbol *lua_local_symbol);
RAVICOMP_EXPORT const struct var_type *raviX_variable_symbol_type(const struct lua_variable_symbol *lua_local_symbol);
// NULL if global
RAVICOMP_EXPORT const struct block_scope *
raviX_variable_symbol_scope(const struct lua_variable_symbol *lua_local_symbol);

/* label symbol */
RAVICOMP_EXPORT const struct string_object *raviX_label_name(const struct lua_label_symbol *symbol);
RAVICOMP_EXPORT const struct block_scope *raviX_label_scope(const struct lua_label_symbol *symbol);

/* upvalue symbol */
RAVICOMP_EXPORT const struct var_type *raviX_upvalue_symbol_type(const struct lua_upvalue_symbol *symbol);
RAVICOMP_EXPORT const struct lua_variable_symbol *raviX_upvalue_target_variable(const struct lua_upvalue_symbol *symbol);
RAVICOMP_EXPORT const struct function_expression *raviX_upvalue_target_function(const struct lua_upvalue_symbol *symbol);
RAVICOMP_EXPORT unsigned raviX_upvalue_index(const struct lua_upvalue_symbol *symbol);


#endif
