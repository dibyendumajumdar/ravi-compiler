#ifndef ravicomp_ast_h
#define ravicomp_ast_h

/*
A parser and syntax tree builder for Ravi. This is work in progress.
Once ready it will be used to create a new byte code generator for Ravi.

The parser will perform following actions:

a) Generate syntax tree
b) Perform type checking (Ravi enhancement)
*/

#include "ravi_compiler.h"

#include "allocate.h"
#include "membuf.h"
#include "ptrlist.h"
#include "set.h"

#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { MAXVARS = 125 };
#define LUA_ENV "_ENV"
#define LUA_MAXINTEGER INT_MAX

typedef unsigned long long lua_Unsigned;
typedef unsigned char lu_byte;

//////////////////////////

struct lua_symbol_list;
struct linearizer_state;
struct lexer_state;

/*
 * Encapsulate all the compiler state.
 * All memory is held by this object or sub-objects. Memory is freed when
 * the object is destroyed.
 */
struct compiler_state {
	struct allocator ast_node_allocator;
	struct allocator ptrlist_allocator;
	struct allocator block_scope_allocator;
	struct allocator symbol_allocator;
	struct allocator string_allocator;
	struct allocator string_object_allocator;
	struct set *strings;
	struct ast_node *main_function;
	struct linearizer_state *linearizer;
	int (*error_handler)(const char *fmt, ...);
	Mbuffer buff;		 /* temp storage for literals */
	jmp_buf env;		 /* For error handling */
	membuff_t error_message; /* For error handling */
	bool killed;		 /* flag to check if this is already destroyed */
};

/* number of reserved words */
#define NUM_RESERVED ((int)(TK_WHILE - FIRST_RESERVED + 1))

/* state of the lexer plus state of the parser when shared by all
   functions */
struct lexer_state {
	int current;	 /* current character (charint) */
	int linenumber;	 /* input line counter */
	int lastline;	 /* line of last token 'consumed' */
	Token t;	 /* current token */
	Token lookahead; /* look ahead token */
	struct compiler_state *container;
	const char *buf;
	size_t bufsize;
	size_t n;
	const char *p;
	Mbuffer *buff;	    /* buffer for tokens */
	const char *source; /* current source name */
	const char *envn;   /* environment variable name */
};

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
	RAVI_TUSERDATA	  /* userdata or lightuserdata */
} ravitype_t;

/* Lua type info. We need to support user defined types too which are known by name */
struct var_type {
	ravitype_t type_code;
	const char *type_name; /* type name for user defined types; used to lookup metatable in registry, only set when
				     type_code is RAVI_TUSERDATA */
};

struct pseudo;
struct lua_symbol;
DECLARE_PTR_LIST(lua_symbol_list, struct lua_symbol);

struct block_scope;

/* Types of symbols */
enum symbol_type {
	SYM_LOCAL,
	SYM_UPVALUE,
	SYM_GLOBAL, /* Global symbols are never added to a scope so they are always looked up */
	SYM_LABEL
};

/* A symbol is a name recognised in Ravi/Lua code*/
struct lua_symbol {
	enum symbol_type symbol_type;
	struct var_type value_type;
	union {
		struct {
			const char *var_name;	   /* name of the variable */
			struct block_scope *block; /* NULL if global symbol, as globals are never added to a scope */
			struct pseudo *pseudo;	   /* backend data for the symbol */
		} var;
		struct {
			const char *label_name;
			struct block_scope *block;
		} label;
		struct {
			struct lua_symbol *var;	   /* variable reference */
			struct ast_node *function; /* Where the upvalue lives */
			uint32_t upvalue_index;	   /* index of the upvalue in function */
						   /*TODO add pseudo ?*/
		} upvalue;
	};
};

struct block_scope {
	struct ast_node *function;	     /* function owning this block - of type FUNCTION_EXPR */
	struct block_scope *parent;	     /* parent block, may belong to parent function */
	struct lua_symbol_list *symbol_list; /* symbols defined in this block */
};

