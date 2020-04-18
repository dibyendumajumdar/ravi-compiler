#include "ravi_compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void walk_function(void *data, const struct function_expression *function) {
	printf("function\n");
	if (raviX_function_is_vararg(function)) {
		printf(" vararg: true\n");
	}
	printf(" end function\n");
}

static void walk_ast(struct compiler_state *container) {
	const struct function_expression *main_function = raviX_ast_get_main_function(container);
	printf("main function:\n");
	walk_function(NULL, main_function);
	printf("child functions:\n");
	raviX_function_foreach_child(main_function, NULL, walk_function);
}

int main(int argc, const char *argv[])
{
	if (argc != 2) {
		exit(1);
	}
	const char *code = read_file(argv[1]);
	if (!code) {
		fprintf(stderr, "No code to process\n");
		exit(1);
	}
	printf("%s\n", code);
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
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}

	walk_ast(container);

	L_exit:
	raviX_destroy_compiler(container);

	return rc;
}