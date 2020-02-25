/*
Copyright (C) 2018-2020 Dibyendu Majumdar
*/

#include "fnv_hash.h"
#include "ptrlist.h"
#include "ravi_ast.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void handle_error(struct ast_container *container, const char *msg)
{
	// TODO source and line number
	membuff_add_string(&container->error_message, msg);
	longjmp(container->env, 1);
}

static struct pseudo *linearize_expression(struct proc *proc, struct ast_node *expr);
static struct basic_block *create_block(struct proc *proc);
static void start_block(struct proc *proc, struct basic_block *bb);
static void linearize_statement(struct proc *proc, struct ast_node *node);
static void linearize_statement_list(struct proc *proc, struct ast_node_list *list);
static void start_scope(struct linearizer *linearizer, struct proc *proc, struct block_scope *scope);
static void end_scope(struct linearizer *linearizer, struct proc *proc);
static void instruct_br(struct proc *proc, struct basic_block *target_block);
static bool is_block_terminated(struct basic_block *block);
static struct pseudo *instruct_move(struct proc *proc, struct pseudo *target, struct pseudo *src);

static inline unsigned alloc_reg(struct pseudo_generator *generator)
{
	if (generator->free_pos > 0) {
		return generator->free_regs[--generator->free_pos];
	}
	return generator->next_reg++;
}

static inline void free_reg(struct pseudo_generator *generator, unsigned reg)
{
	if (generator->free_pos == (sizeof generator->free_regs / sizeof generator->free_regs[0])) {
		/* TODO proper error handling */
		fprintf(stderr, "Out of register space\n");
		abort();
	}
	for (int i = 0; i < generator->free_pos; i++) {
		assert(generator->free_regs[i] != reg);
	}
	generator->free_regs[generator->free_pos++] = (uint8_t)reg;
}

/* Linearizer - WIP  */
void raviX_init_linearizer(struct linearizer *linearizer, struct ast_container *container)
{
	memset(linearizer, 0, sizeof *linearizer);
	linearizer->ast_container = container;
	raviX_allocator_init(&linearizer->edge_allocator, "edge_allocator", sizeof(struct edge), sizeof(double), CHUNK);
	raviX_allocator_init(&linearizer->instruction_allocator, "instruction_allocator", sizeof(struct instruction),
			     sizeof(double), CHUNK);
	raviX_allocator_init(&linearizer->ptrlist_allocator, "ptrlist_allocator", sizeof(struct ptr_list),
			     sizeof(double), CHUNK);
	raviX_allocator_init(&linearizer->pseudo_allocator, "pseudo_allocator", sizeof(struct pseudo), sizeof(double),
			     CHUNK);
	raviX_allocator_init(&linearizer->basic_block_allocator, "basic_block_allocator", sizeof(struct basic_block),
			     sizeof(double), CHUNK);
	raviX_allocator_init(&linearizer->proc_allocator, "proc_allocator", sizeof(struct proc), sizeof(double), CHUNK);
	raviX_allocator_init(&linearizer->unsized_allocator, "unsized_allocator", 0, sizeof(double), CHUNK);
	raviX_allocator_init(&linearizer->constant_allocator, "constant_allocator", sizeof(struct constant),
			     sizeof(double), CHUNK);
}

void raviX_destroy_linearizer(struct linearizer *linearizer)
{
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
}

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

static uint32_t hash_constant(const void *c)
{
	const struct constant *c1 = (const struct constant *)c;
	if (c1->type == RAVI_TNUMINT)
		return (uint32_t)c1->i;
	else if (c1->type == RAVI_TNUMFLT)
		return (uint32_t)c1->n; // FIXME maybe use Lua's hash gen
	else
		return (uint32_t)c1->s;
}

