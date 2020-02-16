/* simple smoke test for parser */

#include "ravi_ast.h"

int main(int argc, const char* argv[]) {

	//const char* code = "return { say='hello world' }";
	//const char* code = "if true then return 1 elseif false then return 2 else return 0 end";
	//const char* code = "if 1 == 1 then return 1 else return 2 end";
	const char* code = "if 1 == 1 then return 1 elseif 1 > 2 then return 2 else return 2 end";
	if (argc >= 2) {
		code = argv[1];
	}
	
	int rc = 0;
	struct ast_container* container = raviX_new_ast_container();
	rc = raviX_parse(container, code, strlen(code), "input");	
	if (rc != 0) {
		fprintf(stderr, container->error_message.buf);
		goto L_exit;
	}
	raviX_output_ast(container, stdout);
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		fprintf(stderr, container->error_message.buf);
		goto L_exit;
	}
	raviX_output_ast(container, stdout);

	struct linearizer linearizer;
	raviX_init_linearizer(&linearizer, container);

	rc = raviX_ast_linearize(&linearizer);
	if (rc != 0) {
		fprintf(stderr, container->error_message.buf);
		goto L_linend;
	}
	raviX_output_linearizer(&linearizer, stdout);
	
L_linend:
	raviX_destroy_linearizer(&linearizer);

L_exit:
	raviX_destroy_ast_container(container);

	return rc;
}

