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

typedef struct CompilerState CompilerState;
typedef struct LexerState LexerState;
typedef struct LinearizerState LinearizerState;

typedef long long lua_Integer;
typedef double lua_Number;

/* Initialize the compiler state */
/* During compilation all data structures are stored in the compiler state */
RAVICOMP_EXPORT CompilerState *raviX_init_compiler(void);
/* Destroy the compiler state */
RAVICOMP_EXPORT void raviX_destroy_compiler(CompilerState *compiler);

/* ------------------------ LEXICAL ANALYZER API -------------------------------*/
/* Note: following enum was generate using utils/tokenenum.h                               */
enum TokenType {
	TOK_OFS = 256,

	TOK_and,
	TOK_break,
	TOK_do,
	TOK_else,
	TOK_elseif,
	TOK_end,
	TOK_false,
	TOK_for,
	TOK_function,
	TOK_goto,
	TOK_if,
	TOK_in,
	TOK_local,
	TOK_defer,
	TOK_nil,
	TOK_not,
	TOK_or,
	TOK_repeat,
	TOK_return,
	TOK_then,
	TOK_true,
	TOK_until,
	TOK_while,
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
	TOK_TO_INTEGER,
	TOK_TO_NUMBER,
	TOK_TO_INTARRAY,
	TOK_TO_NUMARRAY,
	TOK_TO_TABLE,
	TOK_TO_STRING,
	TOK_TO_CLOSURE,
	TOK_EOS,
	TOK_FLT,
	TOK_INT,
	TOK_NAME,
	TOK_STRING,

	FIRST_RESERVED = TOK_OFS + 1,
	LAST_RESERVED = TOK_while - TOK_OFS
};

/*
 * Lua strings can have embedded 0 bytes therefore we
 * need a string type that has a length associated with it.
 *
 * The compiler stores a single copy of each string so that strings
 * can be compared by equality.
 */
typedef struct StringObject {
	uint32_t len;	  /* length of the string */
	int32_t reserved; /* if is this a keyword then token id else -1 */
	uint32_t hash;	  /* hash value of the string */
	const char *str;  /* string data */
} StringObject;

/*
 * Lua literals
 */
typedef union {
	lua_Number r;
	lua_Integer i;
	const StringObject *ts;
} SemInfo;

typedef struct Token {
	int token; /* Token value or character value; token values start from FIRST_RESERVED which is 257, values < 256
		      are characters */
	SemInfo seminfo; /* Literal associated with the token, only valid when token is a literal or an identifier, i.e.
			    token is > TOK_EOS */
} Token;

/*
 * Everything below should be treated as readonly; for efficiency these fields are exposed, however treat them
 * as fields managed by the lexer.
 */
typedef struct {
	int current;	 /* current character (char value as int) */
	int linenumber;	 /* current input line counter */
	int lastline;	 /* line number of the last token 'consumed' */
	Token t;	 /* current token, set after call to raviX_next() */
	Token lookahead; /* look ahead token, set after call to raviX_lookahead() */
} LexerInfo;

/* Following is a dynamic buffer implementation that is not strictly part of the
 * compiler api but is relied upon by various compiler parts. We should perhaps avoid
 * exposing it.
 *
 * The reason for exposing this is that we use it for getting the token string in one of the
 * api calls.
 */
typedef struct {
	char *buf;	 /* pointer to allocated memory, can be reallocated */
	size_t capacity; /* allocated size */
	size_t pos;	 /* current position in the buffer */
} TextBuffer;

/* all strings are interned and stored in a hash set, strings may have embedded
 * 0 bytes therefore explicit length is necessary
 */
RAVICOMP_EXPORT const StringObject *raviX_create_string(CompilerState *compiler_state, const char *s,
								uint32_t len);

/* Initialize lexical analyser. Takes as input a buffer containing Lua/Ravi source and the source name */
RAVICOMP_EXPORT LexerState *raviX_init_lexer(CompilerState *compiler_state, const char *buf,
						     size_t buflen, const char *source_name);
/* Gets the public part of the lexer data structure to allow access the current token. Note that the returned
 * value should be treated as readonly data structure
 */
RAVICOMP_EXPORT const LexerInfo *raviX_get_lexer_info(LexerState *ls);
/* Retrieves the next token and saves it is LexState structure. If a lookahead was set then that is retrieved
 * (and reset to EOS) else the next token is retrieved
 */
RAVICOMP_EXPORT void raviX_next(LexerState *ls);
/* Retrieves the next token and sets it as the lookahead. This means that a next call will get the lookahead.
 * Returns the token id.
 */