enum ast_node_type {
	AST_NONE, /* Used when the node doesn't represent an AST such as test_then_block. */
	AST_RETURN_STMT,
	AST_GOTO_STMT,
	AST_LABEL_STMT,
	AST_DO_STMT,
	AST_LOCAL_STMT,
	AST_FUNCTION_STMT,
	AST_IF_STMT,
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

/* The parse tree is made up of ast_node objects. Some of the ast_nodes reference the appropriate block
scopes but not all scopes may be referenced. The tree captures Lua syntax tree - i.e. statements such as
while, repeat, and for are captured in the way user uses them and not the way Lua generates code. Potentially
we can have a transformation step to convert to a tree that is more like the code generation */
struct ast_node {
	enum ast_node_type type;
	union {
		struct {
			struct ast_node_list *expr_list;
		} return_stmt; /*AST_RETURN_STMT */
		struct {
			struct lua_symbol *symbol;
		} label_stmt; /* AST_LABEL_STMT */
		struct {
			const char *name;	     /* target label, used to resolve the goto destination */
			struct ast_node *label_stmt; /* Initially this will be NULL; set by a separate pass */
		} goto_stmt;			     /* AST_GOTO_STMT */
		struct {
			struct lua_symbol_list *var_list;
			struct ast_node_list *expr_list;
		} local_stmt; /* AST_LOCAL_STMT local variable declarations */
		struct {
			struct ast_node_list *var_expr_list; /* Optional var expressions, comma separated */
			struct ast_node_list *expr_list;     /* Comma separated expressions */
		} expression_stmt;			     /* AST_EXPR_STMT: Also covers assignments */
		struct {
			struct ast_node *name;		 /* base symbol to be looked up */
			struct ast_node_list *selectors; /* Optional */
			struct ast_node *method_name;	 /* Optional */
			struct ast_node *function_expr;	 /* Function's AST */
		} function_stmt;
		struct {
			struct block_scope *scope;		 /* The do statement only creates a new scope */
			struct ast_node_list *do_statement_list; /* statements in this block */
		} do_stmt;
		struct {
			struct ast_node *condition;
			struct block_scope *test_then_scope;
			struct ast_node_list *test_then_statement_list; /* statements in this block */
		} test_then_block; /* Used internally in if_stmt, not an independent AST node */
		struct {
			struct ast_node_list *if_condition_list; /* Actually a list of test_then_blocks */
			struct block_scope *else_block;
			struct ast_node_list *else_statement_list; /* statements in this block */
		} if_stmt;
		struct {
			struct ast_node *condition;
			struct block_scope *loop_scope;
			struct ast_node_list *loop_statement_list; /* statements in this block */
		} while_or_repeat_stmt;
		struct {
			struct lua_symbol_list *symbols;
			struct ast_node_list *expr_list;
			struct block_scope *for_body;
			struct ast_node_list *for_statement_list; /* statements in this block */
		} for_stmt;					  /* Used for both generic and numeric for loops */
		struct {
			struct var_type type;
		} common_expr; /* To access the type field common to all expr objects */
		/* all expr types must be compatible with common_expr */
		struct {
			struct var_type type;
			union {
				lua_Integer i;
				lua_Number n;
				const struct string_object *s;
			} u;
		} literal_expr;
		struct { /* primaryexp -> NAME | '(' expr ')', NAME is parsed as AST_SYMBOL_EXPR */
			struct var_type type;
			struct lua_symbol *var;
		} symbol_expr;
		struct { /* AST_Y_INDEX_EXPR or AST_FIELD_SELECTOR_EXPR */
			struct var_type type;
			struct ast_node *expr; /* '[' expr ']' */
		} index_expr;
		struct { /* AST_UNARY_EXPR */
			struct var_type type;
			UnOpr unary_op;
			struct ast_node *expr;
		} unary_expr;
		struct {
			struct var_type type;
			BinOpr binary_op;
			struct ast_node *expr_left;
			struct ast_node *expr_right;
		} binary_expr;
		struct {
			struct var_type type;
			unsigned int is_vararg : 1;
			unsigned int is_method : 1;
			struct ast_node *parent_function;	       /* parent function or NULL if main chunk */
			struct block_scope *main_block;		       /* the function's main block */
			struct ast_node_list *function_statement_list; /* statements in this block */
			struct lua_symbol_list
			    *args; /* arguments, also must be part of the function block's symbol list */
			struct ast_node_list *child_functions; /* child functions declared in this function */
			struct lua_symbol_list *upvalues;      /* List of upvalues */
			struct lua_symbol_list *locals;	       /* List of locals */
		} function_expr; /* a literal expression whose result is a value of type function */
		struct {	 /* AST_INDEXED_ASSIGN_EXPR - used in table constructor */
			struct var_type type;
			struct ast_node *key_expr; /* If NULL means this is a list field with next available index,
							else specifies index expression */
			struct ast_node *value_expr;
		} indexed_assign_expr; /* Assign values in table constructor */
		struct {	       /* constructor -> '{' [ field { sep field } [sep] ] '}' where sep -> ',' | ';' */
			struct var_type type;
			struct ast_node_list *expr_list;
		} table_expr; /* table constructor expression AST_TABLE_EXPR occurs in function call and simple expr */
		struct {
			/* suffixedexp -> primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs } */
			/* suffix_list may have AST_FIELD_SELECTOR_EXPR, AST_Y_INDEX_EXPR, AST_FUNCTION_CALL_EXPR */
			struct var_type type;
			struct ast_node *primary_expr;
			struct ast_node_list *suffix_list;
		} suffixed_expr;
		struct {
			/* Note that in Ravi the results from a function call must be type asserted during assignment to
			 * variables. This is not explicit in the AST but is required to ensure that function return
			 * values do not overwrite the type of the variables in an inconsistent way.
			 */
			struct var_type type;
			const char *method_name;	/* Optional method_name */
			struct ast_node_list *arg_list; /* Call arguments */
		} function_call_expr;
	};
};

static inline void set_typecode(struct var_type *vt, ravitype_t t) { vt->type_code = t; }
static inline void set_type(struct var_type *vt, ravitype_t t)
{
	vt->type_code = t;
	vt->type_name = NULL;
}
static void inline set_typename(struct var_type *vt, ravitype_t t, const char *name)
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
	struct lexer_state *ls;
	struct compiler_state *container;
	struct ast_node *current_function;
	struct block_scope *current_scope;
};

