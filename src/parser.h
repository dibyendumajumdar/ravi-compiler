#ifndef ravicomp_IMPLEMENTATION_H
#define ravicomp_IMPLEMENTATION_H

/*
 * Internal header file for the implementation.
 * The data structures defined here are private.
 */

#include "ravi_compiler.h"

#include "allocate.h"
#include "membuf.h"
#include "ptrlist.h"
#include "set.h"

#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { MAXVARS = 125 };
#define LUA_ENV "_ENV"
#define LUA_MAXINTEGER		LLONG_MAX
#define LUA_MININTEGER		LLONG_MIN

typedef unsigned long long lua_Unsigned;
typedef unsigned char lu_byte;

//////////////////////////

struct lua_symbol_list;

/*
 * Encapsulate all the compiler state.
 * All memory is held by this object or sub-objects. Memory is freed when
 * the object is destroyed.
 */
struct CompilerState {
	struct allocator ast_node_allocator;
	struct allocator ptrlist_allocator;
	struct allocator block_scope_allocator;
	struct allocator symbol_allocator;
	struct allocator string_allocator;
	struct allocator string_object_allocator;
	struct set *strings;
	struct ast_node *main_function;
	LinearizerState *linearizer;
	int (*error_handler)(const char *fmt, ...);
	buffer_t buff;		 /* temp storage for literals, used by the lexer and parser */
	jmp_buf env;		 /* For error handling */
	buffer_t error_message; /* For error handling, error message is saved here */
	bool killed;		 /* flag to check if this is already destroyed */
	const struct string_object *_ENV; /* name of the env variable */
};

/* number of reserved words */
#define NUM_RESERVED ((int)(TOK_while - FIRST_RESERVED + 1))

/* state of the lexer plus state of the parser when shared by all
   functions */
struct LexerState {
	int current;	 /* current character (charint) */
	int linenumber;	 /* input line counter */
	int lastline;	 /* line of last token 'consumed' */
	Token t;	 /* current token */
	Token lookahead; /* look ahead token */
	CompilerState *container;
	const char *buf;
	size_t bufsize;
	size_t n;
	const char *p;
	buffer_t *buff;    /* buffer for tokens, points to the buffer in compiler_state */
	const char *source; /* current source name */
	const char *envn;   /* environment variable name */
};
void raviX_syntaxerror(LexerState *ls, const char *msg);

struct ast_node;
DECLARE_PTR_LIST(ast_node_list, struct ast_node);

struct var_type;
DECLARE_PTR_LIST(var_type_list, struct var_type);

/* RAVI: Following are the types we will use
** use in parsing. The rationale for types is
** performance - as of now these are the only types that
** we care about from a performance point of view - if any
** other types appear then they are all treated as ANY
**/
typedef enum {
	RAVI_TANY = 0,	  /* Lua dynamic type */
	RAVI_TNUMINT = 1, /* integer number */
	RAVI_TNUMFLT,	  /* floating point number */
	RAVI_TARRAYINT,	  /* array of ints */
	RAVI_TARRAYFLT,	  /* array of doubles */
	RAVI_TFUNCTION,	  /* Lua or C Function */
	RAVI_TTABLE,	  /* Lua table */
	RAVI_TSTRING,	  /* string */
	RAVI_TNIL,	  /* NIL */
	RAVI_TBOOLEAN,	  /* boolean */
	RAVI_TUSERDATA,	  /* userdata or lightuserdata */
	RAVI_TVARARGS     /* Not a real type - represents ... */
} ravitype_t;

/* Lua type info. We need to support user defined types too which are known by name */
struct var_type {
	ravitype_t type_code;
	/* type name for user defined types; used to lookup metatable in registry, only set when type_code is
	 * RAVI_TUSERDATA */
	const struct string_object *type_name;
};

struct pseudo;
DECLARE_PTR_LIST(lua_symbol_list, struct lua_symbol);

