/******************************************************************************
 * Copyright (C) 2018-2021 Dibyendu Majumdar
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/
/* Portions Copyright (C) 1994-2019 Lua.org, PUC-Rio.*/

#include <parser.h>

#include <math.h>
#include <string.h>

static void process_expression_list(CompilerState *container, AstNodeList *node);
static void process_statement_list(CompilerState *container, AstNodeList *node);
static void process_statement(CompilerState *container, AstNode *node);

static void process_expression(CompilerState *container, AstNode *node)
{
	switch (node->type) {
	case EXPR_FUNCTION:
		process_statement_list(container, node->function_expr.function_statement_list);
		break;
	case EXPR_SUFFIXED:
		process_expression(container, node->suffixed_expr.primary_expr);
		if (node->suffixed_expr.suffix_list) {
			process_expression_list(container, node->suffixed_expr.suffix_list);
		}
		break;
	case EXPR_FUNCTION_CALL:
		process_expression_list(container, node->function_call_expr.arg_list);
		break;
	case EXPR_SYMBOL:
		break;
	case EXPR_BINARY:
		process_expression(container, node->binary_expr.expr_left);
		process_expression(container, node->binary_expr.expr_right);
		break;
	case EXPR_CONCAT:
		process_expression_list(container, node->string_concatenation_expr.expr_list);
		break;
	case EXPR_UNARY:
		process_expression(container, node->unary_expr.expr);
		break;
	case EXPR_LITERAL:
		break;
	case EXPR_FIELD_SELECTOR:
		process_expression(container, node->index_expr.expr);
		break;
	case EXPR_Y_INDEX:
		process_expression(container, node->index_expr.expr);
		break;
	case EXPR_TABLE_ELEMENT_ASSIGN:
		if (node->table_elem_assign_expr.key_expr) {
			process_expression(container, node->table_elem_assign_expr.key_expr);
		}
		process_expression(container, node->table_elem_assign_expr.value_expr);
		break;
	case EXPR_TABLE_LITERAL:
		process_expression_list(container, node->table_expr.expr_list);
		break;
	default:
		assert(0);
		break;
	}
}

static void process_expression_list(CompilerState *container, AstNodeList *list)
{
	AstNode *node;
	FOR_EACH_PTR(list, AstNode, node) { process_expression(container, node); }
	END_FOR_EACH_PTR(node);
}

static void process_statement_list(CompilerState *container, AstNodeList *list)
{
	AstNode *node;
	FOR_EACH_PTR(list, AstNode, node) { process_statement(container, node); }
	END_FOR_EACH_PTR(node);
}

static void add_ast_node(CompilerState *container, AstNodeList **list, AstNode *node)
{
	raviX_ptrlist_add((PtrList **)list, node, &container->ptrlist_allocator);
}

static AstNode *allocate_expr_ast_node(CompilerState *container, enum AstNodeType type)
{
	AstNode *node = raviX_allocate_ast_node_at_line(container, type, 0);
	node->common_expr.truncate_results = 0;
	set_typecode(&node->common_expr.type, RAVI_TANY);
	return node;
}

static AstNode *true_expression(CompilerState *container)
{
	AstNode *expr = allocate_expr_ast_node(container, EXPR_LITERAL);
	set_type(&expr->literal_expr.type, RAVI_TBOOLEAN);
	expr->literal_expr.u.i = 1; /* initialize */
	return expr;
}

static AstNode *make_symbol_expr(CompilerState *container, LuaSymbol *symbol)
{
	AstNode *symbol_expr = allocate_expr_ast_node(container, EXPR_SYMBOL);
	symbol_expr->symbol_expr.type = symbol->variable.value_type;
	symbol_expr->symbol_expr.var = symbol;
	return symbol_expr;
}

