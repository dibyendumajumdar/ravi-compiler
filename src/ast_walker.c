#include <ravi_compiler.h>

#include <parser.h>

const struct function_expression *raviX_ast_get_main_function(const CompilerState *compiler_state)
{
	return &compiler_state->main_function->function_expr;
}
const struct var_type *raviX_function_type(const struct function_expression *function_expression)
{
	return &function_expression->type;
}
bool raviX_function_is_vararg(const struct function_expression *function_expression)
{
	return function_expression->is_vararg;
}
bool raviX_function_is_method(const struct function_expression *function_expression)
{
	return function_expression->is_method;
}
const struct function_expression *raviX_function_parent(const struct function_expression *function_expression)
{
	if (function_expression->parent_function == NULL)
		return NULL;
	else
		return &function_expression->parent_function->function_expr;
}
void raviX_function_foreach_child(const struct function_expression *function_expression, void *userdata,
				  void (*callback)(void *userdata,
						   const struct function_expression *function_expression))
{
	struct ast_node *node;
	FOR_EACH_PTR(function_expression->child_functions, node) { callback(userdata, &node->function_expr); }
	END_FOR_EACH_PTR(node)
}
const struct block_scope *raviX_function_scope(const struct function_expression *function_expression)
{
	return function_expression->main_block;
}
void raviX_function_foreach_statement(const struct function_expression *function_expression, void *userdata,
				      void (*callback)(void *userdata, const struct statement *statement))
{
	struct ast_node *node;
	FOR_EACH_PTR(function_expression->function_statement_list, node)
	{
		assert(node->type <= STMT_EXPR);
		callback(userdata, (struct statement *)node);
	}
	END_FOR_EACH_PTR(node)
}
enum ast_node_type raviX_statement_type(const struct statement *statement) { return statement->type; }
void raviX_function_foreach_argument(const struct function_expression *function_expression, void *userdata,
				     void (*callback)(void *userdata, const struct lua_variable_symbol *symbol))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(function_expression->args, symbol) { callback(userdata, &symbol->variable); }
	END_FOR_EACH_PTR(symbol)
}
void raviX_function_foreach_local(const struct function_expression *function_expression, void *userdata,
				  void (*callback)(void *userdata, const struct lua_variable_symbol *lua_local_symbol))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(function_expression->locals, symbol) { callback(userdata, &symbol->variable); }
	END_FOR_EACH_PTR(symbol)
}
void raviX_function_foreach_upvalue(const struct function_expression *function_expression, void *userdata,
				    void (*callback)(void *userdata, const struct lua_upvalue_symbol *symbol))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(function_expression->upvalues, symbol) { callback(userdata, &symbol->upvalue); }
	END_FOR_EACH_PTR(symbol)
}

const StringObject *raviX_variable_symbol_name(const struct lua_variable_symbol *lua_local_symbol)
{
	return lua_local_symbol->var_name;
}

const struct var_type *raviX_variable_symbol_type(const struct lua_variable_symbol *lua_local_symbol)
{
	return &lua_local_symbol->value_type;
}

const struct block_scope *raviX_variable_symbol_scope(const struct lua_variable_symbol *lua_local_symbol)
{
	return lua_local_symbol->block;
}

