/**
 * This is a sample AST walker that illustrates how to use the
 * AST api. Note that this sample doesn't actually do anything.
 *
 * This is meant to be a starting point for writing a useful AST
 * walker.
 */

#include "ravi_compiler.h"
#include "tcommon.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void walk_statement(void *data, const struct statement *statement);
static void walk_expression(void *data, const struct expression *expression);
static void walk_function(void *data, const struct function_expression *function);

struct ast_state {
	int dummy;
};

static void walk_variable_symbol(void *data, const struct lua_variable_symbol *symbol)
{
	const struct string_object *name = raviX_variable_symbol_name(symbol);
	assert(name != NULL);
	const struct block_scope *scope = raviX_variable_symbol_scope(symbol);
	// If global scope will be NULL
	(void)scope;
	const struct var_type *type = raviX_variable_symbol_type(symbol);
	assert(type != NULL);
}

static void walk_symbol(void *data, const struct lua_symbol *symbol)
{
	enum symbol_type type = raviX_symbol_type(symbol);
	switch (type) {
	case SYM_LABEL: {
		const struct lua_label_symbol *label = raviX_symbol_label(symbol);
		(void)label;
		break;
	}
	case SYM_GLOBAL:
	case SYM_LOCAL: {
		const struct lua_variable_symbol *variable = raviX_symbol_variable(symbol);
		walk_variable_symbol(data, variable);
		break;
	}
	case SYM_UPVALUE: {
		const struct lua_upvalue_symbol *upvalue = raviX_symbol_upvalue(symbol);
		(void)upvalue;
		break;
	}
	default:
		assert(false);
	}
}

static void walk_scope(void *data, const struct block_scope *scope)
{
	raviX_scope_foreach_symbol(scope, data, walk_symbol);
}

static void walk_index_expression(void *data, const struct index_expression *index_expression)
{
	const struct var_type *type = raviX_index_expression_type(index_expression);
	(void)type;
	walk_expression(data, raviX_index_expression_expression(index_expression));
}

static void walk_symbol_expression(void *data, const struct symbol_expression *symbol_expression)
{
	const struct var_type *type = raviX_symbol_expression_type(symbol_expression);
	(void)type;
	walk_symbol(data, raviX_symbol_expression_symbol(symbol_expression));
}

static void
walk_table_assignment_expression(void *data,
				 const struct table_element_assignment_expression *table_element_assignment_expression)
{
	const struct var_type *type =
	    raviX_table_element_assignment_expression_type(table_element_assignment_expression);
	(void)type;
	const struct expression *key_expression =
	    raviX_table_element_assignment_expression_key(table_element_assignment_expression);
	if (key_expression) {
		walk_expression(data,
				raviX_table_element_assignment_expression_key(table_element_assignment_expression));
	}
	walk_expression(data, raviX_table_element_assignment_expression_value(table_element_assignment_expression));
}

