/**
 * This is a sample AST walker that illustrates how to use the
 * AST api. Note that this sample doesn't actually do anything.
 */

#include "ravi_compiler.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct arguments {
	const char *filename;
	const char *code;
	unsigned typecheck : 1, linearize : 1;
};

static void parse_arguments(struct arguments *args, int argc, const char *argv[])
{
	memset(args, 0, sizeof *args);
	args->typecheck = 1;
	args->linearize = 1;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--notypecheck") == 0) {
			args->typecheck = 0;
		} else if (strcmp(argv[i], "--nolinearize") == 0) {
			args->linearize = 0;
		} else if (strcmp(argv[i], "-f") == 0) {
			if (args->filename) {
				fprintf(stderr, "-f already accepted\n");
				continue;
			}
			if (i < argc - 1) {
				i++;
				args->filename = strdup(argv[i]);
			} else {
				fprintf(stderr, "Missing file name after -f\n");
				exit(1);
			}
		} else {
			if (args->code) {
				fprintf(stderr, "Bad argument at %d", i);
				exit(1);
			} else {
				args->code = strdup(argv[i]);
			}
		}
	}
}

static const char *read_file(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open file %s\n", filename);
		return NULL;
	}
	if (fseek(fp, 0, SEEK_END) != 0) {
		fprintf(stderr, "Failed to seek to file end\n");
		fclose(fp);
		return NULL;
	}
	long long len = ftell(fp);
	if (fseek(fp, 0, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to seek to file beginning\n");
		fclose(fp);
		return NULL;
	}
	char *buffer = calloc(1, len + 10);
	size_t n = fread(buffer, 1, len, fp);
	if (n == 0) {
		fprintf(stderr, "Failed to read file\n");
		fclose(fp);
		free(buffer);
		return NULL;
	}
	fclose(fp);
	return buffer;
}

struct ast_state {
	int dummy;
};

static void walk_symbol(void *data, const struct lua_symbol *symbol)
{
	enum symbol_type type = raviX_symbol_type(symbol);
	switch (type) {
	case SYM_LABEL: {
		const struct lua_label_symbol *label = raviX_symbol_label(symbol);
		(void)label;
		break;
	}
	case SYM_GLOBAL:
	case SYM_LOCAL: {
		const struct lua_variable_symbol *variable = raviX_symbol_variable(symbol);
		(void)variable;
		break;
	}
	case SYM_UPVALUE: {
		const struct lua_upvalue_symbol *upvalue = raviX_symbol_upvalue(symbol);
		(void)upvalue;
		break;
	}
	default:
		assert(false);
	}
}

static void walk_function(void *data, const struct function_expression *function)
{
	struct ast_state *state = (struct ast_state *)data;
	if (raviX_function_is_vararg(function)) {
	}
	if (raviX_function_is_method(function)) {
	}
	const struct block_scope *scope = raviX_function_scope(function);
	assert(function == raviX_scope_owning_function(scope));
	const struct block_scope *parent = raviX_scope_parent_scope(scope);
	(void)parent;
	raviX_scope_foreach_symbol(scope, state, walk_symbol);
}

static void walk_ast(struct compiler_state *container)
{
	// Dummy struct - in a useful implementation this is where you
	// would maintain state
	struct ast_state state = {0};
	// First lets get the main function from the parse tree
	const struct function_expression *main_function = raviX_ast_get_main_function(container);
	walk_function(&state, main_function);
	// Now walk all the child functions
	raviX_function_foreach_child(main_function, &state, walk_function);
}

int main(int argc, const char *argv[])
{
	struct arguments args;
	parse_arguments(&args, argc, argv);
	const char *code = NULL;
	if (args.code) {
		code = args.code;
	} else if (args.filename) {
		code = read_file(args.filename);
	}
	if (!code) {
		fprintf(stderr, "No code to process\n");
		exit(1);
	}
	int rc = 0;
	struct compiler_state *container = raviX_init_compiler();
	rc = raviX_parse(container, code, strlen(code), "input");
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}

	walk_ast(container);

L_exit:
	raviX_destroy_compiler(container);

	return rc;
}