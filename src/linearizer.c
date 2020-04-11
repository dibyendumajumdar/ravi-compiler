/*
Copyright (C) 2018-2020 Dibyendu Majumdar
*/

#include "fnv_hash.h"
#include "ptrlist.h"
#include "ravi_ast.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void handle_error(struct compiler_state *container, const char *msg)
{
	// TODO source and line number
	raviX_buffer_add_string(&container->error_message, msg);
	longjmp(container->env, 1);
}

static struct pseudo *linearize_expression(struct proc *proc, struct ast_node *expr);
static struct basic_block *create_block(struct proc *proc);
static void start_block(struct proc *proc, struct basic_block *bb);
static void linearize_statement(struct proc *proc, struct ast_node *node);
static void linearize_statement_list(struct proc *proc, struct ast_node_list *list);
static void start_scope(struct linearizer_state *linearizer, struct proc *proc, struct block_scope *scope);
static void end_scope(struct linearizer_state *linearizer, struct proc *proc);
static void instruct_br(struct proc *proc, struct pseudo *pseudo);
static bool is_block_terminated(struct basic_block *block);
static struct pseudo *instruct_move(struct proc *proc, struct pseudo *target, struct pseudo *src);
static void linearize_function(struct linearizer_state *linearizer);

/**
 * Allocates a register by reusing a free'd register if possible otherwise
 * allocating a new one
 */
static inline unsigned allocate_register(struct pseudo_generator *generator)
{
	if (generator->free_pos > 0) {
		return generator->free_regs[--generator->free_pos];
	}
	return generator->next_reg++;
}

/**
 * Puts a register in the free list (must not already have been put there).
 */
static inline void free_register(struct proc *proc, struct pseudo_generator *generator, unsigned reg)
{
	if (generator->free_pos == (sizeof generator->free_regs / sizeof generator->free_regs[0])) {
		/* TODO proper error handling */
		handle_error(proc->linearizer->ast_container, "Out of register space\n");
		return;
	}
	// Debug check - ensure register being freed hasn't already been freed
	for (int i = 0; i < generator->free_pos; i++) {
		assert(generator->free_regs[i] != reg);
	}
	generator->free_regs[generator->free_pos++] = (uint8_t)reg;
}

/* Linearizer initialization  */
struct linearizer_state *raviX_init_linearizer(struct compiler_state *container)
{
	struct linearizer_state *linearizer = (struct linearizer_state *)calloc(1, sizeof(struct linearizer_state));
	linearizer->ast_container = container;
	raviX_allocator_init(&linearizer->edge_allocator, "edge_allocator", sizeof(struct edge), sizeof(double),
			     sizeof(struct edge) * 32);
	raviX_allocator_init(&linearizer->instruction_allocator, "instruction_allocator", sizeof(struct instruction),
			     sizeof(double), sizeof(struct instruction) * 128);
	raviX_allocator_init(&linearizer->ptrlist_allocator, "ptrlist_allocator", sizeof(struct ptr_list),
			     sizeof(double), sizeof(struct ptr_list) * 64);
	raviX_allocator_init(&linearizer->pseudo_allocator, "pseudo_allocator", sizeof(struct pseudo), sizeof(double),
			     sizeof(struct pseudo) * 128);
	raviX_allocator_init(&linearizer->basic_block_allocator, "basic_block_allocator", sizeof(struct basic_block),
			     sizeof(double), sizeof(struct basic_block) * 32);
	raviX_allocator_init(&linearizer->proc_allocator, "proc_allocator", sizeof(struct proc), sizeof(double),
			     sizeof(struct proc) * 32);
	raviX_allocator_init(&linearizer->unsized_allocator, "unsized_allocator", 0, sizeof(double), CHUNK);
	raviX_allocator_init(&linearizer->constant_allocator, "constant_allocator", sizeof(struct constant),
			     sizeof(double), sizeof(struct constant) * 64);
	linearizer->proc_id = 0;
	return linearizer;
}

void raviX_destroy_linearizer(struct linearizer_state *linearizer)
{
	if (linearizer == NULL)
		return;
	struct proc *proc;
	FOR_EACH_PTR(linearizer->all_procs, proc)
	{
		if (proc->constants)
			set_destroy(proc->constants, NULL);
	}
	END_FOR_EACH_PTR(proc);
	raviX_allocator_destroy(&linearizer->edge_allocator);
	raviX_allocator_destroy(&linearizer->instruction_allocator);
	raviX_allocator_destroy(&linearizer->ptrlist_allocator);
	raviX_allocator_destroy(&linearizer->pseudo_allocator);
	raviX_allocator_destroy(&linearizer->basic_block_allocator);
	raviX_allocator_destroy(&linearizer->proc_allocator);
	raviX_allocator_destroy(&linearizer->unsized_allocator);
	raviX_allocator_destroy(&linearizer->constant_allocator);
	free(linearizer);
}

/**
 * We assume strings are all interned and can be compared by
 * address
 */
static int compare_constants(const void *a, const void *b)
{
	const struct constant *c1 = (const struct constant *)a;
	const struct constant *c2 = (const struct constant *)b;
	if (c1->type != c2->type)
		return 1;
	if (c1->type == RAVI_TNUMINT)
		return c1->i == c2->i;
	else if (c1->type == RAVI_TNUMFLT)
		return c1->n == c2->n;
	else
		return c1->s == c2->s;
}

/**
 * Hashes a constant
 */
static uint32_t hash_constant(const void *c)
{
	const struct constant *c1 = (const struct constant *)c;
	if (c1->type == RAVI_TNUMINT)
		return (uint32_t)c1->i;
	else if (c1->type == RAVI_TNUMFLT)
		return (uint32_t)c1->n; // FIXME maybe use Lua's hash gen
	else
		return (uint32_t)c1->s->hash;
}

/**
 * Adds a constant to the proc's constant table. The constant is also assigned a
 * pseudo register.
 */
static const struct constant *add_constant(struct proc *proc, const struct constant *c)
{
	struct set_entry *entry = set_search(proc->constants, c);
	if (entry == NULL) {
		int reg = proc->num_constants++;
		struct constant *c1 = raviX_allocator_allocate(&proc->linearizer->constant_allocator, 0);
		assert(c1); // FIXME
		memcpy(c1, c, sizeof(struct constant));
		c1->index = reg;
		set_add(proc->constants, c1);
		// printf("Created new constant and assigned reg %d\n", reg);
		return c1;
	} else {
		const struct constant *c1 = entry->key;
		// printf("Found constant at reg %d\n", c1->index);
		return c1;
	}
}

/**
 * Allocates and adds a constant to the Proc's constants table.
 * Input is expected to be AST_LITERAL_EXPR
 */
static const struct constant *allocate_constant(struct proc *proc, struct ast_node *node)
{
	assert(node->type == AST_LITERAL_EXPR);
	struct constant c = {.type = node->literal_expr.type.type_code};
	if (c.type == RAVI_TNUMINT)
		c.i = node->literal_expr.u.i;
	else if (c.type == RAVI_TNUMFLT)
		c.n = node->literal_expr.u.n;
	else
		c.s = node->literal_expr.u.s;
	return add_constant(proc, &c);
}

static const struct constant *allocate_integer_constant(struct proc *proc, int i)
{
	struct constant c = {.type = RAVI_TNUMINT, .i = i};
	return add_constant(proc, &c);
}

static const struct constant *allocate_string_constant(struct proc *proc, const struct string_object *s)
{
	struct constant c = {.type = RAVI_TSTRING, .s = s};
	return add_constant(proc, &c);
}

struct pseudo *allocate_symbol_pseudo(struct proc *proc, struct lua_symbol *sym, unsigned reg)
{
	assert(sym->var.pseudo == NULL);
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_SYMBOL;
	pseudo->symbol = sym;
	pseudo->regnum = reg;
	if (sym->symbol_type == SYM_LOCAL) {
		sym->var.pseudo = pseudo;
	}
	return pseudo;
}

struct pseudo *allocate_constant_pseudo(struct proc *proc, const struct constant *constant)
{
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_CONSTANT;
	pseudo->constant = constant;
	pseudo->regnum = constant->index;
	return pseudo;
}

struct pseudo *allocate_closure_pseudo(struct proc *proc)
{
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_PROC;
	pseudo->proc = proc;
	return pseudo;
}

struct pseudo *allocate_nil_pseudo(struct proc *proc)
{
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_NIL;
	pseudo->proc = proc;
	return pseudo;
}

struct pseudo *allocate_boolean_pseudo(struct proc *proc, bool is_true)
{
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = is_true ? PSEUDO_TRUE : PSEUDO_FALSE;
	pseudo->proc = proc;
	return pseudo;
}

struct pseudo *allocate_block_pseudo(struct proc *proc, struct basic_block *block)
{
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_BLOCK;
	pseudo->block = block;
	return pseudo;
}

