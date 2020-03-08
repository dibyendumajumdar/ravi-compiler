/* simple smoke test for parser */

#include "ravi_compiler.h"

#include <string.h>

int main(int argc, const char *argv[])
{

	// const char* code = "return { say='hello world' }";
	// const char* code = "if true then return 1 elseif false then return 2 else return 0 end";
	// const char* code = "if 1 == 1 then return 1 else return 2 end";
	const char *code = "if 1 == 1 then return 1 elseif 1 > 2 then return 2 else return 2 end";
	if (argc >= 2) {
		code = argv[1];
	}

	int rc = 0;
	struct compiler_state *container = raviX_init_compiler();
	rc = raviX_parse(container, code, strlen(code), "input");
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	raviX_output_ast(container, stdout);
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		fprintf(stderr, raviX_get_last_error(container));
		goto L_exit;
	}
	raviX_output_ast(container, stdout);

	struct linearizer_state *linearizer = raviX_init_linearizer(container);

	rc = raviX_ast_linearize(linearizer);
	if (rc != 0) {
		fprintf(stderr, raviX_get_last_error(container));
		goto L_linend;
	}
	raviX_output_linearizer(linearizer, stdout);

L_linend:
	raviX_destroy_linearizer(linearizer);

L_exit:
	raviX_destroy_compiler(container);

	return rc;
}