static const struct constant *add_constant(struct proc *proc, const struct constant *c)
{
	struct set_entry *entry = set_search(proc->constants, c);
	if (entry == NULL) {
		int reg = proc->num_constants++;
		struct constant *c1 = raviX_allocator_allocate(&proc->linearizer->constant_allocator, 0);
		assert(c1);
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

static const struct constant *allocate_constant_for_i(struct proc *proc, int i)
{
	struct constant c = {.type = RAVI_TNUMINT, .i = i};
	return add_constant(proc, &c);
}

static const struct constant *allocate_string_constant(struct proc *proc, const char *s)
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

struct pseudo *allocate_closure_pseudo(struct linearizer *linearizer, struct proc *proc)
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
	unsigned reg = alloc_reg(gen);
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = pseudo_type;
	pseudo->regnum = reg;
	pseudo->temp_type = type;
	return pseudo;
}

struct pseudo *allocate_range_pseudo(struct proc *proc, unsigned regnum, int range)
{
	assert(range != -1 || proc->temp_pseudos.next_reg >= regnum);
	struct pseudo *pseudo = raviX_allocator_allocate(&proc->linearizer->pseudo_allocator, 0);
	pseudo->type = PSEUDO_RANGE;
	pseudo->regnum = regnum;
	pseudo->range = range;
	return pseudo;
}

void free_temp_pseudo(struct proc *proc, struct pseudo *pseudo)
{
	struct pseudo_generator *gen;
	switch (pseudo->type) {
	case PSEUDO_TEMP_FLT:
		gen = &proc->temp_int_pseudos;
		break;
	case PSEUDO_TEMP_INT:
		gen = &proc->temp_flt_pseudos;
		break;
	case PSEUDO_RANGE:
		if (pseudo->range != -1)
			return;
	case PSEUDO_TEMP_ANY:
		gen = &proc->temp_pseudos;
		break;
	default:
		// Not a temp, so no need to do anything
		return;
	}
	free_reg(gen, pseudo->regnum);
}

/**
 * Allocate a new proc. If there is a current proc, then the new proc gets added to the
 * current procs children.
 */
static struct proc *allocate_proc(struct linearizer *linearizer, struct ast_node *function_expr)
{
	assert(function_expr->type == AST_FUNCTION_EXPR);
	struct proc *proc = raviX_allocator_allocate(&linearizer->proc_allocator, 0);
	proc->function_expr = function_expr;
	ptrlist_add((struct ptr_list **)&linearizer->all_procs, proc, &linearizer->ptrlist_allocator);
	if (linearizer->current_proc) {
		proc->parent = linearizer->current_proc;
		ptrlist_add((struct ptr_list **)&linearizer->current_proc->procs, proc, &linearizer->ptrlist_allocator);
	}
	proc->constants = set_create(hash_constant, compare_constants);
	proc->linearizer = linearizer;
	return proc;
}

static void set_main_proc(struct linearizer *linearizer, struct proc *proc)
{
	assert(linearizer->main_proc == NULL);
	assert(linearizer->current_proc == NULL);
	linearizer->main_proc = proc;
	assert(proc->function_expr->function_expr.parent_function == NULL);
}

static inline void set_current_proc(struct linearizer *linearizer, struct proc *proc)
{
	linearizer->current_proc = proc;
}

static void linearize_function_args(struct linearizer *linearizer)
{
	struct proc *proc = linearizer->current_proc;
	struct ast_node *func_expr = proc->function_expr;
	struct lua_symbol *sym;
	FOR_EACH_PTR(func_expr->function_expr.args, sym)
	{
		handle_error(linearizer->ast_container, "feature not yet implemented");
		// printf("Assigning register %d to argument %s\n", (int)reg, getstr(sym->var.var_name));
	}
	END_FOR_EACH_PTR(sym);
}

static void linearize_statement_list(struct proc *proc, struct ast_node_list *list)
{
	struct ast_node *node;
	FOR_EACH_PTR(list, node) { linearize_statement(proc, node); }
	END_FOR_EACH_PTR(node);
}

static struct instruction *alloc_instruction(struct proc *proc, enum opcode op)
{
	struct instruction *insn = raviX_allocator_allocate(&proc->linearizer->instruction_allocator, 0);
	insn->opcode = op;
	return insn;
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

static struct pseudo *linearize_unaryop(struct proc *proc, struct ast_node *node)
{
	UnOpr op = node->unary_expr.unary_op;
	struct pseudo *subexpr = linearize_expression(proc, node->unary_expr.expr);
	ravitype_t subexpr_type = node->unary_expr.expr->common_expr.type.type_code;
	enum opcode targetop = op_nop;
	switch (op) {
	case OPR_MINUS:
		if (subexpr_type == RAVI_TNUMINT)
			targetop = op_unmi;
		else if (subexpr_type == RAVI_TNUMFLT)
			targetop = op_unmf;
		else
			targetop = op_unm;
		break;
	case OPR_LEN:
		if (subexpr_type == RAVI_TARRAYINT || subexpr_type == RAVI_TARRAYFLT)
			targetop = op_leni;
		else
			targetop = op_len;
		break;
	case OPR_TO_INTEGER:
		targetop = subexpr_type != RAVI_TNUMINT ? op_toint : op_nop;
		break;
	case OPR_TO_NUMBER:
		targetop = subexpr_type != RAVI_TNUMFLT ? op_toflt : op_nop;
		break;
	case OPR_TO_CLOSURE:
		targetop = subexpr_type != RAVI_TFUNCTION ? op_toclosure : op_nop;
		break;
	case OPR_TO_STRING:
		targetop = subexpr_type != RAVI_TSTRING ? op_tostring : op_nop;
		break;
	case OPR_TO_INTARRAY:
		targetop = subexpr_type != RAVI_TARRAYINT ? op_toiarray : op_nop;
		break;
	case OPR_TO_NUMARRAY:
		targetop = subexpr_type != RAVI_TARRAYFLT ? op_tofarray : op_nop;
		break;
	case OPR_TO_TABLE:
		targetop = subexpr_type != RAVI_TTABLE ? op_totable : op_nop;
		break;
	case OPR_TO_TYPE:
		targetop = op_totype;
		break;
	case OPR_NOT:
		targetop = op_not;
		break;
	case OPR_BNOT:
		targetop = op_bnot;
		break;
	default:
		handle_error(proc->linearizer->ast_container, "unexpected unary op");
		break;
	}
	if (targetop == op_nop) {
		return subexpr;
	}
	struct instruction *insn = alloc_instruction(proc, targetop);
	struct pseudo *target = subexpr;
	if (op == OPR_TO_TYPE) {
		const struct constant *tname_constant = allocate_string_constant(proc, node->unary_expr.type.type_name);
		struct pseudo *tname_pseudo = allocate_constant_pseudo(proc, tname_constant);
		ptrlist_add((struct ptr_list **)&insn->operands, tname_pseudo, &proc->linearizer->ptrlist_allocator);
	} else if (op == OPR_NOT || op == OPR_BNOT) {
		ptrlist_add((struct ptr_list **)&insn->operands, target, &proc->linearizer->ptrlist_allocator);
		target = allocate_temp_pseudo(proc, RAVI_TANY);
	} else if (op == OPR_MINUS || op == OPR_LEN) {
		ptrlist_add((struct ptr_list **)&insn->operands, target, &proc->linearizer->ptrlist_allocator);
		target = allocate_temp_pseudo(proc, subexpr_type);
	}
	ptrlist_add((struct ptr_list **)&insn->targets, target, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
	return target;
}

static struct pseudo *instruct_move(struct proc *proc, struct pseudo *target, struct pseudo *src)
{
	struct instruction *mov = alloc_instruction(proc, op_mov);
	ptrlist_add((struct ptr_list **)&mov->operands, src, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&mov->targets, target, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, mov, &proc->linearizer->ptrlist_allocator);
	return target;
}

static void instruct_cbr(struct proc *proc, struct pseudo *conditin_pseudo, struct basic_block *true_block,
			 struct basic_block *false_block)
{
	struct pseudo *true_pseudo = allocate_block_pseudo(proc, true_block);
	struct pseudo *false_pseudo = allocate_block_pseudo(proc, false_block);
	struct instruction *insn = alloc_instruction(proc, op_cbr);
	ptrlist_add((struct ptr_list **)&insn->operands, conditin_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->targets, true_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->targets, false_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
}

static void instruct_br(struct proc *proc, struct basic_block *target_block)
{
	if (is_block_terminated(proc->current_bb)) {
		start_block(proc, create_block(proc));
	}
	struct pseudo *pseudo = allocate_block_pseudo(proc, target_block);
	struct instruction *insn = alloc_instruction(proc, op_br);
	ptrlist_add((struct ptr_list **)&insn->targets, pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
}

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
	instruct_br(proc, end_block);

	start_block(proc, end_block);

	return result;
}

/* Type checker - WIP  */
static struct pseudo *linearize_binaryop(struct proc *proc, struct ast_node *node)
{
	BinOpr op = node->binary_expr.binary_op;

	if (op == OPR_AND) {
		return linearize_bool(proc, node, true);
	} else if (op == OPR_OR) {
		return linearize_bool(proc, node, false);
	}

	struct ast_node *e1 = node->binary_expr.expr_left;
	struct ast_node *e2 = node->binary_expr.expr_right;
	struct pseudo *operand1 = linearize_expression(proc, e1);
	struct pseudo *operand2 = linearize_expression(proc, e2);

	enum opcode targetop;
	switch (op) {
	case OPR_ADD:
		targetop = op_add;
		break;
	case OPR_SUB:
		targetop = op_sub;
		break;
	case OPR_MUL:
		targetop = op_mul;
		break;
	case OPR_DIV:
		targetop = op_div;
		break;
	case OPR_IDIV:
		targetop = op_idiv;
		break;
	case OPR_BAND:
		targetop = op_band;
		break;
	case OPR_BOR:
		targetop = op_bor;
		break;
	case OPR_BXOR:
		targetop = op_bxor;
		break;
	case OPR_SHL:
		targetop = op_shl;
		break;
	case OPR_SHR:
		targetop = op_shr;
		break;
	case OPR_EQ:
	case OPR_NE:
		targetop = op_eq;
		break;
	case OPR_LT:
	case OPR_GT:
		targetop = op_lt;
		break;
	case OPR_LE:
	case OPR_GE:
		targetop = op_le;
		break;
	case OPR_MOD:
		targetop = op_mod;
		break;
	case OPR_POW:
		targetop = op_pow;
		break;
	default:
		handle_error(proc->linearizer->ast_container, "unexpected binary op");
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
		swap = op == OPR_NE || op == OPR_GT || op == OPR_GE;
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
	}

	ravitype_t target_type = node->binary_expr.type.type_code;
	struct pseudo *target = allocate_temp_pseudo(proc, target_type);
	struct instruction *insn = alloc_instruction(proc, targetop);
	ptrlist_add((struct ptr_list **)&insn->operands, operand1, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->operands, operand2, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->targets, target, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
	free_temp_pseudo(proc, operand1);
	free_temp_pseudo(proc, operand2);

	return target;
}

/* generates closure instruction - linearize a proc, and then add instruction to create closure from it */
static struct pseudo *linearize_function_expr(struct proc *proc, struct ast_node *expr)
{
	struct proc *curproc = proc->linearizer->current_proc;
	struct proc *newproc = allocate_proc(proc->linearizer, expr);
	set_current_proc(proc->linearizer, newproc);
	// printf("linearizing function\n");

	set_current_proc(proc->linearizer, curproc); // restore the proc
	ravitype_t target_type = expr->function_expr.type.type_code;
	struct pseudo *target = allocate_temp_pseudo(proc, target_type);
	struct pseudo *operand = allocate_closure_pseudo(proc->linearizer, newproc);
	struct instruction *insn = alloc_instruction(proc, op_closure);
	ptrlist_add((struct ptr_list **)&insn->operands, operand, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->targets, target, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);

	return target;
}

static struct pseudo *linearize_symbol_expression(struct proc *proc, struct ast_node *expr)
{
	struct lua_symbol *sym = expr->symbol_expr.var;
	if (sym->symbol_type == SYM_GLOBAL) {
		struct pseudo *target = allocate_temp_pseudo(proc, RAVI_TANY);
		struct pseudo *operand = allocate_symbol_pseudo(proc, sym, 0); // no register actually
		struct instruction *insn = alloc_instruction(proc, op_loadglobal);
		target->insn = insn;
		ptrlist_add((struct ptr_list **)&insn->operands, operand, &proc->linearizer->ptrlist_allocator);
		ptrlist_add((struct ptr_list **)&insn->targets, target, &proc->linearizer->ptrlist_allocator);
		ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
		return target;
	} else if (sym->symbol_type == SYM_LOCAL) {
		return sym->var.pseudo;
	} else {
		handle_error(proc->linearizer->ast_container, "feature not yet implemented");
		return NULL;
	}
}

// static struct pseudo *linearize_index_expression(struct proc *proc, struct ast_node *expr)
//{
//	return linearize_expression(proc, expr->indexed_assign_expr.index_expr);
//}

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
	}
	struct pseudo *target_pseudo = allocate_temp_pseudo(proc, target_type);
	struct instruction *insn = alloc_instruction(proc, op);
	ptrlist_add((struct ptr_list **)&insn->operands, container_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->operands, key_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->targets, target_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
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

	struct instruction *insn = alloc_instruction(proc, op);
	ptrlist_add((struct ptr_list **)&insn->operands, table, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->operands, index_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&insn->operands, value_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
}

static void convert_loadglobal_to_store(struct proc* proc, struct instruction* insn, struct pseudo* value_pseudo,
	ravitype_t value_type) {
	ptrlist_remove((struct ptr_list**) & proc->current_bb->insns, insn, 1);
	insn->opcode = op_storeglobal;
	ptrlist_add((struct ptr_list**) & insn->operands, value_pseudo, &proc->linearizer->ptrlist_allocator);
	struct pseudo* get_target = ptrlist_delete_last((struct ptr_list**) & insn->targets);
	free_temp_pseudo(proc, get_target);
	ptrlist_add((struct ptr_list**) & proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
}

static void convert_indexed_load_to_store(struct proc *proc, struct instruction *insn, struct pseudo *value_pseudo,
					  ravitype_t value_type)
{
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
	ptrlist_remove((struct ptr_list **)&proc->current_bb->insns, insn, 1);
	insn->opcode = putop;
	ptrlist_add((struct ptr_list **)&insn->operands, value_pseudo, &proc->linearizer->ptrlist_allocator);
	struct pseudo *get_target = ptrlist_delete_last((struct ptr_list **)&insn->targets);
	free_temp_pseudo(proc, get_target);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
}

/**
 * Lua function calls can return multiple values, and the caller decides how many values to accept.
 * We indicate multiple values using a PSEUDO_RANGE with range = -1.
 * We also handle method call:
 * <pseudo>:name(...) -> is translated to <pseudo>.name(<pseudo>, ...)
 */
static struct pseudo *linearize_function_call_expression(struct proc *proc, struct ast_node *expr,
							 struct ast_node *callsite_expr, struct pseudo *callsite_pseudo)
{
	struct instruction *insn = alloc_instruction(proc, op_call);
	struct pseudo *self_arg = NULL;
	if (expr->function_call_expr.method_name) {
		const struct constant *name_constant =
		    allocate_string_constant(proc, expr->function_call_expr.method_name);
		struct pseudo *name_pseudo = allocate_constant_pseudo(proc, name_constant);
		self_arg = callsite_pseudo; /* The original callsite must be passed as 'self' */
		callsite_pseudo = instruct_indexed_load(proc, callsite_expr->common_expr.type.type_code,
							callsite_pseudo, RAVI_TSTRING, name_pseudo, RAVI_TANY);
	}

	ptrlist_add((struct ptr_list **)&insn->operands, callsite_pseudo, &proc->linearizer->ptrlist_allocator);
	if (self_arg)
		ptrlist_add((struct ptr_list **)&insn->operands, self_arg, &proc->linearizer->ptrlist_allocator);

	struct ast_node *arg;
	int argc = ptrlist_size((const struct ptr_list *)expr->function_call_expr.arg_list);
	FOR_EACH_PTR(expr->function_call_expr.arg_list, arg)
	{
		argc -= 1;
		struct pseudo *arg_pseudo = linearize_expression(proc, arg);
		if (argc != 0 && arg_pseudo->type == PSEUDO_RANGE) {
			// Not last one, so range can only be 1
			arg_pseudo->range = 1;
		}
		ptrlist_add((struct ptr_list **)&insn->operands, arg_pseudo, &proc->linearizer->ptrlist_allocator);
	}
	END_FOR_EACH_PTR(arg);

	struct pseudo *return_pseudo = allocate_range_pseudo(
	    proc, callsite_pseudo->regnum, -1); /* Base reg for function call - where return values will be placed */
	ptrlist_add((struct ptr_list **)&insn->targets, return_pseudo, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
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
 * Lua parser adoes this by creating a VINDEXED node which is only coverted to load/store
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
			prev_pseudo->range = 1;
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
	// copy_type(node->suffixed_expr.type, prev_node->common_expr.type);
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
		const struct constant *constant = allocate_constant_for_i(proc, next++);
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
	struct instruction *insn = alloc_instruction(proc, op);
	ptrlist_add((struct ptr_list **)&insn->targets, target, &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);

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
	}  
	else if (var_pseudo->insn && var_pseudo->insn->opcode == op_loadglobal) {
		convert_loadglobal_to_store(proc, var_pseudo->insn, val_pseudo, val_type);
	}
	else {
		instruct_move(proc, var_pseudo, val_pseudo); // TODO add type specialization
	}
}

struct node_info {
	struct ast_node *node;
	struct pseudo *pseudo;
};

static void linearize_expression_statement(struct proc *proc, struct ast_node *node)
{
	struct ast_node *var;
	struct ast_node *expr;

	int nv = ptrlist_size((const struct ptr_list *)node->expression_stmt.var_expr_list);
	struct node_info *varinfo = (struct node_info *)alloca(nv * sizeof(struct node_info));
	int i = 0;
	FOR_EACH_PTR(node->expression_stmt.var_expr_list, var)
	{
		struct pseudo *var_pseudo = linearize_expression(proc, var);
		varinfo[i].node = var;
		varinfo[i].pseudo = var_pseudo;
		i++;
	}
	END_FOR_EACH_PTR(var);

	int ne = ptrlist_size((const struct ptr_list *)node->expression_stmt.expr_list);
	struct node_info *valinfo = (struct node_info *)alloca(ne * sizeof(struct node_info));
	struct pseudo *last_val_pseudo = NULL;
	i = 0;
	FOR_EACH_PTR(node->expression_stmt.expr_list, expr)
	{
		struct pseudo *val_pseudo = last_val_pseudo = linearize_expression(proc, expr);
		valinfo[i].node = expr;
		valinfo[i].pseudo = val_pseudo;
		i++;
		if (i < ne && val_pseudo->type == PSEUDO_RANGE) {
			val_pseudo->range = 1;
		}
	}
	END_FOR_EACH_PTR(var);

	while (nv > 0) {
		if (nv > ne) {
			if (last_val_pseudo->type == PSEUDO_RANGE) {
				int regnum = last_val_pseudo->regnum + (nv - ne);
				linearize_store_var(proc, varinfo[nv - 1].node->common_expr.type.type_code,
						    varinfo[nv - 1].pseudo,
						    valinfo[ne - 1].node->common_expr.type.type_code,
						    allocate_range_pseudo(proc, regnum, 1));
			} else {
				// TODO store NIL
			}
			nv--;
		} else {
			if (valinfo[ne - 1].pseudo->type == PSEUDO_RANGE && valinfo[ne - 1].pseudo->range != 1)
				valinfo[ne - 1].pseudo = allocate_range_pseudo(proc, valinfo[ne - 1].pseudo->regnum, 1);
			linearize_store_var(proc, varinfo[nv - 1].node->common_expr.type.type_code,
					    varinfo[nv - 1].pseudo, valinfo[ne - 1].node->common_expr.type.type_code,
					    valinfo[ne - 1].pseudo);
			free_temp_pseudo(proc, valinfo[ne - 1].pseudo);
			nv--;
			ne--;
		}
	}
}

static struct pseudo *linearize_expression(struct proc *proc, struct ast_node *expr)
{
	switch (expr->type) {
	case AST_LITERAL_EXPR: {
		return linearize_literal(proc, expr);
	} break;
	case AST_BINARY_EXPR: {
		return linearize_binaryop(proc, expr);
	} break;
	case AST_FUNCTION_EXPR: {
		return linearize_function_expr(proc, expr);
	} break;
	case AST_UNARY_EXPR: {
		return linearize_unaryop(proc, expr);
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
	// case AST_FUNCTION_CALL_EXPR: {
	//	return linearize_function_call_expression(proc, expr);
	//} break;
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
			pseudo->range = 1; // Only accept one result unless it is the last expr
		}
		ptrlist_add((struct ptr_list **)pseudo_list, pseudo, &proc->linearizer->ptrlist_allocator);
	}
	END_FOR_EACH_PTR(expr);
}

static void linearize_return(struct proc *proc, struct ast_node *node)
{
	assert(node->type == AST_RETURN_STMT);
	struct instruction *insn = alloc_instruction(proc, op_ret);
	linearize_expr_list(proc, node->return_stmt.expr_list, insn, &insn->operands);
	ptrlist_add((struct ptr_list **)&insn->targets, allocate_block_pseudo(proc, n2bb(proc->exit)),
		    &proc->linearizer->ptrlist_allocator);
	ptrlist_add((struct ptr_list **)&proc->current_bb->insns, insn, &proc->linearizer->ptrlist_allocator);
	// FIXME add edge to exit block
	// FIXME terminate block
	// FIXME free all temps
}

static bool is_block_terminated(struct basic_block *block)
{
	struct instruction *last_insn = (struct instruction *)ptrlist_last((struct ptr_list *)block->insns);
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

static void linearize_test_then(struct proc *proc, struct ast_node *node, struct basic_block *true_block,
				struct basic_block *false_block)
{
	start_block(proc, true_block);
	start_scope(proc->linearizer, proc, node->test_then_block.test_then_scope);
	linearize_statement_list(proc, node->test_then_block.test_then_statement_list);
	end_scope(proc->linearizer, proc);
	if (!is_block_terminated(proc->current_bb))
		instruct_br(proc, false_block);
}

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
			instruct_br(proc, end_block);
	}

	start_block(proc, end_block);
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
		// typecheck_local_statement(container, function, node);
		handle_error(proc->linearizer->ast_container, "AST_LOCAL_STMT not yet implemented");
		break;
	}
	case AST_FUNCTION_STMT: {
		// typecheck_ast_node(container, function, node->function_stmt.function_expr);
		handle_error(proc->linearizer->ast_container, "AST_FUNCTION_STMT not yet implemented");
		break;
	}
	case AST_LABEL_STMT: {
		handle_error(proc->linearizer->ast_container, "AST_LABEL_STMT not yet implemented");
		break;
	}
	case AST_GOTO_STMT: {
		handle_error(proc->linearizer->ast_container, "AST_GOTO_STMT not yet implemented");
		break;
	}
	case AST_DO_STMT: {
		handle_error(proc->linearizer->ast_container, "AST_DO_STMT not yet implemented");
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
		// typecheck_for_num_statment(container, function, node);
		handle_error(proc->linearizer->ast_container, "AST_FORNUM_STMT not yet implemented");
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
 * All future instructions will be added to the end of this block
 */
static void start_block(struct proc *proc, struct basic_block *bb)
{
	// printf("Starting block %d\n", bb->index);
	if (proc->current_bb && !is_block_terminated(proc->current_bb)) {
		instruct_br(proc, bb);
	}
	proc->current_bb = bb;
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
static void start_scope(struct linearizer *linearizer, struct proc *proc, struct block_scope *scope)
{
	proc->current_scope = scope;
	struct lua_symbol *sym;
	FOR_EACH_PTR(scope->symbol_list, sym)
	{
		if (sym->symbol_type == SYM_LOCAL) {
			uint8_t reg = alloc_reg(&proc->local_pseudos);
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
static void end_scope(struct linearizer *linearizer, struct proc *proc)
{
	struct block_scope *scope = proc->current_scope;
	struct lua_symbol *sym;
	FOR_EACH_PTR_REVERSE(scope->symbol_list, sym)
	{
		if (sym->symbol_type == SYM_LOCAL) {
			struct pseudo *pseudo = sym->var.pseudo;
			assert(pseudo && pseudo->type == PSEUDO_SYMBOL && pseudo->symbol == sym);
			// printf("Free register %d for local %s\n", (int)pseudo->regnum, getstr(sym->var.var_name));
			free_reg(&proc->local_pseudos, pseudo->regnum);
		}
	}
	END_FOR_EACH_PTR_REVERSE(sym);
	proc->current_scope = scope->parent;
}

static void linearize_function(struct linearizer *linearizer)
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

void output_pseudo(struct pseudo *pseudo, membuff_t *mb)
{
	switch (pseudo->type) {
	case PSEUDO_CONSTANT: {
		const struct constant *constant = pseudo->constant;
		const char *tc = "";
		if (constant->type == RAVI_TNUMFLT) {
			membuff_add_fstring(mb, "%.12f", constant->n);
			tc = "flt";
		} else if (constant->type == RAVI_TNUMINT) {
			membuff_add_fstring(mb, "%ld", constant->i);
			tc = "int";
		} else {
			membuff_add_fstring(mb, "'%s'", constant->s);
			tc = "s";
		}
		membuff_add_fstring(mb, " K%s(%d)", tc, pseudo->regnum);
	} break;
	case PSEUDO_TEMP_INT:
		membuff_add_fstring(mb, "Tint(%d)", pseudo->regnum);
		break;
	case PSEUDO_TEMP_FLT:
		membuff_add_fstring(mb, "Tflt(%d)", pseudo->regnum);
		break;
	case PSEUDO_TEMP_ANY:
		membuff_add_fstring(mb, "T(%d)", pseudo->regnum);
		break;
	case PSEUDO_PROC:
		membuff_add_fstring(mb, "Proc(%p)", pseudo->proc);
		break;
	case PSEUDO_NIL:
		membuff_add_string(mb, "nil");
		break;
	case PSEUDO_FALSE:
		membuff_add_string(mb, "false");
		break;
	case PSEUDO_TRUE:
		membuff_add_string(mb, "true");
		break;
	case PSEUDO_SYMBOL:
		switch (pseudo->symbol->symbol_type) {
		case SYM_UPVALUE:
		case SYM_LOCAL:
		case SYM_GLOBAL: {
			membuff_add_string(mb, pseudo->symbol->var.var_name);
			break;
		}
		default:
			// handle_error(proc->linearizer->ast_container, "feature not yet implemented");
			abort();
		}
		break;
	case PSEUDO_BLOCK: {
		membuff_add_fstring(mb, "L%d", pseudo->block->index);
		break;
	}
	case PSEUDO_RANGE: {
		if (pseudo->range != 1) {
			membuff_add_fstring(mb, "T(%d..%d)", pseudo->regnum, pseudo->range);
		}
		else {
			membuff_add_fstring(mb, "T(%d)", pseudo->regnum, pseudo->range);
		}
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
    "GETsk",	 "TGET",      "TGETik",	  "TGETsk",  "IAGET",	 "IAGETik", "FAGET",  "FAGETik", "STOREGLOBAL"
};

void output_pseudo_list(struct pseudo_list *list, membuff_t *mb)
{
	struct pseudo *pseudo;
	membuff_add_string(mb, " {");
	int i = 0;
	FOR_EACH_PTR(list, pseudo)
	{
		if (i > 0)
			membuff_add_string(mb, ", ");
		output_pseudo(pseudo, mb);
		i++;
	}
	END_FOR_EACH_PTR(pseudo);
	membuff_add_string(mb, "}");
}

void output_instruction(struct instruction *insn, membuff_t *mb)
{
	membuff_add_fstring(mb, "\t%s", op_codenames[insn->opcode]);
	if (insn->operands) {
		output_pseudo_list(insn->operands, mb);
	}
	if (insn->targets) {
		output_pseudo_list(insn->targets, mb);
	}
	membuff_add_string(mb, "\n");
}

void output_instructions(struct instruction_list *list, membuff_t *mb)
{
	struct instruction *insn;
	FOR_EACH_PTR(list, insn) { output_instruction(insn, mb); }
	END_FOR_EACH_PTR(insn);
}

void output_basic_block(struct proc *proc, struct basic_block *bb, membuff_t *mb)
{
	membuff_add_fstring(mb, "L%d", bb->index);
	if (bb2n(bb) == proc->entry) {
		membuff_add_string(mb, " (entry)\n");
	} else if (bb2n(bb) == proc->exit) {
		membuff_add_string(mb, " (exit)\n");
	} else {
		membuff_add_string(mb, "\n");
	}
	output_instructions(bb->insns, mb);
}

void output_proc(struct proc *proc, membuff_t *mb)
{
	struct basic_block *bb;
	for (int i = 0; i < (int)proc->node_count; i++) {
		bb = n2bb(proc->nodes[i]);
		output_basic_block(proc, bb, mb);
	}
}

int raviX_ast_linearize(struct linearizer *linearizer)
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

void raviX_show_linearizer(struct linearizer *linearizer, membuff_t *mb) { output_proc(linearizer->main_proc, mb); }

void raviX_output_linearizer(struct linearizer *linearizer, FILE *fp)
{
	membuff_t mb;
	membuff_init(&mb, 4096);
	raviX_show_linearizer(linearizer, &mb);
	fputs(mb.buf, fp);
}