/*
We have several types of temp pseudos.
Specific types for floating and integer values so that we can
localise the assignment of these to registers.
The generic 'any' type is used for other types
but has variant called PSEUDO_RANGE. This is used in function calls
to represent multiple return values, and will most likely also
be used to represent var arg. Most of the time these get converted
back to normal temp pseudo, but in some cases we need to reference
a particular value in the range and for that we use PSEUDO_RANGE_SELECT.
*/
struct pseudo *allocate_temp_pseudo(struct proc *proc, ravitype_t type)
{
	struct pseudo_generator *gen;
	enum pseudo_type pseudo_type;
	switch (type) {
	case RAVI_TNUMFLT:
		gen = &proc->temp_int_pseudos;
		pseudo_type = PSEUDO_TEMP_FLT;
		break;
	case RAVI_TNUMINT:
		gen = &proc->temp_flt_pseudos;
		pseudo_type = PSEUDO_TEMP_INT;
		break;
	default:
		gen = &proc->temp_pseudos;
		pseudo_type = PSEUDO_TEMP_ANY;
		break;
	}
	unsigned reg = allocate_register(gen);
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = pseudo_type;
	pseudo->regnum = reg;
	pseudo->temp_type = type;
	return pseudo;
}

struct pseudo *allocate_range_pseudo(struct proc *proc, struct pseudo *orig_pseudo)
{
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_RANGE;
	pseudo->regnum = orig_pseudo->regnum;
	if (orig_pseudo->type == PSEUDO_TEMP_ANY) {
		orig_pseudo->freed = 1;
	}
	return pseudo;
}

/*
A PSEUDO_RANGE_SELECT picks or selects a particular offset in the range
specified by a PSEUDO_RANGE.
*/
struct pseudo *allocate_range_select_pseudo(struct proc *proc, struct pseudo *range_pseudo, int pick)
{
	assert(range_pseudo->type == PSEUDO_RANGE);
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_RANGE_SELECT;
	pseudo->regnum = range_pseudo->regnum + pick;
	pseudo->range_pseudo = range_pseudo;
	return pseudo;
}

void free_temp_pseudo(struct proc *proc, struct pseudo *pseudo)
{
	if (pseudo->freed)
		return;
	struct pseudo_generator *gen;
	switch (pseudo->type) {
	case PSEUDO_TEMP_FLT:
		gen = &proc->temp_int_pseudos;
		break;
	case PSEUDO_TEMP_INT:
		gen = &proc->temp_flt_pseudos;
		break;
	case PSEUDO_RANGE:
	case PSEUDO_TEMP_ANY:
		gen = &proc->temp_pseudos;
		break;
	default:
		// Not a temp, so no need to do anything
		return;
	}
	free_register(proc, gen, pseudo->regnum);
}

/**
 * Allocate a new Proc. If there is a current Proc, then the new Proc gets added to the
 * current Proc's children.
 */
static struct proc *allocate_proc(struct linearizer_state *linearizer, struct ast_node *function_expr)
{
	assert(function_expr->type == AST_FUNCTION_EXPR);
	struct proc *proc = raviX_allocator_allocate(&linearizer->proc_allocator, 0);
	proc->function_expr = function_expr;
	proc->id = ptrlist_size((struct ptr_list *)linearizer->all_procs);
	ptrlist_add((struct ptr_list **)&linearizer->all_procs, proc, &linearizer->ptrlist_allocator);
	if (linearizer->current_proc) {
		proc->parent = linearizer->current_proc;
		ptrlist_add((struct ptr_list **)&linearizer->current_proc->procs, proc, &linearizer->ptrlist_allocator);
	}
	proc->constants = set_create(hash_constant, compare_constants);
	proc->linearizer = linearizer;
	return proc;
}

static void set_main_proc(struct linearizer_state *linearizer, struct proc *proc)
{
	assert(linearizer->main_proc == NULL);
	assert(linearizer->current_proc == NULL);
	linearizer->main_proc = proc;
	assert(proc->function_expr->function_expr.parent_function == NULL);
}

static inline void set_current_proc(struct linearizer_state *linearizer, struct proc *proc)
{
	linearizer->current_proc = proc;
}

static void linearize_function_args(struct linearizer_state *linearizer)
{
	struct proc *proc = linearizer->current_proc;
	struct ast_node *func_expr = proc->function_expr;
	struct lua_symbol *sym;
	FOR_EACH_PTR(func_expr->function_expr.args, sym)
	{
		/* The arg symbols already have register assigned by the local scope */
		/* TODO we need to add type assertion operators for typed args */
		// handle_error(linearizer->ast_container, "feature not yet implemented");
	}
	END_FOR_EACH_PTR(sym);
}

static void linearize_statement_list(struct proc *proc, struct ast_node_list *list)
{
	struct ast_node *node;
	FOR_EACH_PTR(list, node) { linearize_statement(proc, node); }
	END_FOR_EACH_PTR(node);
}

static struct instruction *allocate_instruction(struct proc *proc, enum opcode op)
{
	struct instruction *insn = raviX_allocator_allocate(&proc->linearizer->instruction_allocator, 0);
	insn->opcode = op;
	return insn;
}

static void free_instruction_operand_pseudos(struct proc *proc, struct instruction *insn)
{
	struct pseudo *operand;
	FOR_EACH_PTR_REVERSE(insn->operands, operand) { free_temp_pseudo(proc, operand); }
	END_FOR_EACH_PTR_REVERSE(operand);
}

static inline void add_instruction(struct proc *proc, struct instruction *insn)
{
	assert(insn->block == NULL || insn->block == proc->current_bb);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
	insn->block = proc->current_bb;
}

static inline void remove_instruction(struct proc *proc, struct instruction *insn)
{
	ptrlist_remove((struct ptr_list **)&proc->current_bb->insns, insn, 1);
}

static inline struct instruction *last_instruction(struct basic_block *block)
{
	if (ptrlist_size((struct ptr_list *)block->insns) == 0)
		return NULL;
	return (struct instruction *)ptrlist_last((struct ptr_list *)block->insns);
}

static inline void add_instruction_operand(struct proc *proc, struct instruction *insn, struct pseudo *pseudo)
{
	ptrlist_add((struct ptr_list **)&insn->operands, pseudo, &proc->linearizer->ptrlist_allocator);
}

static inline void add_instruction_target(struct proc *proc, struct instruction *insn, struct pseudo *pseudo)
{
	ptrlist_add((struct ptr_list **)&insn->targets, pseudo, &proc->linearizer->ptrlist_allocator);
}

static inline struct pseudo *convert_range_to_temp(struct pseudo *pseudo)
{
	assert(pseudo->type == PSEUDO_RANGE);
	pseudo->type = PSEUDO_TEMP_ANY;
	return pseudo;
}

static struct pseudo *linearize_literal(struct proc *proc, struct ast_node *expr)
{
	assert(expr->type == AST_LITERAL_EXPR);
	ravitype_t type = expr->literal_expr.type.type_code;
	struct pseudo *pseudo = NULL;
	switch (type) {
	case RAVI_TNUMFLT:
	case RAVI_TNUMINT:
	case RAVI_TSTRING:
		pseudo = allocate_constant_pseudo(proc, allocate_constant(proc, expr));
		break;
	case RAVI_TNIL:
		pseudo = allocate_nil_pseudo(proc);
		break;
	case RAVI_TBOOLEAN:
		pseudo = allocate_boolean_pseudo(proc, expr->literal_expr.u.i);
		break;
	default:
		handle_error(proc->linearizer->ast_container, "feature not yet implemented");
		break;
	}
	return pseudo;
}

static struct pseudo *linearize_unary_operator(struct proc *proc, struct ast_node *node)
{
	// TODO if any expr is range we need to convert to temp?
	UnaryOperatorType op = node->unary_expr.unary_op;
	struct pseudo *subexpr = linearize_expression(proc, node->unary_expr.expr);
	ravitype_t subexpr_type = node->unary_expr.expr->common_expr.type.type_code;
	enum opcode targetop = op_nop;
	switch (op) {
	case UNOPR_MINUS:
		if (subexpr_type == RAVI_TNUMINT)
			targetop = op_unmi;
		else if (subexpr_type == RAVI_TNUMFLT)
			targetop = op_unmf;
		else
			targetop = op_unm;
		break;
	case UNOPR_LEN:
		if (subexpr_type == RAVI_TARRAYINT || subexpr_type == RAVI_TARRAYFLT)
			targetop = op_leni;
		else
			targetop = op_len;
		break;
	case UNOPR_TO_INTEGER:
		targetop = subexpr_type != RAVI_TNUMINT ? op_toint : op_nop;
		break;
	case UNOPR_TO_NUMBER:
		targetop = subexpr_type != RAVI_TNUMFLT ? op_toflt : op_nop;
		break;
	case UNOPR_TO_CLOSURE:
		targetop = subexpr_type != RAVI_TFUNCTION ? op_toclosure : op_nop;
		break;
	case UNOPR_TO_STRING:
		targetop = subexpr_type != RAVI_TSTRING ? op_tostring : op_nop;
		break;
	case UNOPR_TO_INTARRAY:
		targetop = subexpr_type != RAVI_TARRAYINT ? op_toiarray : op_nop;
		break;
	case UNOPR_TO_NUMARRAY:
		targetop = subexpr_type != RAVI_TARRAYFLT ? op_tofarray : op_nop;
		break;
	case UNOPR_TO_TABLE:
		targetop = subexpr_type != RAVI_TTABLE ? op_totable : op_nop;
		break;
	case UNOPR_TO_TYPE:
		targetop = op_totype;
		break;
	case UNOPR_NOT:
		targetop = op_not;
		break;
	case UNOPR_BNOT:
		targetop = op_bnot;
		break;
	default:
		handle_error(proc->linearizer->ast_container, "unexpected unary op");
		break;
	}
	if (targetop == op_nop) {
		return subexpr;
	}
	struct instruction *insn = allocate_instruction(proc, targetop);
	struct pseudo *target = subexpr;
	if (op == UNOPR_TO_TYPE) {
		const struct constant *tname_constant = allocate_string_constant(proc, node->unary_expr.type.type_name);
		struct pseudo *tname_pseudo = allocate_constant_pseudo(proc, tname_constant);
		add_instruction_operand(proc, insn, tname_pseudo);
	} else if (op == UNOPR_NOT || op == UNOPR_BNOT) {
		add_instruction_operand(proc, insn, target);
		target = allocate_temp_pseudo(proc, RAVI_TANY);
	} else if (op == UNOPR_MINUS || op == UNOPR_LEN) {
		add_instruction_operand(proc, insn, target);
		target = allocate_temp_pseudo(proc, subexpr_type);
	}
	add_instruction_target(proc, insn, target);
	add_instruction(proc, insn);
	return target;
}