#define n(v) ((struct ast_node *)v)
const struct return_statement *raviX_return_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_RETURN);
	return &n(stmt)->return_stmt;
}
const struct label_statement *raviX_label_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_LABEL);
	return &n(stmt)->label_stmt;
}
const struct goto_statement *raviX_goto_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_GOTO);
	return &n(stmt)->goto_stmt;
}
const struct local_statement *raviX_local_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_LOCAL);
	return &n(stmt)->local_stmt;
}
const struct expression_statement *raviX_expression_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_EXPR);
	return &n(stmt)->expression_stmt;
}
const struct function_statement *raviX_function_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_FUNCTION);
	return &n(stmt)->function_stmt;
}
const struct do_statement *raviX_do_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_DO);
	return &n(stmt)->do_stmt;
}
const struct test_then_statement *raviX_test_then_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_TEST_THEN);
	return &n(stmt)->test_then_block;
}
const struct if_statement *raviX_if_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_IF);
	return &n(stmt)->if_stmt;
}
const struct while_or_repeat_statement *raviX_while_or_repeat_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_WHILE || stmt->type == STMT_REPEAT);
	return &n(stmt)->while_or_repeat_stmt;
}
const struct for_statement *raviX_for_statement(const struct statement *stmt)
{
	assert(stmt->type == STMT_FOR_IN || stmt->type == STMT_FOR_NUM);
	return &n(stmt)->for_stmt;
}
enum ast_node_type raviX_expression_type(const struct expression *expression) { return expression->type; }
const struct literal_expression *raviX_literal_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_LITERAL);
	return &n(expr)->literal_expr;
}
const struct symbol_expression *raviX_symbol_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_SYMBOL);
	return &n(expr)->symbol_expr;
}
const struct index_expression *raviX_index_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_Y_INDEX || expr->type == EXPR_FIELD_SELECTOR);
	return &n(expr)->index_expr;
}
const struct unary_expression *raviX_unary_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_UNARY);
	return &n(expr)->unary_expr;
}
const struct binary_expression *raviX_binary_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_BINARY);
	return &n(expr)->binary_expr;
}
const struct function_expression *raviX_function_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_FUNCTION);
	return &n(expr)->function_expr;
}
const struct table_element_assignment_expression *
raviX_table_element_assignment_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_TABLE_ELEMENT_ASSIGN);
	return &n(expr)->table_elem_assign_expr;
}
const struct table_literal_expression *raviX_table_literal_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_TABLE_LITERAL);
	return &n(expr)->table_expr;
}
const struct suffixed_expression *raviX_suffixed_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_SUFFIXED);
	return &n(expr)->suffixed_expr;
}
const struct function_call_expression *raviX_function_call_expression(const struct expression *expr)
{
	assert(expr->type == EXPR_FUNCTION_CALL);
	return &n(expr)->function_call_expr;
}
#undef n

void raviX_return_statement_foreach_expression(const struct return_statement *statement, void *userdata,
					       void (*callback)(void *, const struct expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->expr_list, node)
	{
		assert(node->type >= EXPR_LITERAL && node->type <= EXPR_FUNCTION_CALL);
		callback(userdata, (struct expression *)node);
	}
	END_FOR_EACH_PTR(node)
}

const StringObject *raviX_label_statement_label_name(const struct label_statement *statement)
{
	return statement->symbol->label.label_name;
}
const struct block_scope *raviX_label_statement_label_scope(const struct label_statement *statement)
{
	return statement->symbol->label.block;
}

const StringObject *raviX_goto_statement_label_name(const struct goto_statement *statement)
{
	return statement->name;
}
const struct block_scope *raviX_goto_statement_scope(const struct goto_statement *statement)
{
	return statement->goto_scope;
}
bool raviX_goto_statement_is_break(const struct goto_statement *statement) { return statement->is_break; }