static void walk_expression(void *data, const struct expression *expression)
{
	switch (raviX_expression_type(expression)) {
	case EXPR_SYMBOL:
		walk_symbol_expression(data, raviX_symbol_expression(expression));
		break;
	case EXPR_FUNCTION_CALL: {
		const struct function_call_expression *function_call_expression =
		    raviX_function_call_expression(expression);
		const struct var_type *type = raviX_function_call_expression_type(function_call_expression);
		(void)type;
		const struct string_object *method_name =
		    raviX_function_call_expression_method_name(function_call_expression);
		(void)method_name;
		raviX_function_call_expression_foreach_argument(function_call_expression, data, walk_expression);
		break;
	}
	case EXPR_SUFFIXED: {
		const struct suffixed_expression *suffixed_expression = raviX_suffixed_expression(expression);
		const struct var_type *type = raviX_suffixed_expression_type(suffixed_expression);
		(void)type;
		walk_expression(data, raviX_suffixed_expression_primary(suffixed_expression));
		raviX_suffixed_expression_foreach_suffix(suffixed_expression, data, walk_expression);
		break;
	}
	case EXPR_TABLE_LITERAL: {
		const struct table_literal_expression *table_literal_expression =
		    raviX_table_literal_expression(expression);
		const struct var_type *type = raviX_table_literal_expression_type(table_literal_expression);
		(void)type;
		raviX_table_literal_expression_foreach_element(table_literal_expression, data,
							       walk_table_assignment_expression);
		break;
	}
	case EXPR_TABLE_ELEMENT_ASSIGN: {
		walk_table_assignment_expression(data, raviX_table_element_assignment_expression(expression));
		break;
	}
	case EXPR_FUNCTION: {
		walk_function(data, raviX_function_expression(expression));
		break;
	}
	case EXPR_BINARY: {
		const struct binary_expression *binary_expression = raviX_binary_expression(expression);
		const struct var_type *type = raviX_binary_expression_type(binary_expression);
		(void)type;
		enum BinaryOperatorType binary_operator = raviX_binary_expression_operator(binary_expression);
		(void)binary_operator;
		walk_expression(data, raviX_binary_expression_left_expression(binary_expression));
		walk_expression(data, raviX_binary_expression_right_expression(binary_expression));
		break;
	}
	case EXPR_UNARY: {
		const struct unary_expression *unary_expression = raviX_unary_expression(expression);
		const struct var_type *type = raviX_unary_expression_type(unary_expression);
		(void)type;
		enum UnaryOperatorType unary_operator = raviX_unary_expression_operator(unary_expression);
		(void)unary_operator;
		walk_expression(data, raviX_unary_expression_expression(unary_expression));
		break;
	}
	case EXPR_LITERAL: {
		const struct literal_expression *literal_expression = raviX_literal_expression(expression);
		const struct var_type *type = raviX_literal_expression_type(literal_expression);
		(void)type;
		const SemInfo *sem_info = raviX_literal_expression_literal(literal_expression);
		(void)sem_info;
		break;
	}
	case EXPR_FIELD_SELECTOR: {
		walk_index_expression(data, raviX_index_expression(expression));
		break;
	}
	default: {
		// cannot happen
		assert(false);
	}
	}
}

static void walk_test_then_statement(void *data, const struct test_then_statement *test_then_statement)
{
	walk_expression(data, raviX_test_then_statement_condition(test_then_statement));
	walk_scope(data, raviX_test_then_statement_scope(test_then_statement));
	raviX_test_then_statement_foreach_statement(test_then_statement, data, walk_statement);
}

