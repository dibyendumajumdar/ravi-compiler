/*
Copyright (C) 2018-2020 Dibyendu Majumdar
*/
#include <ravi_ast.h>

static bool is_type_same(const struct var_type *a, const struct var_type *b)
{
	// String comparion of type_name relies upon strings being interned
	return a->type_code == b->type_code && a->type_name == b->type_name;
}

static void handle_error(struct compiler_state *container, const char *msg)
{
	// TODO source and line number
	raviX_buffer_add_string(&container->error_message, msg);
	longjmp(container->env, 1);
}

/* Type checker - WIP  */
static void typecheck_ast_node(struct compiler_state *container, struct ast_node *function, struct ast_node *node);

/* Type checker - WIP  */
static void typecheck_ast_list(struct compiler_state *container, struct ast_node *function, struct ast_node_list *list)
{
	struct ast_node *node;
	FOR_EACH_PTR(list, node) { typecheck_ast_node(container, function, node); }
	END_FOR_EACH_PTR(node);
}

/* Type checker - WIP  */
static void typecheck_unary_operator(struct compiler_state *container, struct ast_node *function, struct ast_node *node)
{
	UnOpr op = node->unary_expr.unary_op;
	typecheck_ast_node(container, function, node->unary_expr.expr);
	ravitype_t subexpr_type = node->unary_expr.expr->common_expr.type.type_code;
	switch (op) {
	case UNOPR_MINUS:
		if (subexpr_type == RAVI_TNUMINT) {
			set_type(&node->unary_expr.type, RAVI_TNUMINT);
		} else if (subexpr_type == RAVI_TNUMFLT) {
			set_type(&node->unary_expr.type, RAVI_TNUMFLT);
		}
		break;
	case UNOPR_LEN:
		if (subexpr_type == RAVI_TARRAYINT || subexpr_type == RAVI_TARRAYFLT) {
			set_type(&node->unary_expr.type, RAVI_TNUMINT);
		}
		break;
	case UNOPR_TO_INTEGER:
		set_type(&node->unary_expr.type, RAVI_TNUMINT);
		break;
	case UNOPR_TO_NUMBER:
		set_type(&node->unary_expr.type, RAVI_TNUMFLT);
		break;
	case UNOPR_TO_CLOSURE:
		set_type(&node->unary_expr.type, RAVI_TFUNCTION);
		break;
	case UNOPR_TO_STRING:
		set_type(&node->unary_expr.type, RAVI_TSTRING);
		break;
	case UNOPR_TO_INTARRAY:
		set_type(&node->unary_expr.type, RAVI_TARRAYINT);
		if (node->unary_expr.expr->type == AST_TABLE_EXPR) {
			set_type(&node->unary_expr.expr->table_expr.type, RAVI_TARRAYINT);
		}
		break;
	case UNOPR_TO_NUMARRAY:
		set_type(&node->unary_expr.type, RAVI_TARRAYFLT);
		if (node->unary_expr.expr->type == AST_TABLE_EXPR) {
			set_type(&node->unary_expr.expr->table_expr.type, RAVI_TARRAYFLT);
		}
		break;
	case UNOPR_TO_TABLE:
		set_type(&node->unary_expr.type, RAVI_TTABLE);
		break;
	case UNOPR_TO_TYPE:
		assert(node->unary_expr.type.type_name != NULL); // Should already be set by the parser
		set_typecode(&node->unary_expr.type, RAVI_TUSERDATA);
		break;
	default:
		break;
	}
}

