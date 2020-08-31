/*
A compiler for Ravi and Lua 5.3. This is work in progress.
Once ready it will be used to create a JIT compiler for Ravi.

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
/* During compilation all data structures are stored in the compiler state */
RAVICOMP_EXPORT struct compiler_state *raviX_init_compiler(void);
/* Destroy the compiler state */
RAVICOMP_EXPORT void raviX_destroy_compiler(struct compiler_state *compiler);

/* ------------------------ LEXICAL ANALYZER API -------------------------------*/
/* This is derived from LuaJit implementation                              */
#define TKDEF(_, __, ___)                                                                                              \
    _(and) _(break) _(do) _(else) _(elseif) _(end) \
    _(false) _(for) _(function) _(goto) _(if) _(in) _(local) \
    _(defer) /* Ravi extension */ \
    _(nil) _(not) _(or) \
    _(repeat) _(return) _(then) _(true) _(until) _(while) \
    /* other terminal symbols */ \
    ___(IDIV, /) __(CONCAT, ..) __(DOTS, ...) __(EQ, ==) \
    __(GE, >=) __(LE, <=) __(NE, ~=) __(SHL, <<) \
    __(SHR, >>) __(DBCOLON, ::) \
    /** RAVI extensions */ \
    __(TO_INTEGER, @integer) __(TO_NUMBER, @number) \
    __(TO_INTARRAY, @integer[]) __(TO_NUMARRAY, @number[]) \
    __(TO_TABLE, @table) __(TO_STRING, @string) __(TO_CLOSURE, @closure) __(EOS, <eof>) \
    /* Tokens below this populate the seminfo */ \
    __(FLT, <number>) __(INT, <integer>) __(NAME, <name>) __(STRING, <string>)

enum TokenType {
  TOK_OFS = 256,
#define TKENUM1(name)		TOK_##name,
#define TKENUM2(name, sym)	TOK_##name,
#define TKENUM3(name, sym)	TOK_##name,
TKDEF(TKENUM1, TKENUM2, TKENUM3)
#undef TKENUM1
#undef TKENUM2
#undef TKENUM3
  FIRST_RESERVED = TOK_OFS +1,
  LAST_RESERVED = TOK_while - TOK_OFS
};

/*
 * Lua strings can have embedded 0 bytes therefore we
 * need a string type that has a length associated with it.
 *
 * The compiler stores a single copy of each string so that strings
 * can be compared by equality.
 */
struct string_object {
	uint32_t len;	  /* length of the string */
	int32_t reserved; /* if is this a keyword then token id else -1 */
	uint32_t hash;	  /* hash value of the string */
	const char *str;  /* string data */
	void* userdata; /* For use by host such as Ravi */
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
	SemInfo seminfo; /* Literal associated with the token, only valid when token is a literal or an identifier, i.e. token is > TOK_EOS */
} Token;

/*
 * Everything below should be treated as readonly
 */
typedef struct {
	int current;	 /* current character (char as int) */
	int linenumber;	 /* input line counter */
	int lastline;	 /* line of last token 'consumed' */
	Token t;	 /* current token, set after call to raviX_next() */
	Token lookahead; /* look ahead token, set after call to raviX_lookahead() */
} LexState;

/* Following is a dynamic buffer implementation that is not strictly part of the
 * compiler api but is relied upon by various compiler parts. We should perhaps avoid
 * exposing it.
 *
 * The reason for exposing this is that we use it for getting the token string in one of the
 * api calls.
 */
typedef struct {
	char *buf;
	size_t allocated_size;
	size_t pos;
} buffer_t;

/* all strings are interned and stored in a hash set, strings may have embedded
 * 0 bytes therefore explicit length is necessary
 */
RAVICOMP_EXPORT struct string_object *raviX_create_string(struct compiler_state *container, const char *s,
								uint32_t len);

/* Initialize lexical analyser. Takes as input a buffer containing Lua/Ravi source and the source name */
RAVICOMP_EXPORT struct lexer_state *raviX_init_lexer(struct compiler_state *compiler_state, const char *buf,
						     size_t buflen, const char *source_name);
/* Gets the lexer data structure that can be used to access the current token. Note that this is
 * a readonly data structure
 */
RAVICOMP_EXPORT const LexState *raviX_get_lexer_info(struct lexer_state *ls);
/* Retrieves the next token and saves it is LexState structure. If a lookahead was set then that is retrieved
 * (and reset to EOS) else the next token is retrieved
 */
RAVICOMP_EXPORT void raviX_next(struct lexer_state *ls);
/* Retrieves the next token and sets it as the lookahead. This means that a next call will get the lookahead.
 * Returns the token id.
 */