void raviX_local_statement_foreach_expression(const struct local_statement *statement, void *userdata,
					      void (*callback)(void *, const struct expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->expr_list, node)
	{
		assert(node->type >= EXPR_LITERAL && node->type <= EXPR_FUNCTION_CALL);
		callback(userdata, (struct expression *)node);
	}
	END_FOR_EACH_PTR(node)
}
void raviX_local_statement_foreach_symbol(const struct local_statement *statement, void *userdata,
					  void (*callback)(void *, const struct lua_variable_symbol *expr))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(statement->var_list, symbol)
	{
		assert(symbol->symbol_type == SYM_LOCAL);
		callback(userdata, &symbol->variable);
	}
	END_FOR_EACH_PTR(node)
}
void raviX_expression_statement_foreach_lhs_expression(const struct expression_statement *statement, void *userdata,
						       void (*callback)(void *, const struct expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->var_expr_list, node)
	{
		assert(node->type >= EXPR_LITERAL && node->type <= EXPR_FUNCTION_CALL);
		callback(userdata, (struct expression *)node);
	}
	END_FOR_EACH_PTR(node)
}
void raviX_expression_statement_foreach_rhs_expression(const struct expression_statement *statement, void *userdata,
						       void (*callback)(void *, const struct expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->expr_list, node)
	{
		assert(node->type >= EXPR_LITERAL && node->type <= EXPR_FUNCTION_CALL);
		callback(userdata, (struct expression *)node);
	}
	END_FOR_EACH_PTR(node)
}
const struct symbol_expression *raviX_function_statement_name(const struct function_statement *statement)
{
	assert(statement->name->type == EXPR_SYMBOL);
	return &statement->name->symbol_expr;
}
bool raviX_function_statement_is_method(const struct function_statement *statement)
{
	return statement->method_name != NULL;
}
const struct index_expression *raviX_function_statement_method_name(const struct function_statement *statement)
{
	assert(statement->method_name->type == EXPR_Y_INDEX || statement->method_name->type == EXPR_FIELD_SELECTOR);
	return &statement->method_name->index_expr;
}
bool raviX_function_statement_has_selectors(const struct function_statement *statement)
{
	return statement->selectors != NULL;
}
void raviX_function_statement_foreach_selector(const struct function_statement *statement, void *userdata,
					       void (*callback)(void *, const struct index_expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->selectors, node)
	{
		assert(node->type == EXPR_Y_INDEX || node->type == EXPR_FIELD_SELECTOR);
		callback(userdata, &node->index_expr);
	}
	END_FOR_EACH_PTR(node)
}
const struct function_expression *raviX_function_ast(const struct function_statement *statement)
{
	assert(statement->function_expr->type == EXPR_FUNCTION);
	return &statement->function_expr->function_expr;
}
const struct block_scope *raviX_do_statement_scope(const struct do_statement *statement) { return statement->scope; }
void raviX_do_statement_foreach_statement(const struct do_statement *statement, void *userdata,
					  void (*callback)(void *userdata, const struct statement *statement))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->do_statement_list, node)
	{
		assert(node->type <= STMT_EXPR);
		callback(userdata, (struct statement *)node);
	}
	END_FOR_EACH_PTR(node)
}
const struct block_scope *raviX_test_then_statement_scope(const struct test_then_statement *statement)
{
	return statement->test_then_scope;
}
void raviX_test_then_statement_foreach_statement(const struct test_then_statement *statement, void *userdata,
						 void (*callback)(void *userdata, const struct statement *statement))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->test_then_statement_list, node)
	{
		assert(node->type <= STMT_EXPR);
		callback(userdata, (struct statement *)node);
	}
	END_FOR_EACH_PTR(node)
}
const struct expression *raviX_test_then_statement_condition(const struct test_then_statement *statement)
{
	assert(statement->condition->type >= EXPR_LITERAL && statement->condition->type <= EXPR_FUNCTION_CALL);
	return (struct expression *)statement->condition;
}
void raviX_if_statement_foreach_test_then_statement(const struct if_statement *statement, void *userdata,
						    void (*callback)(void *, const struct test_then_statement *stmt))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->if_condition_list, node)
	{
		assert(node->type == STMT_TEST_THEN);
		callback(userdata, &node->test_then_block);
	}
	END_FOR_EACH_PTR(node)
}
const struct block_scope *raviX_if_then_statement_else_scope(const struct if_statement *statement)
{
	return statement->else_block;
}
void raviX_if_statement_foreach_else_statement(const struct if_statement *statement, void *userdata,
					       void (*callback)(void *userdata, const struct statement *statement))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->else_statement_list, node)
	{
		assert(node->type <= STMT_EXPR);
		callback(userdata, (struct statement *)node);
	}
	END_FOR_EACH_PTR(node)
}