static struct pseudo *instruct_move(struct proc *proc, struct pseudo *target, struct pseudo *src)
{
	// TODO we should use type specific MOVE instructions
	struct instruction *mov = allocate_instruction(proc, op_mov);
	add_instruction_operand(proc, mov, src);
	add_instruction_target(proc, mov, target);
	add_instruction(proc, mov);
	return target;
}

static void instruct_cbr(struct proc *proc, struct pseudo *conditin_pseudo, struct basic_block *true_block,
			 struct basic_block *false_block)
{
	struct pseudo *true_pseudo = allocate_block_pseudo(proc, true_block);
	struct pseudo *false_pseudo = allocate_block_pseudo(proc, false_block);
	struct instruction *insn = allocate_instruction(proc, op_cbr);
	add_instruction_operand(proc, insn, conditin_pseudo);
	add_instruction_target(proc, insn, true_pseudo);
	add_instruction_target(proc, insn, false_pseudo);
	add_instruction(proc, insn);
}

static void instruct_br(struct proc *proc, struct pseudo *pseudo)
{
	assert(pseudo->type == PSEUDO_BLOCK);
	if (is_block_terminated(proc->current_bb)) {
		start_block(proc, create_block(proc));
	}
	struct instruction *insn = allocate_instruction(proc, op_br);
	add_instruction_target(proc, insn, pseudo);
	add_instruction(proc, insn);
}

// clang-format off
/*
Lua and/or operators are processed so that with 'and' the result is the final 
true value, and with 'or' it is the first true value.

and IR

	result = eval(expr_left);
	if (result)
		goto Lnext:
	else
		goto Ldone;
Lnext:
	result = eval(expr_right);
	goto Ldone;
Ldone:

or IR

	result = eval(expr_left);
	if (result)
		goto Ldone:
	else
		goto Lnext;
Lnext:
	result = eval(expr_right);
	goto Ldone;
Ldone:

*/
// clang-format on
static struct pseudo *linearize_bool(struct proc *proc, struct ast_node *node, bool is_and)
{
	struct ast_node *e1 = node->binary_expr.expr_left;
	struct ast_node *e2 = node->binary_expr.expr_right;

	struct basic_block *first_block = create_block(proc);
	struct basic_block *end_block = create_block(proc);

	struct pseudo *result = allocate_temp_pseudo(proc, RAVI_TANY);
	struct pseudo *operand1 = linearize_expression(proc, e1);
	instruct_move(proc, result, operand1);
	free_temp_pseudo(proc, operand1);
	if (is_and)
		instruct_cbr(proc, result, first_block, end_block); // If first value is true then evaluate the second
	else
		instruct_cbr(proc, result, end_block, first_block);

	start_block(proc, first_block);
	struct pseudo *operand2 = linearize_expression(proc, e2);
	instruct_move(proc, result, operand2);
	free_temp_pseudo(proc, operand2);
	instruct_br(proc, allocate_block_pseudo(proc, end_block));

	start_block(proc, end_block);

	return result;
}

/* Utility to create a binary instruction where operands and target pseudo is known */
static void create_binary_instruction(struct proc* proc, enum opcode targetop, struct pseudo* operand1,
	struct pseudo* operand2, struct pseudo* target) {
	struct instruction* insn = allocate_instruction(proc, targetop);
	add_instruction_operand(proc, insn, operand1);
	add_instruction_operand(proc, insn, operand2);
	add_instruction_target(proc, insn, target);
	add_instruction(proc, insn);
}

static struct pseudo *linearize_binary_operator(struct proc *proc, struct ast_node *node)
{
	// TODO if any expr is range we need to convert to temp?

	BinaryOperatorType op = node->binary_expr.binary_op;

	if (op == BINOPR_AND) {
		return linearize_bool(proc, node, true);
	} else if (op == BINOPR_OR) {
		return linearize_bool(proc, node, false);
	}

	struct ast_node *e1 = node->binary_expr.expr_left;
	struct ast_node *e2 = node->binary_expr.expr_right;
	struct pseudo *operand1 = linearize_expression(proc, e1);
	struct pseudo *operand2 = linearize_expression(proc, e2);

	enum opcode targetop;
	switch (op) {
	case BINOPR_ADD:
		targetop = op_add;
		break;
	case BINOPR_SUB:
		targetop = op_sub;
		break;
	case BINOPR_MUL:
		targetop = op_mul;
		break;
	case BINOPR_DIV:
		targetop = op_div;
		break;
	case BINOPR_IDIV:
		targetop = op_idiv;
		break;
	case BINOPR_BAND:
		targetop = op_band;
		break;
	case BINOPR_BOR:
		targetop = op_bor;
		break;
	case BINOPR_BXOR:
		targetop = op_bxor;
		break;
	case BINOPR_SHL:
		targetop = op_shl;
		break;
	case BINOPR_SHR:
		targetop = op_shr;
		break;
	case BINOPR_EQ:
	case BINOPR_NE:
		targetop = op_eq;
		break;
	case BINOPR_LT:
	case BINOPR_GT:
		targetop = op_lt;
		break;
	case BINOPR_LE:
	case BINOPR_GE:
		targetop = op_le;
		break;
	case BINOPR_MOD:
		targetop = op_mod;
		break;
	case BINOPR_POW:
		targetop = op_pow;
		break;
	default:
		handle_error(proc->linearizer->ast_container, "unexpected binary op");
		targetop = op_nop;
		break;
	}

	ravitype_t t1 = e1->common_expr.type.type_code;
	ravitype_t t2 = e2->common_expr.type.type_code;

	bool swap = false;
	switch (targetop) {
	case op_add:
	case op_mul:
		swap = t1 == RAVI_TNUMINT && t2 == RAVI_TNUMFLT;
		break;
	case op_eq:
	case op_lt:
	case op_le:
		swap = op == BINOPR_NE || op == BINOPR_GT || op == BINOPR_GE;
		break;
	default:
		break;
	}

	if (swap) {
		struct pseudo *temp;
		struct ast_node *ntemp;
		temp = operand1;
		operand1 = operand2;
		operand2 = temp;
		ntemp = e1;
		e1 = e2;
		e2 = ntemp;
		t1 = e1->common_expr.type.type_code;
		t2 = e2->common_expr.type.type_code;
	}

	switch (targetop) {
	case op_add:
	case op_mul:
		if (t1 == RAVI_TNUMFLT && t2 == RAVI_TNUMFLT)
			targetop += 1;
		else if (t1 == RAVI_TNUMFLT && t2 == RAVI_TNUMINT)
			targetop += 2;
		else if (t1 == RAVI_TNUMINT && t2 == RAVI_TNUMINT)
			targetop += 3;
		break;
	case op_div:
	case op_sub:
		if (t1 == RAVI_TNUMFLT && t2 == RAVI_TNUMFLT)
			targetop += 1;
		else if (t1 == RAVI_TNUMFLT && t2 == RAVI_TNUMINT)
			targetop += 2;
		else if (t1 == RAVI_TNUMINT && t2 == RAVI_TNUMFLT)
			targetop += 3;
		else if (t1 == RAVI_TNUMINT && t2 == RAVI_TNUMINT)
			targetop += 4;
		break;
	case op_band:
	case op_bor:
	case op_bxor:
	case op_shl:
	case op_shr:
		if (t1 == RAVI_TNUMINT && t2 == RAVI_TNUMINT)
			targetop += 1;
		break;
	case op_eq:
	case op_le:
	case op_lt:
		if (t1 == RAVI_TNUMINT && t2 == RAVI_TNUMINT)
			targetop += 1;
		else if (t1 == RAVI_TNUMFLT && t2 == RAVI_TNUMFLT)
			targetop += 2;
		break;
	default:
		break;
	}

	ravitype_t target_type = node->binary_expr.type.type_code;
	struct pseudo *target = allocate_temp_pseudo(proc, target_type);
	create_binary_instruction(proc, targetop, operand1, operand2, target);
	free_temp_pseudo(proc, operand1);
	free_temp_pseudo(proc, operand2);

	return target;
}

