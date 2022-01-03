/******************************************************************************
 * Copyright (C) 2020-2022 Dibyendu Majumdar
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

#include "tcommon.h"
#include "ravi_alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_arguments(struct arguments *args, int argc, const char *argv[])
{
	memset(args, 0, sizeof *args);
	args->typecheck = 1;
	args->linearize = 1;
	args->astdump = 1;
	args->irdump = 1;
	args->cfgdump = 1;
	args->codump = 1;
	args->simplify_ast = 0;
	args->remove_unreachable_blocks = 0;
	args->gen_C = 0;
	args->mainfunc = "setup";
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--notypecheck") == 0) {
			args->typecheck = 0;
		} else if (strcmp(argv[i], "--nolinearize") == 0) {
			args->linearize = 0;
		} else if (strcmp(argv[i], "--noastdump") == 0) {
			args->astdump = 0;
		} else if (strcmp(argv[i], "--noirdump") == 0) {
			args->irdump = 0;
		} else if (strcmp(argv[i], "--nocodump") == 0) {
			args->codump = 0;
		} else if (strcmp(argv[i], "--nocfgdump") == 0) {
			args->cfgdump = 0;
		} else if (strcmp(argv[i], "--simplify-ast") == 0) {
			args->simplify_ast = 1;
		} else if (strcmp(argv[i], "--remove-unreachable-blocks") == 0) {
			args->remove_unreachable_blocks = 1;
		} else if (strcmp(argv[i], "--gen-C") == 0) {
			args->gen_C = 1;
		} else if (strcmp(argv[i], "-main") == 0) {
			if (i < argc - 1) {
				i++;
				args->mainfunc = strdup(argv[i]);
			} else {
				fprintf(stderr, "Missing argument after -main\n");
				exit(1);
			}
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
	if (args->filename && !args->code) {
		args->code = read_file(args->filename);
	}
}

const char *read_file(const char *filename)
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
	char *buffer = (char *)calloc(1, len + 10);
        if(!buffer) {
                fprintf(stderr, "out of memory\n");
                exit(1);
        }
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

void destroy_arguments(struct arguments *args)
{
	free((void *)args->filename);
	free((void *)args->code);
}

void create_allocator(C_MemoryAllocator *allocator) {
	allocator->arena = create_mspace(0, 0);
	allocator->realloc = mspace_realloc;
	allocator->calloc = mspace_calloc;
	allocator->free = mspace_free;
	allocator->create_arena = create_mspace;
	allocator->destroy_arena = destroy_mspace;
}

void destroy_allocator(C_MemoryAllocator *allocator) {
	allocator->destroy_arena(allocator->arena);
}