/* Type checker - WIP  */
static void typecheck_binary_operator(struct compiler_state *container, struct ast_node *function, struct ast_node *node)
{
	BinOpr op = node->binary_expr.binary_op;
	struct ast_node *e1 = node->binary_expr.expr_left;
	struct ast_node *e2 = node->binary_expr.expr_right;
	typecheck_ast_node(container, function, e1);
	typecheck_ast_node(container, function, e2);
	switch (op) {
	case BINOPR_ADD:
	case BINOPR_SUB:
	case BINOPR_MUL:
	case BINOPR_DIV:
		if (e1->common_expr.type.type_code == RAVI_TNUMFLT && e2->common_expr.type.type_code == RAVI_TNUMFLT)
			set_typecode(&node->binary_expr.type, RAVI_TNUMFLT);
		else if (e1->common_expr.type.type_code == RAVI_TNUMFLT &&
			 e2->common_expr.type.type_code == RAVI_TNUMINT)
			set_typecode(&node->binary_expr.type, RAVI_TNUMFLT);
		else if (e1->common_expr.type.type_code == RAVI_TNUMINT &&
			 e2->common_expr.type.type_code == RAVI_TNUMFLT)
			set_typecode(&node->binary_expr.type, RAVI_TNUMFLT);
		else if (op != BINOPR_DIV && e1->common_expr.type.type_code == RAVI_TNUMINT &&
			 e2->common_expr.type.type_code == RAVI_TNUMINT)
			set_typecode(&node->binary_expr.type, RAVI_TNUMINT);
		else if (op == BINOPR_DIV && e1->common_expr.type.type_code == RAVI_TNUMINT &&
			 e2->common_expr.type.type_code == RAVI_TNUMINT)
			set_typecode(&node->binary_expr.type, RAVI_TNUMFLT);
		break;
	case BINOPR_IDIV:
		if (e1->common_expr.type.type_code == RAVI_TNUMINT && e2->common_expr.type.type_code == RAVI_TNUMINT)
			set_typecode(&node->binary_expr.type, RAVI_TNUMINT);
		// FIXME missing cases
		break;
	case BINOPR_BAND:
	case BINOPR_BOR:
	case BINOPR_BXOR:
	case BINOPR_SHL:
	case BINOPR_SHR:
		if ((e1->common_expr.type.type_code == RAVI_TNUMFLT ||
		     e1->common_expr.type.type_code == RAVI_TNUMINT) &&
		    (e2->common_expr.type.type_code == RAVI_TNUMFLT || e2->common_expr.type.type_code == RAVI_TNUMINT))
			set_typecode(&node->binary_expr.type, RAVI_TNUMINT);
		break;
	case BINOPR_EQ:
	case BINOPR_NE:
	case BINOPR_GE:
	case BINOPR_GT:
	case BINOPR_LE:
	case BINOPR_LT:
		/* This case is not handled in default parser - why? */
		if ((e1->common_expr.type.type_code == RAVI_TNUMINT || e1->common_expr.type.type_code == RAVI_TNUMFLT ||
		     e1->common_expr.type.type_code == RAVI_TBOOLEAN) &&
		    (e2->common_expr.type.type_code == RAVI_TNUMFLT || e2->common_expr.type.type_code == RAVI_TNUMINT ||
		     e2->common_expr.type.type_code == RAVI_TBOOLEAN))
			set_typecode(&node->binary_expr.type, RAVI_TBOOLEAN);
		break;
	case BINOPR_POW:
		if ((e1->common_expr.type.type_code == RAVI_TNUMFLT ||
		     e1->common_expr.type.type_code == RAVI_TNUMINT) &&
		    (e2->common_expr.type.type_code == RAVI_TNUMFLT || e2->common_expr.type.type_code == RAVI_TNUMINT))
			set_typecode(&node->binary_expr.type, RAVI_TNUMFLT);
		break;
	case BINOPR_MOD:
		if (e1->common_expr.type.type_code == RAVI_TNUMINT && e2->common_expr.type.type_code == RAVI_TNUMINT)
			set_typecode(&node->binary_expr.type, RAVI_TNUMINT);
		else if ((e1->common_expr.type.type_code == RAVI_TNUMINT &&
			  e2->common_expr.type.type_code == RAVI_TNUMFLT) ||
			 (e1->common_expr.type.type_code == RAVI_TNUMFLT &&
			  e2->common_expr.type.type_code == RAVI_TNUMINT))
			set_typecode(&node->binary_expr.type, RAVI_TNUMFLT);
		break;
	default:
		set_typecode(&node->binary_expr.type, RAVI_TANY);
		break;
	}
}