const struct expression *raviX_while_or_repeat_statement_condition(const struct while_or_repeat_statement *statement)
{
	assert(statement->condition->type >= EXPR_LITERAL && statement->condition->type <= EXPR_FUNCTION_CALL);
	return (struct expression *)statement->condition;
}
const struct block_scope *raviX_while_or_repeat_statement_scope(const struct while_or_repeat_statement *statement)
{
	return statement->loop_scope;
}
void raviX_while_or_repeat_statement_foreach_statement(const struct while_or_repeat_statement *statement,
						       void *userdata,
						       void (*callback)(void *userdata,
									const struct statement *statement))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->loop_statement_list, node)
	{
		assert(node->type <= STMT_EXPR);
		callback(userdata, (struct statement *)node);
	}
	END_FOR_EACH_PTR(node)
}
const struct block_scope *raviX_for_statement_scope(const struct for_statement *statement)
{
	return statement->for_scope;
}
void raviX_for_statement_foreach_symbol(const struct for_statement *statement, void *userdata,
					void (*callback)(void *, const struct lua_variable_symbol *expr))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(statement->symbols, symbol)
	{
		assert(symbol->symbol_type == SYM_LOCAL);
		callback(userdata, &symbol->variable);
	}
	END_FOR_EACH_PTR(node)
}
void raviX_for_statement_foreach_expression(const struct for_statement *statement, void *userdata,
					    void (*callback)(void *, const struct expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->expr_list, node)
	{
		assert(node->type >= EXPR_LITERAL && node->type <= EXPR_FUNCTION_CALL);
		callback(userdata, (struct expression *)node);
	}
	END_FOR_EACH_PTR(node)
}
const struct block_scope *raviX_for_statement_body_scope(const struct for_statement *statement)
{
	return statement->for_body;
}
void raviX_for_statement_body_foreach_statement(const struct for_statement *statement, void *userdata,
						void (*callback)(void *userdata, const struct statement *statement))
{
	struct ast_node *node;
	FOR_EACH_PTR(statement->for_statement_list, node)
	{
		assert(node->type <= STMT_EXPR);
		callback(userdata, (struct statement *)node);
	}
	END_FOR_EACH_PTR(node)
}
const struct var_type *raviX_literal_expression_type(const struct literal_expression *expression)
{
	return &expression->type;
}
const SemInfo *raviX_literal_expression_literal(const struct literal_expression *expression) { return &expression->u; }
const struct var_type *raviX_symbol_expression_type(const struct symbol_expression *expression)
{
	return &expression->type;
}
const struct lua_symbol *raviX_symbol_expression_symbol(const struct symbol_expression *expression)
{
	return expression->var;
}
const struct var_type *raviX_index_expression_type(const struct index_expression *expression)
{
	return &expression->type;
}
const struct expression *raviX_index_expression_expression(const struct index_expression *expression)
{
	assert(expression->expr->type >= EXPR_LITERAL && expression->expr->type <= EXPR_FUNCTION_CALL);
	return (const struct expression *)expression->expr;
}
const struct var_type *raviX_unary_expression_type(const struct unary_expression *expression)
{
	return &expression->type;
}
const struct expression *raviX_unary_expression_expression(const struct unary_expression *expression)
{
	assert(expression->expr->type >= EXPR_LITERAL && expression->expr->type <= EXPR_FUNCTION_CALL);
	return (const struct expression *)expression->expr;
}
UnaryOperatorType raviX_unary_expression_operator(const struct unary_expression *expression)
{
	return expression->unary_op;
}
const struct var_type *raviX_binary_expression_type(const struct binary_expression *expression)
{
	return &expression->type;
}
const struct expression *raviX_binary_expression_left_expression(const struct binary_expression *expression)
{
	assert(expression->expr_left->type >= EXPR_LITERAL && expression->expr_left->type <= EXPR_FUNCTION_CALL);
	return (const struct expression *)expression->expr_left;
}
const struct expression *raviX_binary_expression_right_expression(const struct binary_expression *expression)
{
	assert(expression->expr_right->type >= EXPR_LITERAL && expression->expr_right->type <= EXPR_FUNCTION_CALL);
	return (const struct expression *)expression->expr_right;
}
BinaryOperatorType raviX_binary_expression_operator(const struct binary_expression *expression)
{
	return expression->binary_op;
}
const struct var_type *
raviX_table_element_assignment_expression_type(const struct table_element_assignment_expression *expression)
{
	return &expression->type;
}
const struct expression *
raviX_table_element_assignment_expression_key(const struct table_element_assignment_expression *expression)
{
	if (!expression->key_expr)
		return NULL;
	assert(expression->key_expr->type >= EXPR_LITERAL && expression->key_expr->type <= EXPR_FUNCTION_CALL);
	return (const struct expression *)expression->key_expr;
}
const struct expression *
raviX_table_element_assignment_expression_value(const struct table_element_assignment_expression *expression)
{
	assert(expression->value_expr->type >= EXPR_LITERAL && expression->value_expr->type <= EXPR_FUNCTION_CALL);
	return (const struct expression *)expression->value_expr;
}
const struct var_type *raviX_table_literal_expression_type(const struct table_literal_expression *expression)
{
	return &expression->type;
}
void raviX_table_literal_expression_foreach_element(
    const struct table_literal_expression *expression, void *userdata,
    void (*callback)(void *, const struct table_element_assignment_expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(expression->expr_list, node)
	{
		assert(node->type == EXPR_TABLE_ELEMENT_ASSIGN);
		callback(userdata, &node->table_elem_assign_expr);
	}
	END_FOR_EACH_PTR(node)
}

const struct var_type *raviX_suffixed_expression_type(const struct suffixed_expression *expression)
{
	return &expression->type;
}
const struct expression *raviX_suffixed_expression_primary(const struct suffixed_expression *expression)
{
	assert(expression->primary_expr->type >= EXPR_LITERAL && expression->primary_expr->type <= EXPR_FUNCTION_CALL);
	return (const struct expression *)expression->primary_expr;
}
void raviX_suffixed_expression_foreach_suffix(const struct suffixed_expression *expression, void *userdata,
					      void (*callback)(void *, const struct expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(expression->suffix_list, node)
	{
		assert(node->type >= EXPR_LITERAL && node->type <= EXPR_FUNCTION_CALL);
		callback(userdata, (struct expression *)node);
	}
	END_FOR_EACH_PTR(node)
}

const struct var_type *raviX_function_call_expression_type(const struct function_call_expression *expression)
{
	return &expression->type;
}
// Can return NULL
const StringObject *
raviX_function_call_expression_method_name(const struct function_call_expression *expression)
{
	return expression->method_name;
}
void raviX_function_call_expression_foreach_argument(const struct function_call_expression *expression, void *userdata,
						     void (*callback)(void *, const struct expression *expr))
{
	struct ast_node *node;
	FOR_EACH_PTR(expression->arg_list, node)
	{
		assert(node->type >= EXPR_LITERAL && node->type <= EXPR_FUNCTION_CALL);
		callback(userdata, (struct expression *)node);
	}
	END_FOR_EACH_PTR(node)
}
const struct function_expression *raviX_scope_owning_function(const struct block_scope *scope)
{
	assert(scope->function->type == EXPR_FUNCTION);
	return &scope->function->function_expr;
}
RAVICOMP_EXPORT const struct block_scope *raviX_scope_parent_scope(const struct block_scope *scope)
{
	return scope->parent;
}
RAVICOMP_EXPORT void raviX_scope_foreach_symbol(const struct block_scope *scope, void *userdata,
						void (*callback)(void *userdata, const struct lua_symbol *symbol))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(scope->symbol_list, symbol) { callback(userdata, symbol); }
	END_FOR_EACH_PTR(node)
}
enum symbol_type raviX_symbol_type(const struct lua_symbol *symbol) { return symbol->symbol_type; }
const struct lua_variable_symbol *raviX_symbol_variable(const struct lua_symbol *symbol)
{
	assert(symbol->symbol_type == SYM_GLOBAL || symbol->symbol_type == SYM_LOCAL);
	return &symbol->variable;
}
const struct lua_upvalue_symbol *raviX_symbol_upvalue(const struct lua_symbol *symbol)
{
	assert(symbol->symbol_type == SYM_UPVALUE);
	return &symbol->upvalue;
}
const struct lua_label_symbol *raviX_symbol_label(const struct lua_symbol *symbol)
{
	assert(symbol->symbol_type == SYM_LABEL);
	return &symbol->label;
}
const StringObject *raviX_label_name(const struct lua_label_symbol *symbol) { return symbol->label_name; }
const struct block_scope *raviX_label_scope(const struct lua_label_symbol *symbol) { return symbol->block; }
const struct var_type *raviX_upvalue_symbol_type(const struct lua_upvalue_symbol *symbol)
{
	return &symbol->value_type;
}
const struct lua_variable_symbol *raviX_upvalue_target_variable(const struct lua_upvalue_symbol *symbol)
{
	if (symbol->target_variable->symbol_type == SYM_ENV) {
		assert(symbol->target_function == NULL);
		return NULL;
	}
	assert(symbol->target_variable->symbol_type == SYM_LOCAL);
	return &symbol->target_variable->variable;
}
const struct function_expression *raviX_upvalue_target_function(const struct lua_upvalue_symbol *symbol)
{
	if (symbol->target_variable->symbol_type == SYM_ENV) {
		assert(symbol->target_function == NULL);
		return NULL;
	}
	assert(symbol->target_function->type == EXPR_FUNCTION);
	return &symbol->target_function->function_expr;
}
unsigned raviX_upvalue_index(const struct lua_upvalue_symbol *symbol) { return symbol->upvalue_index; }