RAVICOMP_EXPORT int raviX_lookahead(LexerState *ls);
/* Convert a token to text format. The token will be written to current position in mb. */
RAVICOMP_EXPORT void raviX_token2str(int token, TextBuffer *mb);
/* Release all data structures used by the lexer */
RAVICOMP_EXPORT void raviX_destroy_lexer(LexerState *);

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
RAVICOMP_EXPORT int raviX_parse(CompilerState *compiler_state, const char *buffer, size_t buflen,
				const char *source_name);
/* Prints out the AST to the file */
RAVICOMP_EXPORT void raviX_output_ast(CompilerState *compiler_state, FILE *fp);
/* Performs type checks on the AST and annotates types of expressions nad variables where possible.
 * As a result the AST will be modified.
 *
 * Returns 0 on success, non-zero on failure.
 */
RAVICOMP_EXPORT int
raviX_ast_typecheck(CompilerState *compiler_state); /* Perform type checks and assign types to AST */

/* ---------------------------- LINEARIZER API --------------------------------------- */
/* linear IR generator.
 * The goal of this component is to convert the AST to a linear IR.
 * This is work in progress, therefore the IR is not yet publicly exposed.
 */
RAVICOMP_EXPORT LinearizerState *raviX_init_linearizer(CompilerState *compiler_state);
/* Attempts to create linear IR for given AST.
 * Returns 0 on success.
 */
RAVICOMP_EXPORT int raviX_ast_linearize(LinearizerState *linearizer);
/* Prints out the content of the linear IR */
RAVICOMP_EXPORT void raviX_output_linearizer(LinearizerState *linearizer, FILE *fp);
/* Cleanup the linearizer */
RAVICOMP_EXPORT void raviX_destroy_linearizer(LinearizerState *linearizer);

/* utilies */
RAVICOMP_EXPORT const char *raviX_get_last_error(CompilerState *compiler_state);

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

RAVICOMP_EXPORT const char *raviX_get_binary_opr_str(BinaryOperatorType op);

/* Unary operators */
typedef enum UnaryOperatorType {
	UNOPR_MINUS = BINOPR_NOBINOPR + 1,
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

RAVICOMP_EXPORT const char *raviX_get_unary_opr_str(UnaryOperatorType op);

/* Types of AST nodes */
enum AstNodeType {
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
	EXPR_Y_INDEX,		   /* [] operator */
	EXPR_FIELD_SELECTOR,	   /* table field access - '.' or ':' operator */
	EXPR_TABLE_ELEMENT_ASSIGN, /* table element assignment in table constructor */
	EXPR_SUFFIXED,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_FUNCTION,	    /* function literal */
	EXPR_TABLE_LITERAL, /* table constructor */
	EXPR_FUNCTION_CALL
};

typedef struct Statement Statement;
typedef struct ReturnStatement ReturnStatement;
typedef struct LabelStatement LabelStatement;
typedef struct GotoStatement GotoStatement;
typedef struct LocalStatement LocalStatement;
typedef struct ExpressionStatement ExpressionStatement;
typedef struct FunctionStatement FunctionStatement;
typedef struct DoStatement DoStatement;
typedef struct TestThenStatement TestThenStatement;
typedef struct IfStatement IfStatement;
typedef struct WhileOrRepeatStatement WhileOrRepeatStatement;
typedef struct ForStatement ForStatement;

struct expression;
typedef struct LiteralExpression LiteralExpression;
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
	SYM_LABEL,   /* lua_label_symbol */
	SYM_ENV	     /* Special symbol type for _ENV */
};
struct lua_symbol;
struct lua_upvalue_symbol;
struct lua_variable_symbol;
struct lua_label_symbol;

/* As described before each parsed Lua script or chunk is wrapped in an anonymous 'main'
 * function hence the AST root is this function.
 */
RAVICOMP_EXPORT const struct function_expression *
raviX_ast_get_main_function(const CompilerState *compiler_state);

/* return statement walking */
RAVICOMP_EXPORT void raviX_return_statement_foreach_expression(const ReturnStatement *statement, void *userdata,
							       void (*callback)(void *, const struct expression *expr));

/* label statement walking */
RAVICOMP_EXPORT const StringObject *raviX_label_statement_label_name(const LabelStatement *statement);
RAVICOMP_EXPORT const struct block_scope *raviX_label_statement_label_scope(const LabelStatement *statement);

/* goto statement walking */
RAVICOMP_EXPORT const StringObject *raviX_goto_statement_label_name(const GotoStatement *statement);
RAVICOMP_EXPORT const struct block_scope *raviX_goto_statement_scope(const GotoStatement *statement);
RAVICOMP_EXPORT bool raviX_goto_statement_is_break(const GotoStatement *statement);

/* local statement walking */
RAVICOMP_EXPORT void raviX_local_statement_foreach_expression(const LocalStatement *statement, void *userdata,
							      void (*callback)(void *, const struct expression *expr));