static void walk_statement(void *data, const struct statement *statement)
{
	switch (raviX_statement_type(statement)) {
	case STMT_DO: {
		const struct do_statement *do_statement = raviX_do_statement(statement);
		walk_scope(data, raviX_do_statement_scope(do_statement));
		raviX_do_statement_foreach_statement(do_statement, data, walk_statement);
		break;
	}
	case STMT_EXPR: {
		const struct expression_statement *expression_statement = raviX_expression_statement(statement);
		raviX_expression_statement_foreach_lhs_expression(expression_statement, data, walk_expression);
		raviX_expression_statement_foreach_rhs_expression(expression_statement, data, walk_expression);
		break;
	}
	case STMT_FOR_NUM:
	case STMT_FOR_IN: {
		const struct for_statement *for_statement = raviX_for_statement(statement);
		walk_scope(data, raviX_for_statement_scope(for_statement));
		raviX_for_statement_foreach_symbol(for_statement, data, walk_variable_symbol);
		raviX_for_statement_foreach_expression(for_statement, data, walk_expression);
		walk_scope(data, raviX_for_statement_body_scope(for_statement));
		raviX_for_statement_body_foreach_statement(for_statement, data, walk_statement);
		break;
	}
	case STMT_FUNCTION: {
		const struct function_statement *function_statement = raviX_function_statement(statement);
		const struct symbol_expression *name_expression = raviX_function_statement_name(function_statement);
		walk_symbol_expression(data, name_expression);
		if (raviX_function_statement_has_selectors(function_statement)) {
			raviX_function_statement_foreach_selector(function_statement, data, walk_index_expression);
		}
		if (raviX_function_statement_is_method(function_statement)) {
			walk_index_expression(data, raviX_function_statement_method_name(function_statement));
		}
		walk_function(data, raviX_function_ast(function_statement));
		break;
	}
	case STMT_GOTO: {
		const struct goto_statement *goto_statement = raviX_goto_statement(statement);
		if (!raviX_goto_statement_is_break(goto_statement)) {
			const struct string_object *goto_label = raviX_goto_statement_label_name(goto_statement);
			(void)goto_label;
		}
		walk_scope(data, raviX_goto_statement_scope(goto_statement));
		break;
	}
	case STMT_IF: {
		const struct if_statement *if_statement = raviX_if_statement(statement);
		raviX_if_statement_foreach_test_then_statement(if_statement, data, walk_test_then_statement);
		walk_scope(data, raviX_if_then_statement_else_scope(if_statement));
		raviX_if_statement_foreach_else_statement(if_statement, data, walk_statement);
		break;
	}
	case STMT_LABEL: {
		const struct label_statement *label_statement = raviX_label_statement(statement);
		const struct string_object *label_name = raviX_label_statement_label_name(label_statement);
		(void)label_name;
		walk_scope(data, raviX_label_statement_label_scope(label_statement));
		break;
	}
	case STMT_LOCAL: {
		const struct local_statement *local_statement = raviX_local_statement(statement);
		raviX_local_statement_foreach_symbol(local_statement, data, walk_variable_symbol);
		raviX_local_statement_foreach_expression(local_statement, data, walk_expression);
		break;
	}
	case STMT_REPEAT:
	case STMT_WHILE: {
		const struct while_or_repeat_statement *while_or_repeat_statement =
		    raviX_while_or_repeat_statement(statement);
		walk_expression(data, raviX_while_or_repeat_statement_condition(while_or_repeat_statement));
		walk_scope(data, raviX_while_or_repeat_statement_scope(while_or_repeat_statement));
		raviX_while_or_repeat_statement_foreach_statement(while_or_repeat_statement, data, walk_statement);
		break;
	}
	case STMT_RETURN: {
		const struct return_statement *return_statement = raviX_return_statement(statement);
		raviX_return_statement_foreach_expression(return_statement, data, walk_expression);
		break;
	}
	default: {
		// Cannot happen
		assert(false);
	}
	}
}

static void walk_function(void *data, const struct function_expression *function)
{
	struct ast_state *state = (struct ast_state *)data;
	if (raviX_function_is_vararg(function)) {
	}
	if (raviX_function_is_method(function)) {
	}
	const struct block_scope *scope = raviX_function_scope(function);
	assert(function == raviX_scope_owning_function(scope));
	const struct block_scope *parent = raviX_scope_parent_scope(scope);
	(void)parent;
	walk_scope(state, scope);
}

static void walk_ast(struct compiler_state *container)
{
	// Dummy struct - in a useful implementation this is where you
	// would maintain state
	struct ast_state state = {0};
	// First lets get the main function from the parse tree
	const struct function_expression *main_function = raviX_ast_get_main_function(container);
	walk_function(&state, main_function);
	// Now walk all the child functions
	raviX_function_foreach_child(main_function, &state, walk_function);
}

int main(int argc, const char *argv[])
{
	struct arguments args;
	parse_arguments(&args, argc, argv);
	const char *code = NULL;
	if (args.code) {
		code = args.code;
	}
	int rc = 0;
	if (!code) {
		fprintf(stderr, "No code to process\n");
		rc = 1;
		goto L_exit;
	}
	struct compiler_state *container = raviX_init_compiler();
	rc = raviX_parse(container, code, strlen(code), "input");
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}

	walk_ast(container);

L_exit:
	raviX_destroy_compiler(container);
	destroy_arguments(&args);

	return rc;
}