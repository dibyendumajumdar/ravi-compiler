/******************************************************************************
 * Copyright (C) 2020-2021 Dibyendu Majumdar
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

/* simple smoke test for parser */

#include "ravi_compiler.h"

#include "allocate.h"
#include "cfg.h"
#include "codegen.h"
#include "membuf.h"
#include "optimizer.h"
#include "parser.h"

#include "ptrlist.h"
#include "tcommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DECLARE_PTR_LIST(string_list, char);

struct chunk_data {
	Allocator string_allocator;
	Allocator ptrlist_allocator;
	struct string_list *list;
};

/* return next line - i.e. first char following newline */
static const char *scan_next(const char *cp, const char *endp)
{
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

static void add_chunk(struct chunk_data *chunks, TextBuffer *buf)
{
	size_t len = raviX_buffer_len(buf);
	char *s = (char *) raviX_allocator_allocate(&chunks->string_allocator, len + 1);
	raviX_string_copy(s, raviX_buffer_data(buf), len + 1);
	raviX_ptrlist_add((struct ptr_list **)&chunks->list, s, &chunks->ptrlist_allocator);
}

/* Input text is supposed to contain multiple chunks
 * separated by delimiter line.
 * Each chunk will be added as an item in the list
 */
static uint32_t read_chunks(const char *input, struct chunk_data *chunks, const char *delim)
{
	TextBuffer buf;
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
		} else if (nextp) {
			raviX_buffer_add_bytes(&buf, cp, nextp - cp);
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
	raviX_allocator_init(&chunks->string_allocator, "strings", 0, sizeof(double), 1024 * 1024);
	chunks->list = NULL;
}

static void destroy_chunks(struct chunk_data *chunks)
{
	raviX_allocator_destroy(&chunks->ptrlist_allocator);
	raviX_allocator_destroy(&chunks->string_allocator);
}

static int do_code(const char *code, const struct arguments *args)
{
	int rc = 0;

	if (args->gen_C) {
		fprintf(stdout, "/*\n");
	}
	if (args->codump) {
		printf("%s\n", code);
	}

	CompilerState *container = raviX_init_compiler();
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
	if (args->simplify_ast) {
		rc = raviX_ast_simplify(container);
		if (rc != 0) {
			fprintf(stderr, "%s\n", raviX_get_last_error(container));
			goto L_exit;
		}
		if (args->astdump) {
			raviX_output_ast(container, stdout);
		}
	}
	LinearizerState *linearizer = raviX_init_linearizer(container);
	rc = raviX_ast_linearize(linearizer);
	if (rc != 0) {
		fprintf(stderr, "%s\n", raviX_get_last_error(container));
		goto L_linend;
	}
	if (args->irdump) {
		raviX_output_linearizer(linearizer, stdout);
	}
	raviX_construct_cfg(linearizer->main_proc);
	if (args->cfgdump &&
	    !args->remove_unreachable_blocks) { // Only dump out final CFG for now as we need it as clean output
		raviX_output_cfg(linearizer->main_proc, stdout);
	}
	if (args->remove_unreachable_blocks) {
		raviX_remove_unreachable_blocks(linearizer);
		if (args->irdump) {
			raviX_output_linearizer(linearizer, stdout);
		}
		if (args->cfgdump) {
			raviX_output_cfg(linearizer->main_proc, stdout);
		}
	}
	if (args->gen_C) {
		fprintf(stdout, "\n*/\n");
		raviX_generate_C_tofile(linearizer, args->mainfunc, stdout);
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

	const char *code = NULL;
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
	FOR_EACH_PTR(chunks.list, const char, chunk)
	{
		if (do_code(chunk, &args) != 0) {
			rc = 1;
		}
	}
	END_FOR_EACH_PTR(chunk)

L_exit:
	destroy_arguments(&args);
	destroy_chunks(&chunks);

	return rc;
}