/* generates closure instruction - linearizes a Proc, and then adds instruction to create closure from it */
static struct pseudo *linearize_function_expr(struct proc *proc, struct ast_node *expr)
{
	struct proc *curproc = proc->linearizer->current_proc;
	struct proc *newproc = allocate_proc(proc->linearizer, expr);
	set_current_proc(proc->linearizer, newproc);
	// printf("linearizing function\n");
	linearize_function(proc->linearizer);
	set_current_proc(proc->linearizer, curproc); // restore the proc
	ravitype_t target_type = expr->function_expr.type.type_code;
	struct pseudo *target = allocate_temp_pseudo(proc, target_type);
	struct pseudo *operand = allocate_closure_pseudo(newproc);
	struct instruction *insn = allocate_instruction(proc, op_closure);
	add_instruction_operand(proc, insn, operand);
	add_instruction_target(proc, insn, target);
	add_instruction(proc, insn);

	return target;
}

static struct pseudo *linearize_symbol_expression(struct proc *proc, struct ast_node *expr)
{
	struct lua_symbol *sym = expr->symbol_expr.var;
	if (sym->symbol_type == SYM_GLOBAL) {
		struct pseudo *target = allocate_temp_pseudo(proc, RAVI_TANY);
		struct pseudo *operand = allocate_symbol_pseudo(proc, sym, 0); // no register actually
		struct instruction *insn = allocate_instruction(proc, op_loadglobal);
		target->insn = insn;
		add_instruction_operand(proc, insn, operand);
		add_instruction_target(proc, insn, target);
		add_instruction(proc, insn);
		return target;
	} else if (sym->symbol_type == SYM_LOCAL) {
		return sym->var.pseudo;
	} else if (sym->symbol_type == SYM_UPVALUE) {
		/* upvalue index is the position of upvalue in the function, we treat this as the pseudo register for
		 * the upvalue */
		/* TODO maybe the pseudo be pre-created when we start linearizing the funcon and stored in the symbol
		 * like we do for locals? */
		return allocate_symbol_pseudo(proc, sym, sym->upvalue.upvalue_index);
	} else {
		handle_error(proc->linearizer->ast_container, "feature not yet implemented");
		return NULL;
	}
}

static struct pseudo *instruct_indexed_load(struct proc *proc, ravitype_t container_type,
					    struct pseudo *container_pseudo, ravitype_t key_type,
					    struct pseudo *key_pseudo, ravitype_t target_type)
{
	enum opcode op = op_get;
	switch (container_type) {
	case RAVI_TTABLE:
		op = op_tget;
		break;
	case RAVI_TARRAYINT:
		op = op_iaget;
		break;
	case RAVI_TARRAYFLT:
		op = op_faget;
		break;
	default:
		break;
	}
	/* Note we rely upon ordering of enums here */
	switch (key_type) {
	case RAVI_TNUMINT:
		op++;
		break;
	case RAVI_TSTRING:
		assert(container_type != RAVI_TARRAYINT && container_type != RAVI_TARRAYFLT);
		op += 2;
		break;
	default:
		break;
	}
	struct pseudo *target_pseudo = allocate_temp_pseudo(proc, target_type);
	struct instruction *insn = allocate_instruction(proc, op);
	add_instruction_operand(proc, insn, container_pseudo);
	add_instruction_operand(proc, insn, key_pseudo);
	add_instruction_target(proc, insn, target_pseudo);
	add_instruction(proc, insn);
	target_pseudo->insn = insn;
	return target_pseudo;
}

static void instruct_indexed_store(struct proc *proc, ravitype_t table_type, struct pseudo *table,
				   struct pseudo *index_pseudo, ravitype_t index_type, struct pseudo *value_pseudo,
				   ravitype_t value_type)
{
	// TODO validate the type of assignment
	// Insert type assertions if needed
	enum opcode op;
	switch (table_type) {
	case RAVI_TARRAYINT:
		op = op_iaput;
		if (value_type == RAVI_TNUMINT) {
			op = op_iaput_ival;
		}
		break;
	case RAVI_TARRAYFLT:
		op = op_faput;
		if (value_type == RAVI_TNUMFLT) {
			op = op_faput_fval;
		}
		break;
	default:
		op = table_type == RAVI_TTABLE ? op_tput : op_put;
		if (index_type == RAVI_TNUMINT) {
			op += 1;
		} else if (index_type == RAVI_TSTRING) {
			op += 2;
		}
		break;
	}

	struct instruction *insn = allocate_instruction(proc, op);
	add_instruction_operand(proc, insn, table);
	add_instruction_operand(proc, insn, index_pseudo);
	add_instruction_operand(proc, insn, value_pseudo);
	add_instruction(proc, insn);
}

static void convert_loadglobal_to_store(struct proc *proc, struct instruction *insn, struct pseudo *value_pseudo,
					ravitype_t value_type)
{
	assert(proc->current_bb == insn->block);
	remove_instruction(proc, insn);
	insn->opcode = op_storeglobal;
	add_instruction_operand(proc, insn, value_pseudo);
	struct pseudo *get_target = ptrlist_delete_last((struct ptr_list **)&insn->targets);
	free_temp_pseudo(proc, get_target);
	add_instruction(proc, insn);
}

static void convert_indexed_load_to_store(struct proc *proc, struct instruction *insn, struct pseudo *value_pseudo,
					  ravitype_t value_type)
{
	assert(proc->current_bb == insn->block);
	enum opcode putop;
	switch (insn->opcode) {
	case op_iaget:
	case op_iaget_ikey:
		putop = value_type == RAVI_TNUMINT ? op_iaput_ival : op_iaput;
		break;
	case op_faget:
	case op_faget_ikey:
		putop = value_type == RAVI_TNUMFLT ? op_faput_fval : op_faput;
		break;
	case op_tget:
		putop = op_tput;
		break;
	case op_tget_ikey:
		putop = op_tput_ikey;
		break;
	case op_tget_skey:
		putop = op_tput_skey;
		break;
	case op_get:
		putop = op_put;
		break;
	case op_get_ikey:
		putop = op_put_ikey;
		break;
	case op_get_skey:
		putop = op_put_skey;
		break;
	default:
		return;
	}
	remove_instruction(proc, insn);
	insn->opcode = putop;
	add_instruction_operand(proc, insn, value_pseudo);
	struct pseudo *get_target = ptrlist_delete_last((struct ptr_list **)&insn->targets);
	free_temp_pseudo(proc, get_target);
	add_instruction(proc, insn);
}

/**
 * Lua function calls can return multiple values, and the caller decides how many values to accept.
 * We indicate multiple values using a PSEUDO_RANGE.
 * We also handle method call:
 * <pseudo>:name(...) -> is translated to <pseudo>.name(<pseudo>, ...)
 */
static struct pseudo *linearize_function_call_expression(struct proc *proc, struct ast_node *expr,
							 struct ast_node *callsite_expr, struct pseudo *callsite_pseudo)
{
	struct instruction *insn = allocate_instruction(proc, op_call);
	struct pseudo *self_arg = NULL; /* For method call */
	if (expr->function_call_expr.method_name) {
		const struct constant *name_constant =
		    allocate_string_constant(proc, expr->function_call_expr.method_name);
		struct pseudo *name_pseudo = allocate_constant_pseudo(proc, name_constant);
		self_arg = callsite_pseudo; /* The original callsite must be passed as 'self' */
		/* create new call site as callsite[name] */
		callsite_pseudo = instruct_indexed_load(proc, callsite_expr->common_expr.type.type_code,
							callsite_pseudo, RAVI_TSTRING, name_pseudo, RAVI_TANY);
	}

	add_instruction_operand(proc, insn, callsite_pseudo);
	if (self_arg) {
		add_instruction_operand(proc, insn, self_arg);
	}

	struct ast_node *arg;
	int argc = ptrlist_size((const struct ptr_list *)expr->function_call_expr.arg_list);
	FOR_EACH_PTR(expr->function_call_expr.arg_list, arg)
	{
		argc -= 1;
		struct pseudo *arg_pseudo = linearize_expression(proc, arg);
		if (argc != 0 && arg_pseudo->type == PSEUDO_RANGE) {
			// Not last one, so range can only be 1
			convert_range_to_temp(arg_pseudo);
		}
		add_instruction_operand(proc, insn, arg_pseudo);
	}
	END_FOR_EACH_PTR(arg);

	struct pseudo *return_pseudo = allocate_range_pseudo(
	    proc, callsite_pseudo); /* Base reg for function call - where return values will be placed */
	add_instruction_target(proc, insn, return_pseudo);
	add_instruction(proc, insn);

	free_instruction_operand_pseudos(proc, insn);

	return return_pseudo;
}

/*
 * Suffixed expression examples:
 * f()[1]
 * x[1][2]
 * x.y[1]
 *
 * The result type of a suffixed expression may initially be an indexed load, but when used in the context of
 * an assignment statement the load will be converted to a store.
 * Lua parser does this by creating a VINDEXED node which is only converted to load/store
 * when the VINDEXED node is used.
 */