/*
Linearizer
*/
struct instruction;
struct node;
struct basic_block;
struct edge;
struct cfg;
struct proc;
struct constant;

DECLARE_PTR_LIST(instruction_list, struct instruction);
DECLARE_PTR_LIST(edge_list, struct edge);
DECLARE_PTR_LIST(pseudo_list, struct pseudo);
DECLARE_PTR_LIST(proc_list, struct proc);

#define container_of(ptr, type, member) ((type *)((char *)(ptr)-offsetof(type, member)))

/* order is important here ! */
enum opcode {
	op_nop,
	op_ret,
	op_loadk,
	op_loadnil,
	op_loadbool,
	op_add,
	op_addff,
	op_addfi,
	op_addii,
	op_sub,
	op_subff,
	op_subfi,
	op_subif,
	op_subii,
	op_mul,
	op_mulff,
	op_mulfi,
	op_mulii,
	op_div,
	op_divff,
	op_divfi,
	op_divif,
	op_divii,
	op_idiv,
	op_band,
	op_bandii,
	op_bor,
	op_borii,
	op_bxor,
	op_bxorii,
	op_shl,
	op_shlii,
	op_shr,
	op_shrii,
	op_eq,
	op_eqii,
	op_eqff,
	op_lt,
	op_ltii,
	op_ltff,
	op_le,
	op_leii,
	op_leff,
	op_mod,
	op_pow,
	op_closure,
	op_unm,
	op_unmi,
	op_unmf,
	op_len,
	op_leni,
	op_toint,
	op_toflt,
	op_toclosure,
	op_tostring,
	op_toiarray,
	op_tofarray,
	op_totable,
	op_totype,
	op_not,
	op_bnot,
	op_loadglobal,
	op_newtable,
	op_newiarray,
	op_newfarray,
	op_put, /* target is any */
	op_put_ikey,
	op_put_skey,
	op_tput, /* target is table */
	op_tput_ikey,
	op_tput_skey,
	op_iaput, /* target is integer[]*/
	op_iaput_ival,
	op_faput, /* target is number[] */
	op_faput_fval,
	op_cbr,
	op_br,
	op_mov,
	op_call,
	op_get,
	op_get_ikey,
	op_get_skey,
	op_tget,
	op_tget_ikey,
	op_tget_skey,
	op_iaget,
	op_iaget_ikey,
	op_faget,
	op_faget_ikey,
	op_storeglobal,
};

