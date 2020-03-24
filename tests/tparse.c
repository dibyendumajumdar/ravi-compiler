/* simple smoke test for parser */

#include "ravi_compiler.h"

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
	char *buffer = calloc(1, len+10);
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

int main(int argc, const char *argv[])
{
	struct arguments args;
	parse_arguments(&args, argc, argv);

	// const char* code = "return { say='hello world' }";
	// const char* code = "if true then return 1 elseif false then return 2 else return 0 end";
	// const char* code = "if 1 == 1 then return 1 else return 2 end";
	//const char *code = "if 1 == 1 then return 1 elseif 1 > 2 then return 2 else return 2 end";
	const char* code = "local i: integer; return t[i/5]";
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