static struct pseudo *linearize_suffixedexpr(struct proc *proc, struct ast_node *node)
{
	/* suffixedexp -> primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs } */
	struct pseudo *prev_pseudo = linearize_expression(proc, node->suffixed_expr.primary_expr);
	struct ast_node *prev_node = node->suffixed_expr.primary_expr;
	struct ast_node *this_node;
	FOR_EACH_PTR(node->suffixed_expr.suffix_list, this_node)
	{
		struct pseudo *next;
		if (prev_pseudo->type == PSEUDO_RANGE)
			convert_range_to_temp(prev_pseudo);
		if (this_node->type == AST_Y_INDEX_EXPR || this_node->type == AST_FIELD_SELECTOR_EXPR) {
			struct pseudo *key_pseudo = linearize_expression(proc, this_node->index_expr.expr);
			ravitype_t key_type = this_node->index_expr.expr->common_expr.type.type_code;
			next = instruct_indexed_load(proc, prev_node->common_expr.type.type_code, prev_pseudo, key_type,
						     key_pseudo, this_node->common_expr.type.type_code);
		} else if (this_node->type == AST_FUNCTION_CALL_EXPR) {
			next = linearize_function_call_expression(proc, this_node, prev_node, prev_pseudo);
		} else {
			next = NULL;
			handle_error(proc->linearizer->ast_container, "Unexpected expr type in suffix list");
		}
		prev_node = this_node;
		prev_pseudo = next;
	}
	END_FOR_EACH_PTR(node);
	return prev_pseudo;
}

static int linearize_indexed_assign(struct proc *proc, struct pseudo *table, ravitype_t table_type,
				    struct ast_node *expr, int next)
{
	struct pseudo *index_pseudo;
	ravitype_t index_type;
	if (expr->indexed_assign_expr.key_expr) {
		index_pseudo = linearize_expression(proc, expr->indexed_assign_expr.key_expr);
		index_type = expr->indexed_assign_expr.key_expr->index_expr.expr->common_expr.type.type_code;
		// TODO check valid index
	} else {
		const struct constant *constant = allocate_integer_constant(proc, next++);
		index_pseudo = allocate_constant_pseudo(proc, constant);
		index_type = RAVI_TNUMINT;
	}
	struct pseudo *value_pseudo = linearize_expression(proc, expr->indexed_assign_expr.value_expr);
	ravitype_t value_type = expr->indexed_assign_expr.value_expr->common_expr.type.type_code;
	instruct_indexed_store(proc, table_type, table, index_pseudo, index_type, value_pseudo, value_type);
	free_temp_pseudo(proc, index_pseudo);
	free_temp_pseudo(proc, value_pseudo);
	return next;
}

static struct pseudo *linearize_table_constructor(struct proc *proc, struct ast_node *expr)
{
	/* constructor -> '{' [ field { sep field } [sep] ] '}' where sep -> ',' | ';' */
	struct pseudo *target = allocate_temp_pseudo(proc, expr->table_expr.type.type_code);
	enum opcode op = op_newtable;
	if (expr->table_expr.type.type_code == RAVI_TARRAYINT)
		op = op_newiarray;
	else if (expr->table_expr.type.type_code == RAVI_TARRAYFLT)
		op = op_newfarray;
	struct instruction *insn = allocate_instruction(proc, op);
	add_instruction_target(proc, insn, target);
	add_instruction(proc, insn);

	/*TODO process constructor elements */
	struct ast_node *ia;
	int i = 1;
	FOR_EACH_PTR(expr->table_expr.expr_list, ia)
	{
		i = linearize_indexed_assign(proc, target, expr->table_expr.type.type_code, ia, i);
	}
	END_FOR_EACH_PTR(ia);

	return target;
}

static void linearize_store_var(struct proc *proc, ravitype_t var_type, struct pseudo *var_pseudo, ravitype_t val_type,
				struct pseudo *val_pseudo)
{
	// ravitype_t var_type = var_node->common_expr.type.type_code;
	// ravitype_t val_type = var_node->common_expr.type.type_code;

	if (var_pseudo->insn && var_pseudo->insn->opcode >= op_get && var_pseudo->insn->opcode <= op_faget_ikey) {
		convert_indexed_load_to_store(proc, var_pseudo->insn, val_pseudo, val_type);
	} else if (var_pseudo->insn && var_pseudo->insn->opcode == op_loadglobal) {
		convert_loadglobal_to_store(proc, var_pseudo->insn, val_pseudo, val_type);
	} else {
		instruct_move(proc, var_pseudo, val_pseudo); // TODO add type specialization
	}
}

struct node_info {
	ravitype_t type_code;
	struct pseudo *pseudo;
};

static void linearize_assignment(struct proc *proc, struct ast_node_list *expr_list, struct node_info *varinfo, int nv)
{
	struct ast_node *expr;

	int ne = ptrlist_size((const struct ptr_list *)expr_list);
	struct node_info *valinfo = (struct node_info *)alloca(ne * sizeof(struct node_info));
	struct pseudo *last_val_pseudo = NULL;
	int i = 0;
	FOR_EACH_PTR(expr_list, expr)
	{
		struct pseudo *val_pseudo = last_val_pseudo = linearize_expression(proc, expr);
		valinfo[i].type_code = expr->common_expr.type.type_code;
		valinfo[i].pseudo = val_pseudo;
		i++;
		if (i < ne && val_pseudo->type == PSEUDO_RANGE) {
			convert_range_to_temp(val_pseudo);
		}
	}
	END_FOR_EACH_PTR(expr);

	/* TODO do we need to insert type assertions in some cases such as function return values ? */

	int note_ne = ne;
	while (nv > 0) {
		if (nv > ne) {
			if (last_val_pseudo != NULL && last_val_pseudo->type == PSEUDO_RANGE) {
				int pick = nv - ne;
				linearize_store_var(proc, varinfo[nv - 1].type_code, varinfo[nv - 1].pseudo,
						    valinfo[ne - 1].type_code,
						    allocate_range_select_pseudo(proc, last_val_pseudo, pick));
			} else {
				// TODO store NIL
			}
			nv--;
		} else {
			if (valinfo[ne - 1].pseudo->type == PSEUDO_RANGE) {
				/* Only the topmost expression can be a range ... assert */
				assert(ne == note_ne);
				valinfo[ne - 1].pseudo = allocate_range_select_pseudo(proc, valinfo[ne - 1].pseudo, 0);
			}
			linearize_store_var(proc, varinfo[nv - 1].type_code, varinfo[nv - 1].pseudo,
					    valinfo[ne - 1].type_code, valinfo[ne - 1].pseudo);
			free_temp_pseudo(proc, valinfo[ne - 1].pseudo);
			nv--;
			ne--;
		}
	}
}

/*
Expression or assignment statement is of the form:

<LHS exp list...> = <RHS exp list...>

Lua requires some special handling of this statement. Firstly
the LHS expressions are evaluated left to right. 

The RHS is processed right to left. If there is a corresponding LHS expr
then we need to assign the value of the RHS expr to the LHS expr.
Excess RHS expression results are discarded.
Excess LHS expressions have to be set to the default value.

So for example if we had:

expr1, expr2 = expr3, expr4, expr5

Then following needs to be generated

result1 = eval(expr1)
result2 = eval(expr2)

eval(expr5)
*result2 = eval(expr4)
*result1 = eval(expr3)

Our code generation has an issue:
We initially generate load instructions for LHS expressions.
Subsequently we convert these to store instructions  (marked above with asterisk)

The handling of 'local' and expression statements can be partially combined
because the main difference is the LHS side of it. The rest of the processing has to be
the same.
*/
static void linearize_expression_statement(struct proc *proc, struct ast_node *node)
{
	struct ast_node *var;

	int nv = ptrlist_size((const struct ptr_list *)node->expression_stmt.var_expr_list);
	struct node_info *varinfo = (struct node_info *)alloca(nv * sizeof(struct node_info));
	int i = 0;
	FOR_EACH_PTR(node->expression_stmt.var_expr_list, var)
	{
		struct pseudo *var_pseudo = linearize_expression(proc, var);
		varinfo[i].type_code = var->common_expr.type.type_code;
		varinfo[i].pseudo = var_pseudo;
		i++;
	}
	END_FOR_EACH_PTR(var);

	linearize_assignment(proc, node->expression_stmt.expr_list, varinfo, nv);
}

static void linearize_local_statement(struct proc *proc, struct ast_node *stmt)
{
	struct lua_symbol *sym;

	int nv = ptrlist_size((const struct ptr_list *)stmt->local_stmt.var_list);
	struct node_info *varinfo = (struct node_info *)alloca(nv * sizeof(struct node_info));
	int i = 0;

	FOR_EACH_PTR(stmt->local_stmt.var_list, sym)
	{
		struct pseudo *var_pseudo = sym->var.pseudo;
		assert(var_pseudo);
		varinfo[i].type_code = sym->value_type.type_code;
		varinfo[i].pseudo = var_pseudo;
		i++;
	}
	END_FOR_EACH_PTR(var);

	linearize_assignment(proc, stmt->local_stmt.expr_list, varinfo, nv);
}

static struct pseudo *linearize_expression(struct proc *proc, struct ast_node *expr)
{
	switch (expr->type) {
	case AST_LITERAL_EXPR: {
		return linearize_literal(proc, expr);
	} break;
	case AST_BINARY_EXPR: {
		return linearize_binary_operator(proc, expr);
	} break;
	case AST_FUNCTION_EXPR: {
		return linearize_function_expr(proc, expr);
	} break;
	case AST_UNARY_EXPR: {
		return linearize_unary_operator(proc, expr);
	} break;
	case AST_SUFFIXED_EXPR: {
		return linearize_suffixedexpr(proc, expr);
	} break;
	case AST_SYMBOL_EXPR: {
		return linearize_symbol_expression(proc, expr);
	} break;
	case AST_TABLE_EXPR: {
		return linearize_table_constructor(proc, expr);
	} break;
	case AST_Y_INDEX_EXPR:
	case AST_FIELD_SELECTOR_EXPR: {
		return linearize_expression(proc, expr->index_expr.expr);
	} break;
	default:
		handle_error(proc->linearizer->ast_container, "feature not yet implemented");
		break;
	}
	assert(false);
	return NULL;
}