RAVICOMP_EXPORT void raviX_local_statement_foreach_symbol(const LocalStatement *statement, void *userdata,
							  void (*callback)(void *,
									   const struct lua_variable_symbol *expr));

/* expression or assignment statement walking */
RAVICOMP_EXPORT void
raviX_expression_statement_foreach_lhs_expression(const ExpressionStatement *statement, void *userdata,
						  void (*callback)(void *, const struct expression *expr));
RAVICOMP_EXPORT void
raviX_expression_statement_foreach_rhs_expression(const ExpressionStatement *statement, void *userdata,
						  void (*callback)(void *, const struct expression *expr));

/* function statement walking */
RAVICOMP_EXPORT const struct symbol_expression *
raviX_function_statement_name(const FunctionStatement *statement);
RAVICOMP_EXPORT bool raviX_function_statement_is_method(const FunctionStatement *statement);
RAVICOMP_EXPORT const struct index_expression *
raviX_function_statement_method_name(const FunctionStatement *statement);
RAVICOMP_EXPORT bool raviX_function_statement_has_selectors(const FunctionStatement *statement);
RAVICOMP_EXPORT void
raviX_function_statement_foreach_selector(const FunctionStatement *statement, void *userdata,
					  void (*callback)(void *, const struct index_expression *expr));
RAVICOMP_EXPORT const struct function_expression *raviX_function_ast(const FunctionStatement *statement);

/* do statement walking */
RAVICOMP_EXPORT const struct block_scope *raviX_do_statement_scope(const DoStatement *statement);
RAVICOMP_EXPORT void raviX_do_statement_foreach_statement(const DoStatement *statement, void *userdata,
							  void (*callback)(void *userdata,
									   const Statement *statement));
/* if statement walking */
/* Lua if statements are a mix of select/case and if/else statements in
 * other languages. The AST represents the initial if condition block and all subsequent
 * elseif blocks as test_then_statments. The final else block is treated as an optional
 * else block.
 */
RAVICOMP_EXPORT void
raviX_if_statement_foreach_test_then_statement(const IfStatement *statement, void *userdata,
					       void (*callback)(void *, const TestThenStatement *stmt));
RAVICOMP_EXPORT const struct block_scope *raviX_if_then_statement_else_scope(const IfStatement *statement);
RAVICOMP_EXPORT void raviX_if_statement_foreach_else_statement(const IfStatement *statement, void *userdata,
							       void (*callback)(void *userdata,
										const Statement *statement));
RAVICOMP_EXPORT const struct block_scope *raviX_test_then_statement_scope(const TestThenStatement *statement);
RAVICOMP_EXPORT void
raviX_test_then_statement_foreach_statement(const TestThenStatement *statement, void *userdata,
					    void (*callback)(void *userdata, const Statement *statement));
RAVICOMP_EXPORT const struct expression *
raviX_test_then_statement_condition(const TestThenStatement *statement);

/* while or repeat statement walking */
RAVICOMP_EXPORT const struct expression *
raviX_while_or_repeat_statement_condition(const WhileOrRepeatStatement *statement);
RAVICOMP_EXPORT const struct block_scope *
raviX_while_or_repeat_statement_scope(const WhileOrRepeatStatement *statement);
RAVICOMP_EXPORT void
raviX_while_or_repeat_statement_foreach_statement(const WhileOrRepeatStatement *statement, void *userdata,
						  void (*callback)(void *userdata, const Statement *statement));

/* for statement walking */
RAVICOMP_EXPORT const struct block_scope *raviX_for_statement_scope(const ForStatement *statement);
RAVICOMP_EXPORT void raviX_for_statement_foreach_symbol(const ForStatement *statement, void *userdata,
							void (*callback)(void *,
									 const struct lua_variable_symbol *expr));
RAVICOMP_EXPORT void raviX_for_statement_foreach_expression(const ForStatement *statement, void *userdata,
							    void (*callback)(void *, const struct expression *expr));
RAVICOMP_EXPORT const struct block_scope *raviX_for_statement_body_scope(const ForStatement *statement);
RAVICOMP_EXPORT void raviX_for_statement_body_foreach_statement(const ForStatement *statement, void *userdata,
								void (*callback)(void *userdata,
										 const Statement *statement));

/* literal expression */
/* Note: '...' value has type RAVI_TVARARGS and no associated SemInfo. */
RAVICOMP_EXPORT const struct var_type *raviX_literal_expression_type(const LiteralExpression *expression);
RAVICOMP_EXPORT const SemInfo *raviX_literal_expression_literal(const LiteralExpression *expression);

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
				 void (*callback)(void *userdata, const Statement *statement));
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
RAVICOMP_EXPORT const StringObject *
raviX_function_call_expression_method_name(const struct function_call_expression *expression);
RAVICOMP_EXPORT void
raviX_function_call_expression_foreach_argument(const struct function_call_expression *expression, void *userdata,
						void (*callback)(void *, const struct expression *expr));

