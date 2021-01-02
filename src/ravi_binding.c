/* This will contain Lua bindings */

#include "ravi_api.h"
#include "ravi_compiler.h"

#include "cfg.h"
#include "codegen.h"
#include "optimizer.h"
#include "parser.h"

int raviX_compile(struct Ravi_CompilerInterface *compiler_interface)
{
	int rc = 0;
	int dump_ir = 0;
	if (compiler_interface->compiler_options != NULL) {
		dump_ir = strstr(compiler_interface->compiler_options, "--dump-ir") != NULL;
	}
	compiler_interface->generated_code = NULL;
	CompilerState *container = raviX_init_compiler();
	rc = raviX_parse(container, compiler_interface->source, compiler_interface->source_len,
			 compiler_interface->source_name);
	if (rc != 0) {
		compiler_interface->error_message(compiler_interface->context, raviX_get_last_error(container));
		goto L_exit;
	}
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		compiler_interface->error_message(compiler_interface->context, raviX_get_last_error(container));
		goto L_exit;
	}
	rc = raviX_ast_simplify(container);
	if (rc != 0) {
		compiler_interface->error_message(compiler_interface->context, raviX_get_last_error(container));
		goto L_exit;
	}
	LinearizerState *linearizer = raviX_init_linearizer(container);
	rc = raviX_ast_linearize(linearizer);
	if (rc != 0) {
		compiler_interface->error_message(compiler_interface->context, raviX_get_last_error(container));
		goto L_linend;
	}
	raviX_construct_cfg(linearizer->main_proc);
	raviX_remove_unreachable_blocks(linearizer);

	TextBuffer buf;
	raviX_buffer_init(&buf, 4096);
	if (dump_ir) {
		raviX_buffer_add_string(&buf, "/* Following is an IR Dump from the compiler\n");
		raviX_show_linearizer(linearizer, &buf);
		raviX_buffer_add_string(&buf, "\nEnd of IR dump*/\n");
	}
	rc = raviX_generate_C(linearizer, &buf, compiler_interface);
	if (rc != 0) {
		raviX_buffer_free(&buf);
	} else {
		compiler_interface->generated_code = buf.buf;
	}

L_linend:
	raviX_destroy_linearizer(linearizer);

L_exit:
	raviX_destroy_compiler(container);

	return rc;
}