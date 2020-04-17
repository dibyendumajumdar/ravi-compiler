#include <ravi_compiler.h>

#include <ravi_ast.h>

const struct function_expression *raviX_ast_get_main_function(const struct compiler_state *compiler_state)
{
	return &compiler_state->main_function->function_expr;
}
const struct var_type *raviX_function_expression_type(const struct function_expression *function_expression)
{
	return &function_expression->type;
}
bool raviX_function_expression_is_vararg(const struct function_expression *function_expression)
{
	return function_expression->is_vararg;
}
bool raviX_function_expression_is_method(const struct function_expression *function_expression)
{
	return function_expression->is_method;
}
const struct function_expression *raviX_function_get_parent(const struct function_expression *function_expression)
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
struct block_scope *raviX_function_get_scope(const struct function_expression *function_expression)
{
	return function_expression->main_block;
}
void raviX_function_foreach_statement(const struct function_expression *function_expression, void *userdata,
				      void (*callback)(void *userdata, const struct statement *statement))
{
	struct ast_node *node;
	FOR_EACH_PTR(function_expression->function_statement_list, node)
	{
		assert(node->type <= AST_EXPR_STMT);
		callback(userdata, (struct statement *)node);
	}
	END_FOR_EACH_PTR(node)
}
enum ast_node_type raviX_statement_type(struct statement *statement) { return statement->type; }
void raviX_function_foreach_argument(const struct function_expression *function_expression, void *userdata,
				     void (*callback)(void *userdata, const struct lua_local_symbol *symbol))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(function_expression->args, symbol) { callback(userdata, (struct lua_local_symbol *)symbol); }
	END_FOR_EACH_PTR(symbol)
}
void raviX_function_foreach_local(const struct function_expression *function_expression, void *userdata,
				  void (*callback)(void *userdata, const struct lua_local_symbol *lua_local_symbol))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(function_expression->locals, symbol) { callback(userdata, (struct lua_local_symbol *)symbol); }
	END_FOR_EACH_PTR(symbol)
}
void raviX_function_foreach_upvalue(const struct function_expression *function_expression, void *userdata,
				    void (*callback)(void *userdata, const struct lua_upvalue_symbol *symbol))
{
	struct lua_symbol *symbol;
	FOR_EACH_PTR(function_expression->upvalues, symbol) { callback(userdata, (struct lua_upvalue_symbol *)symbol); }
	END_FOR_EACH_PTR(symbol)
}

const struct string_object *raviX_local_symbol_name(const struct lua_local_symbol *lua_local_symbol) {
	const struct lua_symbol *symbol = (const struct lua_symbol *)lua_local_symbol;
	assert(symbol->symbol_type == SYM_LOCAL);
	return symbol->var.var_name;
}

const struct var_type *raviX_local_symbol_type(const struct lua_local_symbol *lua_local_symbol) {
	const struct lua_symbol *symbol = (const struct lua_symbol *)lua_local_symbol;
	assert(symbol->symbol_type == SYM_LOCAL);
	return &symbol->value_type;
}

const struct block_scope *raviX_local_symbol_scope(const struct lua_local_symbol *lua_local_symbol) {
	const struct lua_symbol *symbol = (const struct lua_symbol *)lua_local_symbol;
	assert(symbol->symbol_type == SYM_LOCAL);
	return symbol->var.block;
}