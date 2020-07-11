/* simple smoke test for parser */

#include "ravi_compiler.h"
#include "tcommon.h"
#include "ptrlist.h"
#include "allocate.h"
#include "membuf.h"
#include "implementation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DECLARE_PTR_LIST(string_list, char);

struct chunk_data {
	struct allocator string_allocator;
	struct allocator ptrlist_allocator;
	struct string_list *list;
};

/* return next line - i.e. first char following newline */
static const char* scan_next(const char *cp, const char *endp) {
	if (cp >= endp)
		return NULL;
	const char *nextp = strchr(cp, '\n');
	if (!nextp)
		// In case we dont have line ending in new line
		return endp;
	nextp++; // skip past newline
	if (nextp > endp)
		return NULL;
	return nextp;
}

static void add_chunk(struct chunk_data *chunks, membuff_t* buf)
{
	size_t len = raviX_buffer_len(buf);
	char *s = raviX_allocator_allocate(&chunks->string_allocator, len+1);
	raviX_string_copy(s, raviX_buffer_data(buf), len+1);
	ptrlist_add((struct ptr_list **) &chunks->list, s, &chunks->ptrlist_allocator);
}

/* Input text is supposed to contain multiple chunks
 * separated by delimiter line.
 * Each chunk will be added as an item in the list
 */
static uint32_t read_chunks(const char *input, struct chunk_data *chunks, const char *delim) {
	membuff_t buf;
	raviX_buffer_init(&buf, 1024);
	const char *cp = input;
	const char *endp = input + strlen(input);
	const char *nextp = cp;
	uint32_t count = 0;
	while (nextp) {
		cp = nextp;
		nextp = scan_next(cp, endp);
		if (*cp == 0)
			continue;
		if (*cp == *delim) {
			add_chunk(chunks, &buf);
			count++;
			raviX_buffer_reset(&buf);
		}
		else if (nextp) {
			 raviX_buffer_add_bytes(&buf, cp, nextp-cp);
		}
	}
	if (raviX_buffer_len(&buf) > 0) {
		add_chunk(chunks, &buf);
		count++;
	}
	raviX_buffer_free(&buf);
	return count;
}

static void init_chunks(struct chunk_data *chunks)
{
	raviX_allocator_init(&chunks->ptrlist_allocator, "ptrlists", sizeof(struct ptr_list), sizeof(double),
			     sizeof(struct ptr_list) * 32);
	raviX_allocator_init(&chunks->string_allocator, "strings", 0, sizeof(double), 1024);
	chunks->list = NULL;
}

static void destroy_chunks(struct chunk_data *chunks)
{
	raviX_allocator_destroy(&chunks->ptrlist_allocator);
	raviX_allocator_destroy(&chunks->string_allocator);
}

static int do_code(const char *code, const struct arguments *args)
{
	if (args->codump) {
		printf("%s\n", code);
	}
	int rc = 0;
	struct compiler_state *container = raviX_init_compiler();
	rc = raviX_parse(container, code, strlen(code), "input");
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	if (args->astdump) {
		raviX_output_ast(container, stdout);
	}
	rc = raviX_ast_typecheck(container);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_exit;
	}
	if (args->astdump) {
		raviX_output_ast(container, stdout);
	}
	struct linearizer_state *linearizer = raviX_init_linearizer(container);
	rc = raviX_ast_linearize(linearizer);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_linend;
	}
	if (args->irdump) {
		raviX_output_linearizer(linearizer, stdout);
	}
	raviX_construct_cfg(linearizer->main_proc);
	if (args->cfgdump) {
		raviX_output_cfg(linearizer->main_proc, stdout);
	}

	L_linend:
	raviX_destroy_linearizer(linearizer);

	L_exit:
	raviX_destroy_compiler(container);

	return rc;
}

int main(int argc, const char *argv[])
{
	struct arguments args;
	struct chunk_data chunks;

	parse_arguments(&args, argc, argv);
	init_chunks(&chunks);

	const char* code = NULL;
	if (args.code) {
		code = args.code;
	}
	int rc = 0;
	if (!code) {
		fprintf(stderr, "No code to process\n");
		rc = 1;
		goto L_exit;
	}

	uint32_t count = read_chunks(code, &chunks, "#");
	if (count == 0) {
		fprintf(stderr, "No code to process\n");
		rc = 1;
		goto L_exit;
	}

	const char *chunk = NULL;
	FOR_EACH_PTR(chunks.list, chunk) {
		do_code(chunk, &args);
	} END_FOR_EACH_PTR(chunk)

L_exit:
	destroy_arguments(&args);
	destroy_chunks(&chunks);

	return 0;
}
