/* simple smoke test for parser */

#include "ravi_ast.h"

int main(int argc, const char* argv[]) {

	const char* code = "return { say='hello world' }";
	if (argc >= 2) {
		code = argv[1];
	}
	
	int rc = 0;
	struct ast_container* container = raviX_new_ast_container();
	rc = raviX_parse(container, code, strlen(code), "input");	
	if (rc == 0) {
		raviX_output_ast(container, stdout);
	}
	else {
		fprintf(stderr, container->error_message.buf);
	}
	raviX_ast_typecheck(container);
	raviX_destroy_ast_container(container);

	return rc;
}