enum pseudo_type {
	PSEUDO_SYMBOL,
	PSEUDO_TEMP_FLT,
	PSEUDO_TEMP_INT,
	PSEUDO_TEMP_ANY,
	PSEUDO_CONSTANT,
	PSEUDO_PROC,
	PSEUDO_NIL,
	PSEUDO_TRUE,
	PSEUDO_FALSE,
	PSEUDO_BLOCK,
	PSEUDO_RANGE,
	PSEUDO_RANGE_SELECT
};

/* pseudo represents a pseudo (virtual) register */
struct pseudo {
	unsigned type : 4, regnum : 16, freed : 1;
	struct instruction *insn; /* instruction that created this pseudo */
	union {
		struct lua_symbol *symbol;	 /* PSEUDO_SYMBOL */
		const struct constant *constant; /* PSEUDO_CONSTANT */
		ravitype_t temp_type;		 /* PSEUDO_TEMP - not sure we need this */
		struct proc *proc;		 /* PSEUDO_PROC */
		struct basic_block *block;	 /* PSEUDO_BLOCK */
		struct pseudo *range_pseudo;	 /* PSEUDO_RANGE_SELECT */
	};
};

/* single instruction */
struct instruction {
	unsigned opcode : 8;
	struct pseudo_list *operands;
	struct pseudo_list *targets;
	struct basic_block *block; /* owning block */
};

struct edge {
	struct node *from;
	struct node *to;
};

#define NODE_FIELDS                                                                                                    \
	uint32_t index;                                                                                                \
	struct edge_list *pred;                                                                                        \
	struct edge_list *succ

struct node {
	NODE_FIELDS;
};

/* Basic block is a specialization of node */
struct basic_block {
	NODE_FIELDS;
	struct instruction_list *insns;
};
DECLARE_PTR_LIST(basic_block_list, struct basic_block);

#define CFG_FIELDS                                                                                                     \
	unsigned node_count;                                                                                           \
	unsigned allocated;                                                                                            \
	struct node **nodes;                                                                                           \
	struct node *entry;                                                                                            \
	struct node *exit

struct cfg {
	CFG_FIELDS;
};

struct pseudo_generator {
	uint8_t next_reg;
	int16_t free_pos;
	uint8_t free_regs[256];
};

struct constant {
	uint8_t type;
	uint16_t index; /* index number starting from 0 assigned to each constant - acts like a reg num */
	union {
		lua_Integer i;
		lua_Number n;
		const struct string_object *s;
	};
};

/* proc is a type of cfg */
struct proc {
	CFG_FIELDS;
	uint32_t id; /* ID for the proc */
	struct linearizer_state *linearizer;
	struct proc_list *procs;	/* procs defined in this proc */
	struct proc *parent;		/* enclosing proc */
	struct ast_node *function_expr; /* function ast that we are compiling */
	struct block_scope *current_scope;
	struct basic_block *current_bb;
	struct pseudo_generator local_pseudos;	  /* locals */
	struct pseudo_generator temp_int_pseudos; /* temporaries known to be integer type */
	struct pseudo_generator temp_flt_pseudos; /* temporaries known to be number type */
	struct pseudo_generator temp_pseudos;	  /* All other temporaries */
	struct set *constants;			  /* constants used by this proc */
	unsigned num_constants;
};

static inline struct basic_block *n2bb(struct node *n) { return (struct basic_block *)n; }
static inline struct node *bb2n(struct basic_block *bb) { return (struct node *)bb; }

struct linearizer_state {
	struct allocator instruction_allocator;
	struct allocator edge_allocator;
	struct allocator pseudo_allocator;
	struct allocator ptrlist_allocator;
	struct allocator basic_block_allocator;
	struct allocator proc_allocator;
	struct allocator unsized_allocator;
	struct allocator constant_allocator;
	struct compiler_state *ast_container;
	struct proc *main_proc;	     /* The root of the compiled chunk of code */
	struct proc_list *all_procs; /* All procs allocated by the linearizer */
	struct proc *current_proc;   /* proc being compiled */
	uint32_t proc_id;
};

void raviX_print_ast_node(membuff_t *buf, struct ast_node *node, int level); /* output the AST structure recusrively */
void raviX_show_linearizer(struct linearizer_state *linearizer, membuff_t *mb);
void raviX_syntaxerror(struct lexer_state *ls, const char *msg);

#endif