static void linearize_expr_list(struct proc *proc, struct ast_node_list *expr_list, struct instruction *insn,
				struct pseudo_list **pseudo_list)
{
	struct ast_node *expr;
	int ne = ptrlist_size((const struct ptr_list *)expr_list);
	FOR_EACH_PTR(expr_list, expr)
	{
		ne -= 1;
		struct pseudo *pseudo = linearize_expression(proc, expr);
		if (ne != 0 && pseudo->type == PSEUDO_RANGE) {
			convert_range_to_temp(pseudo); // Only accept one result unless it is the last expr
		}
		ptrlist_add((struct ptr_list **)pseudo_list, pseudo, &proc->linearizer->ptrlist_allocator);
	}
	END_FOR_EACH_PTR(expr);
}

static void linearize_return(struct proc *proc, struct ast_node *node)
{
	assert(node->type == AST_RETURN_STMT);
	struct instruction *insn = allocate_instruction(proc, op_ret);
	linearize_expr_list(proc, node->return_stmt.expr_list, insn, &insn->operands);
	add_instruction_target(proc, insn, allocate_block_pseudo(proc, n2bb(proc->exit)));
	add_instruction(proc, insn);
	// FIXME add edge to exit block
	// FIXME terminate block
	// FIXME free all temps
}

/* A block is considered terminated if the last instruction is
   a return or a branch */
static bool is_block_terminated(struct basic_block *block)
{
	struct instruction *last_insn = last_instruction(block);
	if (last_insn == NULL)
		return false;
	if (last_insn->opcode == op_ret || last_insn->opcode == op_cbr || last_insn->opcode == op_br)
		return true;
	return false;
}

static void linearize_test_cond(struct proc *proc, struct ast_node *node, struct basic_block *true_block,
				struct basic_block *false_block)
{
	struct pseudo *condition_pseudo = linearize_expression(proc, node->test_then_block.condition);
	instruct_cbr(proc, condition_pseudo, true_block, false_block);
}

/* linearize the 'else if' block */
static void linearize_test_then(struct proc *proc, struct ast_node *node, struct basic_block *true_block,
				struct basic_block *end_block)
{
	start_block(proc, true_block);
	start_scope(proc->linearizer, proc, node->test_then_block.test_then_scope);
	linearize_statement_list(proc, node->test_then_block.test_then_statement_list);
	end_scope(proc->linearizer, proc);
	if (!is_block_terminated(proc->current_bb))
		instruct_br(proc, allocate_block_pseudo(proc, end_block));
}

// clang-format off
/*
The Lua if statement has a complex structure as it is somewhat like
a combination of case and if statement. The if block is followed by
1 or more elseif blocks. Finally we have an optinal else block.
The elseif blocks are like case statements.

Given

if cond1 then
	block for cond1
elseif cond2 then
	block for cond2
else
	block for else
end

We linearize the statement as follows.

B0:
	if cond1 goto Bcond1 else B2;   // Initial if condition

B2:
	if cond2 goto Bcond2 else B3:   // This is an elseif condition

B3:
	<if AST has else>
	goto Belse;
	<else>
	goto Bend;

Bcond1:
	start scope
	block for cond1
	end scope
	goto Bend;

Bcond2:
	start scope
	block for cond2
	end scope
	goto Bend;

Belse:
	start scope
	block for else
	end scope
	goto Bend;

Bend:
*/
// clang-format on
static void linearize_if_statement(struct proc *proc, struct ast_node *ifnode)
{
	struct basic_block *end_block = NULL;
	struct basic_block *else_block = NULL;
	struct basic_block_list *if_blocks = NULL;
	struct basic_block_list *if_true_blocks = NULL;
	struct ast_node_list *if_else_stmts = ifnode->if_stmt.if_condition_list;
	struct ast_node_list *else_stmts = ifnode->if_stmt.else_statement_list;
	struct block_scope *else_scope = ifnode->if_stmt.else_block;

	struct ast_node *this_node;
	FOR_EACH_PTR(if_else_stmts, this_node)
	{
		struct basic_block *block = create_block(proc);
		ptrlist_add((struct ptr_list **)&if_blocks, block, &proc->linearizer->ptrlist_allocator);
	}
	END_FOR_EACH_PTR(this_node);

	FOR_EACH_PTR(if_else_stmts, this_node)
	{
		struct basic_block *block = create_block(proc);
		ptrlist_add((struct ptr_list **)&if_true_blocks, block, &proc->linearizer->ptrlist_allocator);
	}
	END_FOR_EACH_PTR(this_node);

	if (ifnode->if_stmt.else_statement_list) {
		else_block = create_block(proc);
	}

	end_block = create_block(proc);

	struct basic_block *true_block = NULL;
	struct basic_block *false_block = NULL;
	struct basic_block *block = NULL;

	{
		PREPARE_PTR_LIST(if_blocks, block);
		PREPARE_PTR_LIST(if_true_blocks, true_block);
		FOR_EACH_PTR(if_else_stmts, this_node)
		{
			start_block(proc, block);
			NEXT_PTR_LIST(block);
			if (!block) {
				// last one
				if (else_block)
					false_block = else_block;
				else
					false_block = end_block;
			} else {
				false_block = block;
			}
			linearize_test_cond(proc, this_node, true_block, false_block);
			NEXT_PTR_LIST(true_block);
		}
		END_FOR_EACH_PTR(node);
		FINISH_PTR_LIST(block);
		FINISH_PTR_LIST(true_block);
	}
	{
		PREPARE_PTR_LIST(if_true_blocks, true_block);
		FOR_EACH_PTR(if_else_stmts, this_node)
		{
			linearize_test_then(proc, this_node, true_block, end_block);
			NEXT_PTR_LIST(true_block);
		}
		END_FOR_EACH_PTR(node);
		FINISH_PTR_LIST(true_block);
	}

	if (else_block) {
		start_block(proc, else_block);
		start_scope(proc->linearizer, proc, else_scope);
		linearize_statement_list(proc, else_stmts);
		end_scope(proc->linearizer, proc);
		if (!is_block_terminated(proc->current_bb))
			instruct_br(proc, allocate_block_pseudo(proc, end_block));
	}

	start_block(proc, end_block);
}

/*
handle label statement.
We start a new block which will get associated with the label.
We have to handle the situation where the label pseudo was already created when we
encountered a goto statement but we did not know the block then.
*/
static void linearize_label_statement(struct proc *proc, struct ast_node *node)
{
	struct basic_block *block = create_block(proc);
	start_block(proc, block);
	if (node->label_stmt.symbol->label.pseudo != NULL) {
		/* label pseudo was created by a goto statement */
		assert(node->label_stmt.symbol->label.pseudo->block == NULL);
		node->label_stmt.symbol->label.pseudo->block = block;
	} else {
		node->label_stmt.symbol->label.pseudo = allocate_block_pseudo(proc, block);
	}
}

/* TODO move this logic to parser? */
/* Search for a label going up scopes starting from the scope where the goto statement appeared. */
static struct lua_symbol *find_label(struct proc *proc, struct block_scope *block,
				     const struct string_object *label_name)
{
	struct ast_node *function = block->function; /* We need to stay inside the function when lookng for the label */
	while (block != NULL && block->function == function) {
		struct lua_symbol *symbol;
		FOR_EACH_PTR_REVERSE(block->symbol_list, symbol)
		{
			if (symbol->symbol_type == SYM_LABEL && symbol->label.label_name == label_name) {
				return symbol;
			}
		}
		END_FOR_EACH_PTR_REVERSE(symbol);
		block = block->parent;
	}
	return NULL;
}

/*
When linearizing the goto statement we create a pseudo for the label if it hasn't been already created.
But at this point we may not know the target basic block to goto, which we expect to be filled when the label is
encountered. Of course if the label was linearized before we got to the goto statement then the target block
would already be known and specified in the pseudo.
*/
static void linearize_goto_statement(struct proc *proc, const struct ast_node *node)
{
	/* The AST does not provide link to the label so we have to search for the label in the goto scope 
	   and above */
	if (node->goto_stmt.goto_scope) {
		struct lua_symbol *symbol = find_label(proc, node->goto_stmt.goto_scope, node->goto_stmt.name);
		if (symbol) {
			/* label found */
			if (symbol->label.pseudo == NULL) {
				/* No pseudo? create with NULL target block, must be filled by a label statement */
				symbol->label.pseudo = allocate_block_pseudo(proc, NULL);
			}
			instruct_br(proc, symbol->label.pseudo);
			start_block(proc, create_block(proc));
			return;
		}
	}
	handle_error(proc->linearizer->ast_container, "goto label not found");
}

static void linearize_do_statement(struct proc *proc, struct ast_node *node)
{
	assert(node->type == AST_DO_STMT);
	start_scope(proc->linearizer, proc, node->do_stmt.scope);
	linearize_statement_list(proc, node->do_stmt.do_statement_list);
	end_scope(proc->linearizer, proc);
}

