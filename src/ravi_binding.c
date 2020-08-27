/* This will contain Lua bindings */

#include "ravi_api.h"
#include "ravi_compiler.h"

#include "parser.h"
#include "cfg.h"
#include "optimizer.h"
#include "codegen.h"

int raviX_compile(struct Ravi_CompilerInterface *compiler_interface)
{
	int rc = 0;
	struct compiler_state *container = raviX_init_compiler();
	rc = raviX_parse(container, compiler_interface->source, compiler_interface->source_len, compiler_interface->source_name);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	rc = raviX_ast_simplify(container);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	struct linearizer_state *linearizer = raviX_init_linearizer(container);
	rc = raviX_ast_linearize(linearizer);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_linend;
	}
	raviX_construct_cfg(linearizer->main_proc);
	raviX_remove_unreachable_blocks(linearizer);

	membuff_t buf;
	raviX_buffer_init(&buf, 4096);
	rc = raviX_generate_C(linearizer, &buf, compiler_interface);
	raviX_buffer_free(&buf);

	L_linend:
	raviX_destroy_linearizer(linearizer);

	L_exit:
	raviX_destroy_compiler(container);

	return rc;
}