static bool is_unindexable_type(struct var_type *type)
{
	switch (type->type_code) {
	case RAVI_TNUMFLT:
	case RAVI_TNUMINT:
	case RAVI_TBOOLEAN:
	case RAVI_TNIL:
		return true;
	default:
		return false;
	}
}

/*
 * Suffixed expression examples:
 * f()[1]
 * x[1][2]
 * x.y[1]
 */
static void typecheck_suffixedexpr(struct compiler_state *container, struct ast_node *function, struct ast_node *node)
{
	typecheck_ast_node(container, function, node->suffixed_expr.primary_expr);
	struct ast_node *prev_node = node->suffixed_expr.primary_expr;
	struct ast_node *this_node;
	FOR_EACH_PTR(node->suffixed_expr.suffix_list, this_node)
	{
		typecheck_ast_node(container, function, this_node);
		if (this_node->type == AST_Y_INDEX_EXPR) {
			if (prev_node->common_expr.type.type_code == RAVI_TARRAYFLT) {
				if (this_node->index_expr.expr->common_expr.type.type_code == RAVI_TNUMINT) {
					set_typecode(&this_node->index_expr.type, RAVI_TNUMFLT);
				} else {
					handle_error(container, "invalid type in index");
				}
			} else if (prev_node->common_expr.type.type_code == RAVI_TARRAYINT) {
				if (this_node->index_expr.expr->common_expr.type.type_code == RAVI_TNUMINT) {
					set_typecode(&this_node->index_expr.type, RAVI_TNUMINT);
				} else {
					handle_error(container, "invalid type in index");
				}
			} else if (is_unindexable_type(&prev_node->common_expr.type)) {
				handle_error(container, "invalid type in index");
			}
		}
		prev_node = this_node;
	}
	END_FOR_EACH_PTR(node);
	copy_type(&node->suffixed_expr.type, &prev_node->common_expr.type);
}

static void insert_cast(struct compiler_state *container, struct ast_node *expr, UnOpr opcode, ravitype_t target_type)
{
	/* convert the node to @integer node, the original content of node goes into the subexpr */
	struct ast_node *copy_expr = raviX_allocator_allocate(&container->ast_node_allocator, 0);
	*copy_expr = *expr;
	expr->type = AST_UNARY_EXPR;
	expr->unary_expr.expr = copy_expr;
	expr->unary_expr.unary_op = opcode;
	set_typecode(&expr->unary_expr.type, target_type);
}

static void typecheck_var_assignment(struct compiler_state *container, struct var_type *var_type, struct ast_node *expr,
				     const char *var_name)
{
	if (var_type->type_code == RAVI_TANY)
		// Any value can be assigned to type ANY
		return;

	struct var_type *expr_type = &expr->common_expr.type;

	if (var_type->type_code == RAVI_TNUMINT) {
		/* if the expr is of type number or # operator then insert @integer operator */
		if (expr_type->type_code == RAVI_TNUMFLT ||
		    (expr->type == AST_UNARY_EXPR && expr->unary_expr.unary_op == UNOPR_LEN)) {
			insert_cast(container, expr, UNOPR_TO_INTEGER, RAVI_TNUMINT);
		} else if (expr_type->type_code != RAVI_TNUMINT) {
			fprintf(stderr, "Assignment to local symbol %s is not type compatible\n", var_name);
		}
		return;
	}
	if (var_type->type_code == RAVI_TNUMFLT) {
		if (expr_type->type_code == RAVI_TNUMINT) {
			/* cast to number */
			insert_cast(container, expr, UNOPR_TO_NUMBER, RAVI_TNUMFLT);
		} else if (expr_type->type_code != RAVI_TNUMFLT) {
			fprintf(stderr, "Assignment to local symbol %s is not type compatible\n", var_name);
		}
		return;
	}
	// all other types must strictly match
	if (!is_type_same(var_type, expr_type)) { // We should probably check type convert-ability here
		fprintf(stderr, "Assignment to local symbol %s is not type compatible\n", var_name);
	}
}

static void typecheck_local_statement(struct compiler_state *container, struct ast_node *function,
				      struct ast_node *node)
{
	// The local vars should already be annotated
	// We need to typecheck the expressions to the right of =
	// Then we need to ensure that the assignments are valid
	// We can perhaps insert type assertions where we have a mismatch?

