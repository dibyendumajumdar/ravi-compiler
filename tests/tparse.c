/* simple smoke test for parser */

#include "ravi_compiler.h"
#include "parser.h"
#include "tcommon.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	struct arguments args;
	parse_arguments(&args, argc, argv);

	// const char *code = "return { say='hello world' }";
	// const char* code = "if true then return 1 elseif false then return 2 else return 0 end";
	// const char* code = "if 1 == 1 then return 1 else return 2 end";
	// const char *code = "if 1 == 1 then return 1 elseif 1 > 2 then return 2 else return 2 end";
	// const char* code = "local i: integer; return t[i/5]";
	// const char* code = "local i return function(a) i = a; return i end";
	// const char *code = "return -0//1";
	//const char* code = "::L1:: a = 1; goto L1; return";
	//const char* code = "goto l1; do ::l1:: end";
	//const char* code = "do ::l1:: end goto l1;";
	//const char* code = "::l1:: do goto l1; x = 1; ::l1:: z = 2 end y = 1; goto l1";
	const char* code = "for i=1,10 do print(i) end";
	if (args.code) {
		code = args.code;
	}
	int rc = 0;
	if (!code) {
		fprintf(stderr, "No code to process\n");
		rc = 1;
		goto L_exit;
	}

	printf("%s\n", code);
	CompilerState *container = raviX_init_compiler();
	rc = raviX_parse(container, code, strlen(code), "input");
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	raviX_output_ast(container, stdout);
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	raviX_output_ast(container, stdout);
	if (args.simplify_ast) {
		rc = raviX_ast_simplify(container);
		if (rc != 0) {
			fprintf(stderr, "%s\n", raviX_get_last_error(container));
			goto L_exit;
		}
		if (args.astdump) {
			raviX_output_ast(container, stdout);
		}
	}
	struct linearizer_state *linearizer = raviX_init_linearizer(container);

	rc = raviX_ast_linearize(linearizer);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_linend;
	}
	raviX_output_linearizer(linearizer, stdout);

L_linend:
	raviX_destroy_linearizer(linearizer);

L_exit:
	raviX_destroy_compiler(container);
	destroy_arguments(&args);

	return rc;
}