struct lua_variable_symbol {
	struct var_type value_type;
	const struct string_object *var_name; /* name of the variable */
	struct block_scope *block; /* NULL if global symbol, as globals are never added to a scope */
	struct lua_symbol *env; /* Only applicable for global symbols - this should point to _ENV */
	unsigned escaped: 1, /* Has one or more up-value references */
		function_parameter: 1; /* Is a function parameter */
	struct pseudo *pseudo;	   /* backend data for the symbol */
};
struct lua_label_symbol {
	const struct string_object *label_name;
	struct block_scope *block;
	struct pseudo* pseudo;     /* backend data for the symbol */
};
struct lua_upvalue_symbol {
	struct var_type value_type;
	struct lua_symbol *target_variable;	   /* variable reference */
	struct ast_node *target_function; /* Where the upvalue lives */
	unsigned upvalue_index : 16,   /* index of the upvalue in the function where this upvalue occurs */
	    is_in_parent_stack : 1,    /* 1 if yes - populated by code generator only */
	    parent_upvalue_index : 15; /* if !is_in_parent_stack then upvalue index in parent - populated by code generator only */
	/*TODO add pseudo ?*/
};
/* A symbol is a name recognised in Ravi/Lua code*/
struct lua_symbol {
	enum symbol_type symbol_type;
	union {
		struct lua_variable_symbol variable;
		struct lua_label_symbol label;
		struct lua_upvalue_symbol upvalue;
	};
};
struct block_scope {
	struct ast_node *function;	     /* function owning this block - of type FUNCTION_EXPR */
	struct block_scope *parent;	     /* parent block, may belong to parent function */
	struct lua_symbol_list *symbol_list; /* symbols defined in this block */
	unsigned need_close: 1;              /* When we exit scope of this block the upvalues need to be closed */
};

/*STMT_RETURN */
struct return_statement {
	struct ast_node_list *expr_list;
};
/* STMT_LABEL */
struct label_statement {
	struct lua_symbol *symbol;
};
/* STMT_GOTO */
struct goto_statement {
	unsigned is_break : 1; /* is this a break statement */
	const struct string_object *name; /* target label, used to resolve the goto destination */
	struct block_scope* goto_scope;   /* The scope of the goto statement */
};
/* STMT_LOCAL local variable declarations */
struct local_statement {
	struct lua_symbol_list *var_list;
	struct ast_node_list *expr_list;
};
/* STMT_EXPR: Also covers assignments */
struct expression_statement {
	struct ast_node_list *var_expr_list; /* Optional var expressions, comma separated */
	struct ast_node_list *expr_list;     /* Comma separated expressions */
};
struct function_statement {
	struct ast_node *name;		 /* base symbol to be looked up - symbol_expression */
	struct ast_node_list *selectors; /* Optional list of index_expression(s) */
	struct ast_node *method_name;	 /* Optional - index_expression */
	struct ast_node *function_expr;	 /* Function's AST - function_expression */
};
struct do_statement {
	struct block_scope *scope;		 /* The do statement only creates a new scope */
	struct ast_node_list *do_statement_list; /* statements in this block */
};
/* Used internally in if_stmt, not an independent AST node */
struct test_then_statement {
	struct ast_node *condition;
	struct block_scope *test_then_scope;
	struct ast_node_list *test_then_statement_list; /* statements in this block */
};
struct if_statement {
	struct ast_node_list *if_condition_list; /* Actually a list of test_then_blocks */
	struct block_scope *else_block;
	struct ast_node_list *else_statement_list; /* statements in this block */
};
struct while_or_repeat_statement {
	struct ast_node *condition;
	struct block_scope *loop_scope;
	struct ast_node_list *loop_statement_list; /* statements in this block */
};
/* Used for both generic and numeric for loops */
struct for_statement {
	struct block_scope* for_scope; /* encapsulates the entire for statement */
	struct lua_symbol_list *symbols;
	struct ast_node_list *expr_list;
	struct block_scope *for_body;
	struct ast_node_list *for_statement_list; /* statements in this block */
};
/* To access the type field common to all expr objects */
/* all expr types must be compatible with base_expression */

#define BASE_EXPRESSION_FIELDS struct var_type type; unsigned truncate_results: 1

