#include <ravi_compiler.h>

#include <ravi_ast.h>

enum ast_node_type raviX_ast_node_get_type(const struct ast_node *n) {
	return n->type;
}
const struct ast_node* raviX_ast_node_get_root(const struct compiler_state *compiler_state) {
	return compiler_state->main_function;
}