	typecheck_ast_list(container, function, node->local_stmt.expr_list);

	struct lua_symbol *var;
	struct ast_node *expr;
	PREPARE_PTR_LIST(node->local_stmt.var_list, var);
	PREPARE_PTR_LIST(node->local_stmt.expr_list, expr);

	for (;;) {
		if (!var || !expr)
			break;

		struct var_type *var_type = &var->value_type;
		const char *var_name = var->var.var_name;

		typecheck_var_assignment(container, var_type, expr, var_name);

		NEXT_PTR_LIST(var);
		NEXT_PTR_LIST(expr);
	}
}

static void typecheck_expr_statement(struct compiler_state *container, struct ast_node *function, struct ast_node *node)
{
	if (node->expression_stmt.var_expr_list)
		typecheck_ast_list(container, function, node->expression_stmt.var_expr_list);
	typecheck_ast_list(container, function, node->expression_stmt.expr_list);

	if (!node->expression_stmt.var_expr_list)
		return;

	struct ast_node *var;
	struct ast_node *expr;
	PREPARE_PTR_LIST(node->expression_stmt.var_expr_list, var);
	PREPARE_PTR_LIST(node->local_stmt.expr_list, expr);

	for (;;) {
		if (!var || !expr)
			break;

		struct var_type *var_type = &var->common_expr.type;
		const char *var_name = ""; // FIXME how do we get this?

		typecheck_var_assignment(container, var_type, expr, var_name);

		NEXT_PTR_LIST(var);
		NEXT_PTR_LIST(expr);
	}
}

static void typecheck_for_in_statment(struct compiler_state *container, struct ast_node *function,
				      struct ast_node *node)
{
	typecheck_ast_list(container, function, node->for_stmt.expr_list);
	typecheck_ast_list(container, function, node->for_stmt.for_statement_list);
}

static void typecheck_for_num_statment(struct compiler_state *container, struct ast_node *function,
				       struct ast_node *node)
{
	typecheck_ast_list(container, function, node->for_stmt.expr_list);
	struct ast_node *expr;
	enum { I = 1, F = 2, A = 4 }; /* bits representing integer, number, any */
	int index_type = 0;
	FOR_EACH_PTR(node->for_stmt.expr_list, expr)
	{
		switch (expr->common_expr.type.type_code) {
		case RAVI_TNUMFLT:
			index_type |= F;
			break;
		case RAVI_TNUMINT:
			index_type |= I;
			break;
		default:
			index_type |= A;
			break;
		}
		if ((index_type & A) != 0)
			break;
	}
	END_FOR_EACH_PTR(expr);
	if ((index_type & A) == 0) { /* not any */
		/* for I+F we use F */
		ravitype_t symbol_type = index_type == I ? RAVI_TNUMINT : RAVI_TNUMFLT;
		struct lua_symbol_list *symbols = node->for_stmt.symbols;
		struct lua_symbol *sym;
		/* actually there will be only index variable */
		FOR_EACH_PTR(symbols, sym)
		{
			if (sym->symbol_type == SYM_LOCAL) {
				set_typecode(&sym->value_type, symbol_type);
			} else {
				assert(0); /* cannot happen */
			}
		}
		END_FOR_EACH_PTR(sym);
	}
	typecheck_ast_list(container, function, node->for_stmt.for_statement_list);
}

static void typecheck_if_statement(struct compiler_state *container, struct ast_node *function, struct ast_node *node)
{
	struct ast_node *test_then_block;
	FOR_EACH_PTR(node->if_stmt.if_condition_list, test_then_block)
	{
		typecheck_ast_node(container, function, test_then_block->test_then_block.condition);
		typecheck_ast_list(container, function, test_then_block->test_then_block.test_then_statement_list);
	}
	END_FOR_EACH_PTR(node);
	if (node->if_stmt.else_statement_list) {
		typecheck_ast_list(container, function, node->if_stmt.else_statement_list);
	}
}

