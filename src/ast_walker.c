#include <ravi_compiler.h>

#include <ravi_ast.h>

enum ast_node_type raviX_ast_node_get_type(const struct ast_node *n) {
	return n->type;
}
const struct function_expression* raviX_ast_node_get_main(const struct compiler_state *compiler_state) {
	return &compiler_state->main_function->function_expr;
}
const struct var_type* raviX_function_expression_type(const struct function_expression *function_expression) {
	return &function_expression->type;
}
bool raviX_function_expression_is_vararg(const struct function_expression *function_expression) {
	return function_expression->is_vararg;
}
bool raviX_function_expression_is_method(const struct function_expression *function_expression) {
	return function_expression->is_method;
}
const struct function_expression* raviX_function_get_parent(const struct function_expression *function_expression) {
	if (function_expression->parent_function == NULL)
		return NULL;
	else
		return &function_expression->parent_function->function_expr;
}
void raviX_function_foreach_child(const struct function_expression *function_expression, void *userdata, void (*callback)(void *userdata, const struct function_expression *function_expression)) {
	struct ast_node *node;
	FOR_EACH_PTR(function_expression->child_functions, node) {
		callback(userdata, &node->function_expr);
	}
	END_FOR_EACH_PTR(node)
}