//clang-format off
/*
Lua manual states:

	 for v = e1, e2, e3 do block end

is equivalent to the code:

	 do
	   local var, limit, step = tonumber(e1), tonumber(e2), tonumber(e3)
	   if not (var and limit and step) then error() end
	   var = var - step
	   while true do
		 var = var + step
		 if (step >= 0 and var > limit) or (step < 0 and var < limit) then
		   break
		 end
		 local v = var
		 block
	   end
	 end

We do not need local vars to hold var, limit, step as these can be
temporaries.

	step_positive = 0 < step
	var = var - step
	goto L1
L1:
	var = var + step;
	if step_positive goto L2;
		else goto L3;
L2:
	stop = var > limit
	if stop goto Lend
		else goto Lbody
L3:
	stop = var < limit
	if stop goto Lend
		else goto Lbody
Lbody:
	set local symbol in for loop to var
	do body
	goto L1;

Lend:


*/
//clang-format on
static void linearize_for_num_statement(struct proc* proc, struct ast_node* node)
{
	assert(node->type == AST_FORNUM_STMT);
	start_scope(proc->linearizer, proc, node->for_stmt.for_scope);
	
	/* For now we only allow integer expressions */
	struct ast_node* expr;
	FOR_EACH_PTR(node->for_stmt.expr_list, expr) {
		if (expr->common_expr.type.type_code != RAVI_TNUMINT) {
			handle_error(proc->linearizer->ast_container, "Only for loops with integer expressions currently supported");
		}
	} END_FOR_EACH_PTR(expr);

	struct ast_node* index_var_expr = ptrlist_nth_entry((struct ptr_list*)node->for_stmt.expr_list, 0);
	struct ast_node* limit_expr = ptrlist_nth_entry((struct ptr_list*)node->for_stmt.expr_list, 1);
	struct ast_node* step_expr = ptrlist_nth_entry((struct ptr_list*)node->for_stmt.expr_list, 2);
	struct lua_symbol* var_sym = ptrlist_nth_entry((struct ptr_list*)node->for_stmt.symbols, 0);

	if (index_var_expr == NULL || limit_expr == NULL) {
		handle_error(proc->linearizer->ast_container, "A least index and limit must be supplied");
	}

	struct pseudo* t = linearize_expression(proc, index_var_expr);
	if (t->type == PSEUDO_RANGE) {
		convert_range_to_temp(t); // Only accept one result 
	}
	struct pseudo* index_var_pseudo = allocate_temp_pseudo(proc, RAVI_TNUMINT);
	instruct_move(proc, index_var_pseudo, t);

	t = linearize_expression(proc, limit_expr);
	if (t->type == PSEUDO_RANGE) {
		convert_range_to_temp(t); // Only accept one result 
	}
	struct pseudo* limit_pseudo = allocate_temp_pseudo(proc, RAVI_TNUMINT);
	instruct_move(proc, limit_pseudo, t);

	if (step_expr == NULL)
		t = allocate_constant_pseudo(proc, allocate_integer_constant(proc, 1));
	else {
		t = linearize_expression(proc, step_expr);
		if (t->type == PSEUDO_RANGE) {
			convert_range_to_temp(t); // Only accept one result 
		}
	}
	struct pseudo* step_pseudo = allocate_temp_pseudo(proc, RAVI_TNUMINT);
	instruct_move(proc, step_pseudo, t);

	struct pseudo* step_positive = allocate_temp_pseudo(proc, RAVI_TNUMINT);
	create_binary_instruction(proc, op_ltii, allocate_constant_pseudo(proc, allocate_integer_constant(proc, 0)), 
			step_pseudo, step_positive);

	struct pseudo* stop_pseudo = allocate_temp_pseudo(proc, RAVI_TNUMINT);
	create_binary_instruction(proc, op_subii, index_var_pseudo, step_pseudo, index_var_pseudo);

	struct basic_block *L1 = create_block(proc);
	struct basic_block* L2 = create_block(proc);
	struct basic_block* L3 = create_block(proc);
	struct basic_block* Lbody = create_block(proc);
	struct basic_block* Lend = create_block(proc);
	
	start_block(proc, L1);
	create_binary_instruction(proc, op_addii, index_var_pseudo, step_pseudo, index_var_pseudo);
	instruct_cbr(proc, step_positive, L2, L3);

	start_block(proc, L2);
	create_binary_instruction(proc, op_leii, limit_pseudo, index_var_pseudo, stop_pseudo);
	instruct_cbr(proc, stop_pseudo, Lend, Lbody);

	start_block(proc, L3);
	create_binary_instruction(proc, op_ltii, index_var_pseudo, limit_pseudo, stop_pseudo);
	instruct_cbr(proc, stop_pseudo, Lend, Lbody);

	start_block(proc, Lbody);
	instruct_move(proc, var_sym->var.pseudo, index_var_pseudo);

	start_scope(proc->linearizer, proc, node->for_stmt.for_body);
	linearize_statement_list(proc, node->for_stmt.for_statement_list);
	end_scope(proc->linearizer, proc);

	instruct_br(proc, allocate_block_pseudo(proc, L1));

	end_scope(proc->linearizer, proc);

	free_temp_pseudo(proc, stop_pseudo);
	free_temp_pseudo(proc, step_positive);
	free_temp_pseudo(proc, step_pseudo);
	free_temp_pseudo(proc, limit_pseudo);
	free_temp_pseudo(proc, index_var_pseudo);

	start_block(proc, Lend);
}


static void linearize_statement(struct proc *proc, struct ast_node *node)
{
	switch (node->type) {
	case AST_NONE: {
		break;
	}
	case AST_RETURN_STMT: {
		linearize_return(proc, node);
		break;
	}
	case AST_LOCAL_STMT: {
		linearize_local_statement(proc, node);
		break;
	}
	case AST_FUNCTION_STMT: {
		// typecheck_ast_node(container, function, node->function_stmt.function_expr);
		handle_error(proc->linearizer->ast_container, "AST_FUNCTION_STMT not yet implemented");
		break;
	}
	case AST_LABEL_STMT: {
		linearize_label_statement(proc, node);
		break;
	}
	case AST_GOTO_STMT: {
		linearize_goto_statement(proc, node);
		break;
	}
	case AST_DO_STMT: {
		linearize_do_statement(proc, node);
		break;
	}
	case AST_EXPR_STMT: {
		linearize_expression_statement(proc, node);
		break;
	}
	case AST_IF_STMT: {
		linearize_if_statement(proc, node);
		break;
	}
	case AST_WHILE_STMT:
	case AST_REPEAT_STMT: {
		// typecheck_while_or_repeat_statement(container, function, node);
		handle_error(proc->linearizer->ast_container, "AST_WHILE_STMT/AST_REPEAT_STMT not yet implemented");
		break;
	}
	case AST_FORIN_STMT: {
		// typecheck_for_in_statment(container, function, node);
		handle_error(proc->linearizer->ast_container, "AST_FORIN_STMT not yet implemented");
		break;
	}
	case AST_FORNUM_STMT: {
		linearize_for_num_statement(proc, node);
		break;
	}
	default:
		handle_error(proc->linearizer->ast_container, "unknown statement type");
		break;
	}
}

/**
 * Creates and initializes a basic block to be an empty block. Returns the new basic block.
 */
static struct basic_block *create_block(struct proc *proc)
{
	if (proc->node_count >= proc->allocated) {
		unsigned new_size = proc->allocated + 25;
		struct node **new_data =
		    raviX_allocator_allocate(&proc->linearizer->unsized_allocator, new_size * sizeof(struct node *));
		assert(new_data != NULL);
		if (proc->node_count > 0) {
			memcpy(new_data, proc->nodes, proc->allocated * sizeof(struct node *));
		}
		proc->allocated = new_size;
		proc->nodes = new_data;
	}
	assert(proc->node_count < proc->allocated);
	struct basic_block *new_block = raviX_allocator_allocate(&proc->linearizer->basic_block_allocator, 0);
	new_block->index = proc->node_count;
	proc->nodes[proc->node_count++] = bb2n(new_block);
	return new_block;
}

/**
 * Takes a basic block as an argument and makes it the current block.
 *
 * If the old current block is unterminated then this will terminate that
 * block by adding an unconditional branch to the new current block.
 *
 * All future instructions will be added to the end of the new current block
 */
static void start_block(struct proc *proc, struct basic_block *bb_to_start)
{
	// printf("Starting block %d\n", bb_to_start->index);
	if (proc->current_bb && !is_block_terminated(proc->current_bb)) {
		instruct_br(proc, allocate_block_pseudo(proc, bb_to_start));
	}
	proc->current_bb = bb_to_start;
}

/**
 * Create the initial blocks entry and exit for the proc.
 * sets current block to entry block.
 */
static void initialize_graph(struct proc *proc)
{
	assert(proc != NULL);
	proc->entry = bb2n(create_block(proc));
	proc->exit = bb2n(create_block(proc));
	start_block(proc, n2bb(proc->entry));
}

/**
 * Makes given scope the current scope, and allocates registers for locals.
 */
static void start_scope(struct linearizer_state *linearizer, struct proc *proc, struct block_scope *scope)
{
	proc->current_scope = scope;
	struct lua_symbol *sym;
	FOR_EACH_PTR(scope->symbol_list, sym)
	{
		if (sym->symbol_type == SYM_LOCAL) {
			uint8_t reg = allocate_register(&proc->local_pseudos);
			allocate_symbol_pseudo(proc, sym, reg);
			// printf("Assigning register %d to local %s\n", (int)reg, getstr(sym->var.var_name));
		}
	}
	END_FOR_EACH_PTR(sym);
}