/* Convert a statement to the correct type */
RAVICOMP_EXPORT enum AstNodeType raviX_statement_type(const Statement *statement);
RAVICOMP_EXPORT const ReturnStatement *raviX_return_statement(const Statement *stmt);
RAVICOMP_EXPORT const LabelStatement *raviX_label_statement(const Statement *stmt);
RAVICOMP_EXPORT const GotoStatement *raviX_goto_statement(const Statement *stmt);
RAVICOMP_EXPORT const LocalStatement *raviX_local_statement(const Statement *stmt);
RAVICOMP_EXPORT const ExpressionStatement *raviX_expression_statement(const Statement *stmt);
RAVICOMP_EXPORT const FunctionStatement *raviX_function_statement(const Statement *stmt);
RAVICOMP_EXPORT const DoStatement *raviX_do_statement(const Statement *stmt);
RAVICOMP_EXPORT const TestThenStatement *raviX_test_then_statement(const Statement *stmt);
RAVICOMP_EXPORT const IfStatement *raviX_if_statement(const Statement *stmt);
RAVICOMP_EXPORT const WhileOrRepeatStatement *raviX_while_or_repeat_statement(const Statement *stmt);
RAVICOMP_EXPORT const ForStatement *raviX_for_statement(const Statement *stmt);

/* Convert an expression to the correct type */
RAVICOMP_EXPORT enum AstNodeType raviX_expression_type(const struct expression *expression);
RAVICOMP_EXPORT const LiteralExpression *raviX_literal_expression(const struct expression *expr);
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
RAVICOMP_EXPORT const StringObject *
raviX_variable_symbol_name(const struct lua_variable_symbol *lua_local_symbol);
RAVICOMP_EXPORT const struct var_type *raviX_variable_symbol_type(const struct lua_variable_symbol *lua_local_symbol);
// NULL if global
RAVICOMP_EXPORT const struct block_scope *
raviX_variable_symbol_scope(const struct lua_variable_symbol *lua_local_symbol);

/* label symbol */
RAVICOMP_EXPORT const StringObject *raviX_label_name(const struct lua_label_symbol *symbol);
RAVICOMP_EXPORT const struct block_scope *raviX_label_scope(const struct lua_label_symbol *symbol);

/* upvalue symbol */
RAVICOMP_EXPORT const struct var_type *raviX_upvalue_symbol_type(const struct lua_upvalue_symbol *symbol);
RAVICOMP_EXPORT const struct lua_variable_symbol *
raviX_upvalue_target_variable(const struct lua_upvalue_symbol *symbol);
RAVICOMP_EXPORT const struct function_expression *
raviX_upvalue_target_function(const struct lua_upvalue_symbol *symbol);
RAVICOMP_EXPORT unsigned raviX_upvalue_index(const struct lua_upvalue_symbol *symbol);

/* Utilities */
#ifdef __GNUC__
#define FORMAT_ATTR(pos) __attribute__((__format__(__printf__, pos, pos + 1)))
#else
#define FORMAT_ATTR(pos)
#endif

RAVICOMP_EXPORT void raviX_buffer_init(TextBuffer *mb, size_t initial_size);
RAVICOMP_EXPORT void raviX_buffer_resize(TextBuffer *mb, size_t new_size);
RAVICOMP_EXPORT void raviX_buffer_reserve(TextBuffer *mb, size_t n);
RAVICOMP_EXPORT void raviX_buffer_free(TextBuffer *mb);
static inline char *raviX_buffer_data(const TextBuffer *mb) { return mb->buf; }
static inline size_t raviX_buffer_size(const TextBuffer *mb) { return mb->capacity; }
static inline size_t raviX_buffer_len(const TextBuffer *mb) { return mb->pos; }
static inline void raviX_buffer_reset(TextBuffer *mb) { mb->pos = 0; }

/* following convert input to string before adding */
RAVICOMP_EXPORT void raviX_buffer_add_string(TextBuffer *mb, const char *str);
RAVICOMP_EXPORT void raviX_buffer_add_bytes(TextBuffer *mb, const char *str, size_t len);
RAVICOMP_EXPORT void raviX_buffer_add_fstring(TextBuffer *mb, const char *str, ...) FORMAT_ATTR(2);

/* strncpy() replacement with guaranteed 0 termination */
RAVICOMP_EXPORT void raviX_string_copy(char *buf, const char *src, size_t buflen);

#endif
