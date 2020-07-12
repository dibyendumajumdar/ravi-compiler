#ifndef ravicomp_LINEARIZER_H
#define ravicomp_LINEARIZER_H

#include "ravi_compiler.h"

#include "common.h"
#include "parser.h"
#include "allocate.h"
#include "membuf.h"
#include "ptrlist.h"

/*
Linearizer component is responsible for translating the abstract syntax tree to
a Linear intermediate representation (IR).
*/
struct instruction;
struct basic_block;
struct proc;
struct constant;

DECLARE_PTR_LIST(instruction_list, struct instruction);
DECLARE_PTR_LIST(pseudo_list, struct pseudo);
DECLARE_PTR_LIST(proc_list, struct proc);

#define container_of(ptr, type, member) ((type *)((char *)(ptr)-offsetof(type, member)))

/* order is important here ! */
enum opcode {
	op_nop,
	op_ret,
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
	op_movi,
	op_movif, /* int to float if compatible else error */
	op_movf,
	op_movfi, /* float to int if compatible else error */
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

/* Basic block */
struct basic_block {
	nodeId_t index; /* The index of the block is a key to enable retrieving the block from its container */
	struct instruction_list *insns;
};
DECLARE_PTR_LIST(basic_block_list, struct basic_block);

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
	unsigned node_count;
	unsigned allocated;
	struct basic_block **nodes;
	uint32_t id; /* ID for the proc */
	struct linearizer_state *linearizer;
	struct proc_list *procs;	/* procs defined in this proc */
	struct proc *parent;		/* enclosing proc */
	struct ast_node *function_expr; /* function ast that we are compiling */
	struct block_scope *current_scope;
	struct basic_block *current_bb;
	struct basic_block *current_break_target; /* track the current break target, previous target must be saved / restored in stack discipline */
	struct pseudo_generator local_pseudos;	  /* locals */
	struct pseudo_generator temp_int_pseudos; /* temporaries known to be integer type */
	struct pseudo_generator temp_flt_pseudos; /* temporaries known to be number type */
	struct pseudo_generator temp_pseudos;	  /* All other temporaries */
	struct set *constants;			  /* constants used by this proc */
	unsigned num_constants;
	struct graph *cfg;  /* place holder for control flow graph; the linearizer does not create this */
};

struct linearizer_state {
	struct allocator instruction_allocator;
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

void raviX_show_linearizer(struct linearizer_state *linearizer, membuff_t *mb);
void raviX_output_basic_block_as_table(struct proc *proc, struct basic_block *bb, membuff_t *mb);

struct instruction *raviX_last_instruction(struct basic_block *block);


#endif