static void typecheck_while_or_repeat_statement(struct compiler_state *container, struct ast_node *function,
						struct ast_node *node)
{
	typecheck_ast_node(container, function, node->while_or_repeat_stmt.condition);
	if (node->while_or_repeat_stmt.loop_statement_list) {
		typecheck_ast_list(container, function, node->while_or_repeat_stmt.loop_statement_list);
	}
}

/* Type checker - WIP  */
static void typecheck_ast_node(struct compiler_state *container, struct ast_node *function, struct ast_node *node)
{
	switch (node->type) {
	case AST_FUNCTION_EXPR: {
		/* args need type assertions but those have no ast - i.e. code gen should do it */
		typecheck_ast_list(container, function, node->function_expr.function_statement_list);
		break;
	}
	case AST_NONE: {
		break;
	}
	case AST_RETURN_STMT: {
		typecheck_ast_list(container, function, node->return_stmt.expr_list);
		break;
	}
	case AST_LOCAL_STMT: {
		typecheck_local_statement(container, function, node);
		break;
	}
	case AST_FUNCTION_STMT: {
		typecheck_ast_node(container, function, node->function_stmt.function_expr);
		break;
	}
	case AST_LABEL_STMT: {
		break;
	}
	case AST_GOTO_STMT: {
		break;
	}
	case AST_DO_STMT: {
		break;
	}
	case AST_EXPR_STMT: {
		typecheck_expr_statement(container, function, node);
		break;
	}
	case AST_IF_STMT: {
		typecheck_if_statement(container, function, node);
		break;
	}
	case AST_WHILE_STMT:
	case AST_REPEAT_STMT: {
		typecheck_while_or_repeat_statement(container, function, node);
		break;
	}
	case AST_FORIN_STMT: {
		typecheck_for_in_statment(container, function, node);
		break;
	}
	case AST_FORNUM_STMT: {
		typecheck_for_num_statment(container, function, node);
		break;
	}
	case AST_SUFFIXED_EXPR: {
		typecheck_suffixedexpr(container, function, node);
		break;
	}
	case AST_FUNCTION_CALL_EXPR: {
		if (node->function_call_expr.method_name) {
		} else {
		}
		typecheck_ast_list(container, function, node->function_call_expr.arg_list);
		break;
	}
	case AST_SYMBOL_EXPR: {
		/* symbol type should have been set when symbol was created */
		copy_type(&node->symbol_expr.type, &node->symbol_expr.var->value_type);
		break;
	}
	case AST_BINARY_EXPR: {
		typecheck_binary_operator(container, function, node);
		break;
	}
	case AST_UNARY_EXPR: {
		typecheck_unary_operator(container, function, node);
		break;
	}
	case AST_LITERAL_EXPR: {
		/* type set during parsing */
		break;
	}
	case AST_FIELD_SELECTOR_EXPR: {
		typecheck_ast_node(container, function, node->index_expr.expr);
		break;
	}
	case AST_Y_INDEX_EXPR: {
		typecheck_ast_node(container, function, node->index_expr.expr);
		break;
	}
	case AST_INDEXED_ASSIGN_EXPR: {
		if (node->indexed_assign_expr.key_expr) {
			typecheck_ast_node(container, function, node->indexed_assign_expr.key_expr);
		}
		typecheck_ast_node(container, function, node->indexed_assign_expr.value_expr);
		copy_type(&node->indexed_assign_expr.type, &node->indexed_assign_expr.value_expr->common_expr.type);
		break;
	}
	case AST_TABLE_EXPR: {
		typecheck_ast_list(container, function, node->table_expr.expr_list);
		break;
	}
	default:
		assert(0);
	}
}

/* Type checker - WIP  */
static void typecheck_function(struct compiler_state *container, struct ast_node *func)
{
	typecheck_ast_list(container, func, func->function_expr.function_statement_list);
}

/* Type checker - WIP  */
int raviX_ast_typecheck(struct compiler_state *container)
{
	struct ast_node *main_function = container->main_function;
	raviX_buffer_rewindpos(&container->error_message);
	int rc = setjmp(container->env);
	if (rc == 0) {
		typecheck_function(container, main_function);
	}
	return rc;
}