RAVICOMP_EXPORT int raviX_lookahead(struct lexer_state *ls);
/* Convert a token to text format. The token will be written to current position in mb. */
RAVICOMP_EXPORT void raviX_token2str(int token, buffer_t *mb);
/* Release all data structures used by the lexer */
RAVICOMP_EXPORT void raviX_destroy_lexer(struct lexer_state *);

/* ---------------- PARSER API -------------------------- */

/*
 * Parse a Lua chunk (i.e. script).
 * The Lua chunk will be wrapped in an anonymous Lua function (the 'main' function), so all the code
 * in the chunk will be part of that function. Any functions defined in the chunk will become child functions
 * of the 'main' function.
 *
 * Each Lua chunk / script therefore has an anonymous 'main' function. The name 'main' is just to refer
 * to this function as it has no name in reality.
 *
 * Note that at present a new compiler state should be created when processing a Lua chunk.
 *
 * Returns 0 on success, non-zero on failure.
 */
RAVICOMP_EXPORT int raviX_parse(struct compiler_state *compiler_state, const char *buffer, size_t buflen, const char *name);
/* Prints out the AST to the file */
RAVICOMP_EXPORT void raviX_output_ast(struct compiler_state *compiler_state, FILE *fp);
/* Performs type checks on the AST and annotates types of expressions nad variables where possible.
 * As a result the AST will be modified.
 *
 * Returns 0 on success, non-zero on failure.
 */
RAVICOMP_EXPORT int
raviX_ast_typecheck(struct compiler_state *compiler_state); /* Perform type checks and assign types to AST */

/* ---------------------------- LINEARIZER API --------------------------------------- */
/* linear IR generator.
 * The goal of this component is to convert the AST to a linear IR.
 * This is work in progress, therefore the IR is not yet publicly exposed.
 */
RAVICOMP_EXPORT struct linearizer_state *raviX_init_linearizer(struct compiler_state *compiler_state);
/* Attempts to create linear IR for given AST.
 * Returns 0 on success.
 */
RAVICOMP_EXPORT int raviX_ast_linearize(struct linearizer_state *linearizer);
/* Prints out the content of the linear IR */
RAVICOMP_EXPORT void raviX_output_linearizer(struct linearizer_state *linearizer, FILE *fp);
RAVICOMP_EXPORT void raviX_destroy_linearizer(struct linearizer_state *linearizer);

/* utilies */
RAVICOMP_EXPORT const char *raviX_get_last_error(struct compiler_state *compiler_state);

/* ----------------------- AST WALKING API ------------------------ */

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
	UNOPR_MINUS = BINOPR_NOBINOPR+1,
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

/* Types of AST nodes */
enum ast_node_type {
	AST_NONE, /* Will never be set on a properly initialized node */
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
	EXPR_TABLE_ELEMENT_ASSIGN, /* table element assignment in table constructor */
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

/* As described before each parsed Lua script or chunk is wrapped in an anonymous 'main'
 * function hence the AST root is this function.
 */
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
/* Lua if statements are a mix of select/case and if/else statements in
 * other languages. The AST represents the initial if condition block and all subsequent
 * elseif blocks as test_then_statments. The final else block is treated as an optional
 * else block.
 */
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
/* Note: ... value has type RAVI_TVARARGS and no associated SemInfo. */
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

/* Utilities */
#ifdef __GNUC__
#define FORMAT_ATTR(pos) __attribute__((__format__(__printf__, pos, pos + 1)))
#else
#define FORMAT_ATTR(pos)
#endif

RAVICOMP_EXPORT void raviX_buffer_init(buffer_t *mb, size_t initial_size);
RAVICOMP_EXPORT void raviX_buffer_resize(buffer_t *mb, size_t new_size);
RAVICOMP_EXPORT void raviX_buffer_reserve(buffer_t *mb, size_t n);
RAVICOMP_EXPORT void raviX_buffer_free(buffer_t *mb);
static inline char *raviX_buffer_data(const buffer_t *mb) { return mb->buf; }
static inline size_t raviX_buffer_size(const buffer_t *mb) { return mb->allocated_size; }
static inline size_t raviX_buffer_len(const buffer_t *mb) { return mb->pos; }
static inline void raviX_buffer_reset(buffer_t *mb) { mb->pos = 0; }

/* following convert input to string before adding */
RAVICOMP_EXPORT void raviX_buffer_add_string(buffer_t *mb, const char *str);
RAVICOMP_EXPORT void raviX_buffer_add_bytes(buffer_t *mb, const char *str, size_t len);
RAVICOMP_EXPORT void raviX_buffer_add_fstring(buffer_t *mb, const char *str, ...) FORMAT_ATTR(2);

/* strncpy() replacement with guaranteed 0 termination */
RAVICOMP_EXPORT void raviX_string_copy(char *buf, const char *src, size_t buflen);

#endif