static AstNode *for_expr(CompilerState *container, LuaSymbol *fsym, LuaSymbol *ssym, LuaSymbol *varsym)
{
	AstNode *suffixed_expr = allocate_expr_ast_node(container, EXPR_SUFFIXED);
	suffixed_expr->suffixed_expr.primary_expr = make_symbol_expr(container, fsym);
	set_type(&suffixed_expr->suffixed_expr.type, RAVI_TANY);
	suffixed_expr->suffixed_expr.suffix_list = NULL;

	AstNode *call_expr = allocate_expr_ast_node(container, EXPR_FUNCTION_CALL);
	call_expr->function_call_expr.method_name = NULL;
	call_expr->function_call_expr.arg_list = NULL;
	call_expr->function_call_expr.num_results = 1; /* By default we expect one arg */
	set_type(&call_expr->function_call_expr.type, RAVI_TANY);

	add_ast_node(container, &call_expr->function_call_expr.arg_list, make_symbol_expr(container, ssym));
	add_ast_node(container, &call_expr->function_call_expr.arg_list, make_symbol_expr(container, varsym));

	add_ast_node(container, &suffixed_expr->suffixed_expr.suffix_list, call_expr);

	return suffixed_expr;
}
static void lower_for_in_statement(CompilerState *container, AstNode *node)
{
	ForStatement *for_stmt = &node->for_stmt;
	//Scope *scope = for_stmt->for_scope;
	AstNode *function = for_stmt->for_scope->function;
	AstNode *do_stmt = raviX_allocate_ast_node_at_line(container, STMT_DO, node->line_number);
	do_stmt->do_stmt.do_statement_list = NULL;
	Scope *do_scope = raviX_allocate_scope(container, function, for_stmt->for_scope->parent);
	do_stmt->do_stmt.scope = do_scope;
	// we need three anon vars
	const char f[] = "(for_f)";
	const char s[] = "(for_s)";
	const char var[] = "(for_var)";

	AstNode *local_stmt = raviX_allocate_ast_node_at_line(container, STMT_LOCAL, node->line_number);
	local_stmt->local_stmt.var_list = NULL;
	local_stmt->local_stmt.expr_list = for_stmt->expr_list;

	LuaSymbol *fsym = raviX_new_local_symbol(container, do_scope, raviX_create_string(container, f, sizeof f-1), RAVI_TANY, NULL);
	LuaSymbol *ssym = raviX_new_local_symbol(container, do_scope, raviX_create_string(container, s, sizeof s-1), RAVI_TANY, NULL);
	LuaSymbol *varsym = raviX_new_local_symbol(container, do_scope, raviX_create_string(container, var, sizeof var-1), RAVI_TANY, NULL);

	raviX_add_symbol(container, &local_stmt->local_stmt.var_list, fsym);
	raviX_add_symbol(container, &do_scope->symbol_list, fsym);
	raviX_add_symbol(container, &local_stmt->local_stmt.var_list, ssym);
	raviX_add_symbol(container, &do_scope->symbol_list, ssym);
	raviX_add_symbol(container, &local_stmt->local_stmt.var_list, varsym);
	raviX_add_symbol(container, &do_scope->symbol_list, varsym);

	add_ast_node(container, &do_stmt->do_stmt.do_statement_list, local_stmt);

	AstNode *while_stmt = raviX_allocate_ast_node_at_line(container, STMT_WHILE, node->line_number);
	Scope *while_scope = for_stmt->for_scope; // Original scope
	while_scope->parent = do_scope; // change the original scope's parent
	while_stmt->while_or_repeat_stmt.loop_scope = while_scope;
	while_stmt->while_or_repeat_stmt.loop_statement_list = NULL;
	while_stmt->while_or_repeat_stmt.condition = true_expression(container);
	add_ast_node(container, &do_stmt->do_stmt.do_statement_list, while_stmt);

	AstNode *local_stmt2 = raviX_allocate_ast_node_at_line(container, STMT_LOCAL, node->line_number);
	local_stmt2->local_stmt.var_list = for_stmt->symbols;
	local_stmt2->local_stmt.expr_list = NULL;
	AstNode *call_expr = for_expr(container, fsym, ssym, varsym);
	add_ast_node(container, &local_stmt2->local_stmt.expr_list, call_expr);
	add_ast_node(container, &while_stmt->while_or_repeat_stmt.loop_statement_list, local_stmt2);

	// TODO
	// if var break

	AstNode *n2;
	FOR_EACH_PTR(for_stmt->for_statement_list, AstNode, n2) {
		add_ast_node(container, &while_stmt->while_or_repeat_stmt.loop_statement_list, n2);
	}
	END_FOR_EACH_PTR(n2)

	*node = *do_stmt;
}

static void process_statement(CompilerState *container, AstNode *node)
{
	switch (node->type) {
	case AST_NONE:
		break;
	case STMT_RETURN:
		process_expression_list(container, node->return_stmt.expr_list);
		break;
	case STMT_LOCAL:
		process_expression_list(container, node->local_stmt.expr_list);
		break;
	case STMT_FUNCTION:
		process_expression(container, node->function_stmt.function_expr);
		break;
	case STMT_LABEL:
	case STMT_GOTO:
		break;
	case STMT_DO:
		process_statement_list(container, node->do_stmt.do_statement_list);
		break;
	case STMT_EXPR:
		if (node->expression_stmt.var_expr_list) {
			process_expression_list(container, node->expression_stmt.var_expr_list);
		}
		process_expression_list(container, node->expression_stmt.expr_list);
		break;
	case STMT_IF: {
		AstNode *test_then_block;
		FOR_EACH_PTR(node->if_stmt.if_condition_list, AstNode, test_then_block)
		{
			process_expression(container, test_then_block->test_then_block.condition);
			process_statement_list(container, test_then_block->test_then_block.test_then_statement_list);
		}
		END_FOR_EACH_PTR(node);
		if (node->if_stmt.else_block) {
			process_statement_list(container, node->if_stmt.else_statement_list);
		}
		break;
	}
	case STMT_WHILE:
		process_expression(container, node->while_or_repeat_stmt.condition);
		process_statement_list(container, node->while_or_repeat_stmt.loop_statement_list);
		break;
	case STMT_REPEAT:
		process_statement_list(container, node->while_or_repeat_stmt.loop_statement_list);
		process_expression(container, node->while_or_repeat_stmt.condition);
		break;
	case STMT_FOR_NUM:
		process_expression_list(container, node->for_stmt.expr_list);
		process_statement_list(container, node->for_stmt.for_statement_list);
		break;
	case STMT_FOR_IN:
		lower_for_in_statement(container, node);
		break;
	default:
		fprintf(stderr, "AST = %d\n", node->type);
		assert(0);
		break;
	}
}

int raviX_ast_lower(CompilerState *container)
{
	int rc = setjmp(container->env);
	if (rc == 0) {
		process_expression(container, container->main_function);
	} else {
		// dump it?
	}
	return rc;
}