/**
 * Deallocate local registers when the scope ends, in reverse order
 * so that we have a stack discipline, and then changes current scope to be the
 * parent scope.
 */
static void end_scope(struct linearizer_state *linearizer, struct proc *proc)
{
	struct block_scope *scope = proc->current_scope;
	struct lua_symbol *sym;
	FOR_EACH_PTR_REVERSE(scope->symbol_list, sym)
	{
		if (sym->symbol_type == SYM_LOCAL) {
			struct pseudo *pseudo = sym->var.pseudo;
			assert(pseudo && pseudo->type == PSEUDO_SYMBOL && pseudo->symbol == sym);
			// printf("Free register %d for local %s\n", (int)pseudo->regnum, getstr(sym->var.var_name));
			free_register(proc, &proc->local_pseudos, pseudo->regnum);
		}
	}
	END_FOR_EACH_PTR_REVERSE(sym);
	proc->current_scope = scope->parent;
}

static void linearize_function(struct linearizer_state *linearizer)
{
	struct proc *proc = linearizer->current_proc;
	assert(proc != NULL);
	struct ast_node *func_expr = proc->function_expr;
	assert(func_expr->type == AST_FUNCTION_EXPR);
	initialize_graph(proc);
	start_scope(linearizer, proc, func_expr->function_expr.main_block);
	linearize_function_args(linearizer);
	linearize_statement_list(proc, func_expr->function_expr.function_statement_list);
	end_scope(linearizer, proc);
}

static void output_pseudo(struct pseudo *pseudo, membuff_t *mb)
{
	switch (pseudo->type) {
	case PSEUDO_CONSTANT: {
		const struct constant *constant = pseudo->constant;
		const char *tc = "";
		if (constant->type == RAVI_TNUMFLT) {
			raviX_buffer_add_fstring(mb, "%.12f", constant->n);
			tc = "flt";
		} else if (constant->type == RAVI_TNUMINT) {
			raviX_buffer_add_fstring(mb, "%lld", (long long)constant->i);
			tc = "int";
		} else {
			raviX_buffer_add_fstring(mb, "'%s'", constant->s->str);
			tc = "s";
		}
		raviX_buffer_add_fstring(mb, " K%s(%d)", tc, pseudo->regnum);
	} break;
	case PSEUDO_TEMP_INT:
		raviX_buffer_add_fstring(mb, "Tint(%d)", pseudo->regnum);
		break;
	case PSEUDO_TEMP_FLT:
		raviX_buffer_add_fstring(mb, "Tflt(%d)", pseudo->regnum);
		break;
	case PSEUDO_TEMP_ANY:
		raviX_buffer_add_fstring(mb, "T(%d)", pseudo->regnum);
		break;
	case PSEUDO_RANGE_SELECT:
		raviX_buffer_add_fstring(mb, "T(%d[%d..])", pseudo->regnum, pseudo->range_pseudo->regnum);
		break;
	case PSEUDO_PROC:
		raviX_buffer_add_fstring(mb, "Proc(%d)", pseudo->proc->id);
		break;
	case PSEUDO_NIL:
		raviX_buffer_add_string(mb, "nil");
		break;
	case PSEUDO_FALSE:
		raviX_buffer_add_string(mb, "false");
		break;
	case PSEUDO_TRUE:
		raviX_buffer_add_string(mb, "true");
		break;
	case PSEUDO_SYMBOL:
		switch (pseudo->symbol->symbol_type) {
		case SYM_LOCAL: {
			raviX_buffer_add_fstring(mb, "local(%s, %d)", pseudo->symbol->var.var_name->str,
						 pseudo->regnum);
			break;
		}
		case SYM_UPVALUE: {
			raviX_buffer_add_fstring(mb, "Upval(%u)", pseudo->regnum);
			break;
		}
		case SYM_GLOBAL: {
			raviX_buffer_add_string(mb, pseudo->symbol->var.var_name->str);
			break;
		}
		default:
			// handle_error(proc->linearizer->ast_container, "feature not yet implemented");
			abort();
		}
		break;
	case PSEUDO_BLOCK: {
		raviX_buffer_add_fstring(mb, "L%d", pseudo->block ? (int) pseudo->block->index : -1);
		break;
	}
	case PSEUDO_RANGE: {
		raviX_buffer_add_fstring(mb, "T(%d..)", pseudo->regnum);
		break;
	}
	}
}

static const char *op_codenames[] = {
    "NOOP",	 "RET",	      "LOADK",	  "LOADNIL", "LOADBOOL", "ADD",	    "ADDff",  "ADDfi",	    "ADDii",
    "SUB",	 "SUBff",     "SUBfi",	  "SUBif",   "SUBii",	 "MUL",	    "MULff",  "MULfi",	    "MULii",
    "DIV",	 "DIVff",     "DIVfi",	  "DIVif",   "DIVii",	 "IDIV",    "BAND",   "BANDii",	    "BOR",
    "BORii",	 "BXOR",      "BXORii",	  "SHL",     "SHLii",	 "SHR",	    "SHRii",  "EQ",	    "EQii",
    "EQff",	 "LT",	      "LIii",	  "LTff",    "LE",	 "LEii",    "LEff",   "MOD",	    "POW",
    "CLOSURE",	 "UNM",	      "UNMi",	  "UNMf",    "LEN",	 "LENi",    "TOINT",  "TOFLT",	    "TOCLOSURE",
    "TOSTRING",	 "TOIARRAY",  "TOFARRAY", "TOTABLE", "TOTYPE",	 "NOT",	    "BNOT",   "LOADGLOBAL", "NEWTABLE",
    "NEWIARRAY", "NEWFARRAY", "PUT",	  "PUTik",   "PUTsk",	 "TPUT",    "TPUTik", "TPUTsk",	    "IAPUT",
    "IAPUTiv",	 "FAPUT",     "FAPUTfv",  "CBR",     "BR",	 "MOV",	    "CALL",   "GET",	    "GETik",
    "GETsk",	 "TGET",      "TGETik",	  "TGETsk",  "IAGET",	 "IAGETik", "FAGET",  "FAGETik",    "STOREGLOBAL"};

static void output_pseudo_list(struct pseudo_list *list, membuff_t *mb)
{
	struct pseudo *pseudo;
	raviX_buffer_add_string(mb, " {");
	int i = 0;
	FOR_EACH_PTR(list, pseudo)
	{
		if (i > 0)
			raviX_buffer_add_string(mb, ", ");
		output_pseudo(pseudo, mb);
		i++;
	}
	END_FOR_EACH_PTR(pseudo);
	raviX_buffer_add_string(mb, "}");
}

static void output_instruction(struct instruction *insn, membuff_t *mb)
{
	raviX_buffer_add_fstring(mb, "\t%s", op_codenames[insn->opcode]);
	if (insn->operands) {
		output_pseudo_list(insn->operands, mb);
	}
	if (insn->targets) {
		output_pseudo_list(insn->targets, mb);
	}
	raviX_buffer_add_string(mb, "\n");
}

static void output_instructions(struct instruction_list *list, membuff_t *mb)
{
	struct instruction *insn;
	FOR_EACH_PTR(list, insn) { output_instruction(insn, mb); }
	END_FOR_EACH_PTR(insn);
}

static void output_basic_block(struct proc *proc, struct basic_block *bb, membuff_t *mb)
{
	raviX_buffer_add_fstring(mb, "L%d", bb->index);
	if (bb2n(bb) == proc->entry) {
		raviX_buffer_add_string(mb, " (entry)\n");
	} else if (bb2n(bb) == proc->exit) {
		raviX_buffer_add_string(mb, " (exit)\n");
	} else {
		raviX_buffer_add_string(mb, "\n");
	}
	output_instructions(bb->insns, mb);
}

static void output_proc(struct proc *proc, membuff_t *mb)
{
	struct basic_block *bb;
	raviX_buffer_add_fstring(mb, "define Proc(%d)\n", proc->id);
	for (int i = 0; i < (int)proc->node_count; i++) {
		bb = n2bb(proc->nodes[i]);
		output_basic_block(proc, bb, mb);
	}
}

int raviX_ast_linearize(struct linearizer_state *linearizer)
{
	struct proc *proc = allocate_proc(linearizer, linearizer->ast_container->main_function);
	set_main_proc(linearizer, proc);
	set_current_proc(linearizer, proc);
	int rc = setjmp(linearizer->ast_container->env);
	if (rc == 0) {
		linearize_function(linearizer);
	}
	return rc;
}

void raviX_show_linearizer(struct linearizer_state *linearizer, membuff_t *mb)
{
	output_proc(linearizer->main_proc, mb);
	struct proc *proc;
	FOR_EACH_PTR(linearizer->all_procs, proc)
	{
		if (proc == linearizer->main_proc)
			continue;
		output_proc(proc, mb);
	}
	END_FOR_EACH_PTR(proc);
}

void raviX_output_linearizer(struct linearizer_state *linearizer, FILE *fp)
{
	membuff_t mb;
	raviX_buffer_init(&mb, 4096);
	raviX_show_linearizer(linearizer, &mb);
	fputs(mb.buf, fp);
}