struct base_expression {
	BASE_EXPRESSION_FIELDS;
};
struct literal_expression {
	BASE_EXPRESSION_FIELDS;
	SemInfo u;
};
/* primaryexp -> NAME | '(' expr ')', NAME is parsed as EXPR_SYMBOL */
struct symbol_expression {
	BASE_EXPRESSION_FIELDS;
	struct lua_symbol *var;
};
/* EXPR_Y_INDEX or EXPR_FIELD_SELECTOR */
struct index_expression {
	BASE_EXPRESSION_FIELDS;
	struct ast_node *expr; /* '[' expr ']' */
};
/* EXPR_UNARY */
struct unary_expression {
	BASE_EXPRESSION_FIELDS;
	UnaryOperatorType unary_op;
	struct ast_node *expr;
};
struct binary_expression {
	BASE_EXPRESSION_FIELDS;
	BinaryOperatorType binary_op;
	struct ast_node *expr_left;
	struct ast_node *expr_right;
};
struct function_expression {
	BASE_EXPRESSION_FIELDS;
	unsigned is_vararg : 1;
	unsigned is_method : 1;
	unsigned need_close : 1;
	uint32_t proc_id; /* Backend allocated id */
	struct ast_node *parent_function;	       /* parent function or NULL if main chunk */
	struct block_scope *main_block;		       /* the function's main block */
	struct ast_node_list *function_statement_list; /* statements in this block */
	struct lua_symbol_list
	    *args; /* arguments, also must be part of the function block's symbol list */
	struct ast_node_list *child_functions; /* child functions declared in this function */
	struct lua_symbol_list *upvalues;      /* List of upvalues */
	struct lua_symbol_list *locals;	       /* List of locals */
};
/* Assign values in table constructor */
/* EXPR_TABLE_ELEMENT_ASSIGN - used in table constructor */
struct table_element_assignment_expression {
	BASE_EXPRESSION_FIELDS;
	struct ast_node *key_expr; /* If NULL means this is a list field with next available index,
							else specifies index expression */
	struct ast_node *value_expr;
};
/* constructor -> '{' [ field { sep field } [sep] ] '}' where sep -> ',' | ';' */
/* table constructor expression EXPR_TABLE_LITERAL occurs in function call and simple expr */
struct table_literal_expression {
	BASE_EXPRESSION_FIELDS;
	struct ast_node_list *expr_list;
};
/* suffixedexp -> primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs } */
/* suffix_list may have EXPR_FIELD_SELECTOR, EXPR_Y_INDEX, EXPR_FUNCTION_CALL */
struct suffixed_expression {
	BASE_EXPRESSION_FIELDS;
	struct ast_node *primary_expr;
	struct ast_node_list *suffix_list;
};
struct function_call_expression {
	/* Note that in Ravi the results from a function call must be type asserted during assignment to
	 * variables. This is not explicit in the AST but is required to ensure that function return
	 * values do not overwrite the type of the variables in an inconsistent way.
	 */
	BASE_EXPRESSION_FIELDS;
	const struct string_object *method_name; /* Optional method_name */
	struct ast_node_list *arg_list;		 /* Call arguments */
	int num_results;			 /* How many results do we expect, -1 means all available results */
};
#undef BASE_EXPRESSION_FIELDS

/* ALL AST nodes start with following fields */
#define BASE_AST_FIELDS enum ast_node_type type; int line_number
/* Statement AST nodes have following common fields.
 */
struct statement {
	BASE_AST_FIELDS;
};
/* Expression AST nodes have following common fields
*/
struct expression {
	BASE_AST_FIELDS;
	struct base_expression common_expr;
};

/* The parse tree is made up of ast_node objects. Some of the ast_nodes reference the appropriate block
scopes but not all scopes may be referenced. The tree captures Lua syntax tree - i.e. statements such as
while, repeat, and for are captured in the way user uses them and not the way Lua generates code. Potentially
we can have a transformation step to convert to a tree that is more like the code generation

The ast_node must be aligned with struct expression for expressions, and with struct statement for statements.
*/
struct ast_node {
	BASE_AST_FIELDS;
	union {
		struct return_statement return_stmt; /*STMT_RETURN */
		struct label_statement label_stmt; /* STMT_LABEL */
		struct goto_statement goto_stmt; /* STMT_GOTO */
		struct local_statement local_stmt; /* STMT_LOCAL local variable declarations */
		struct expression_statement expression_stmt;
		struct function_statement function_stmt;
		struct do_statement do_stmt;
		struct test_then_statement test_then_block;
		struct if_statement if_stmt;
		struct while_or_repeat_statement while_or_repeat_stmt;
		struct for_statement for_stmt;
		struct base_expression common_expr;
		struct literal_expression literal_expr;
		struct symbol_expression symbol_expr;
		struct index_expression index_expr;
		struct unary_expression unary_expr;
		struct binary_expression binary_expr;
		struct function_expression function_expr; /* a literal expression whose result is a value of type function */
		struct table_element_assignment_expression table_elem_assign_expr;
		struct table_literal_expression table_expr;
		struct suffixed_expression suffixed_expr;
		struct function_call_expression function_call_expr;
	};
};
#undef BASE_AST_FIELDS

static inline void set_typecode(struct var_type *vt, ravitype_t t) { vt->type_code = t; }
static inline void set_type(struct var_type *vt, ravitype_t t)
{
	vt->type_code = t;
	vt->type_name = NULL;
}
static inline void set_typename(struct var_type *vt, ravitype_t t, const struct string_object *name)
{
	vt->type_code = t;
	vt->type_name = name;
}
static inline void copy_type(struct var_type *a, const struct var_type *b)
{
	a->type_code = b->type_code;
	a->type_name = b->type_name;
}

struct parser_state {
	LexerState *ls;
	CompilerState *container;
	struct ast_node *current_function;
	struct block_scope *current_scope;
};

void raviX_print_ast_node(buffer_t *buf, struct ast_node *node, int level); /* output the AST structure recursively */
const char *raviX_get_type_name(ravitype_t tt);

int raviX_ast_simplify(CompilerState* container);

#endif
