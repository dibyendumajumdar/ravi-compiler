/*
Copyright (C) 2018-2020 Dibyendu Majumdar
*/

#include "membuf.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void raviX_string_copy(char *buf, const char *src, size_t buflen)
{
	if (buflen == 0)
		return;
	strncpy(buf, src, buflen);
	buf[buflen - 1] = 0;
}

void raviX_buffer_init(membuff_t *mb, size_t initial_size)
{
	if (initial_size > 0) {
		mb->buf = (char *)calloc(1, initial_size);
		if (mb->buf == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
	} else
		mb->buf = NULL;
	mb->pos = 0;
	mb->allocated_size = initial_size;
}
void raviX_buffer_resize(membuff_t *mb, size_t new_size)
{
	if (new_size <= mb->allocated_size)
		return;
	char *newmem = (char *)realloc(mb->buf, new_size);
	if (newmem == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	mb->buf = newmem;
	mb->allocated_size = new_size;
}
void raviX_buffer_free(membuff_t *mb) { free(mb->buf); }
void raviX_buffer_add_string(membuff_t *mb, const char *str)
{
	size_t len = strlen(str);
	size_t required_size = mb->pos + len + 1; /* extra byte for NULL terminator */
	raviX_buffer_resize(mb, required_size);
	assert(mb->allocated_size - mb->pos > len);
	raviX_string_copy(&mb->buf[mb->pos], str, mb->allocated_size - mb->pos);
	mb->pos += len;
}
void raviX_buffer_add_fstring(membuff_t *mb, const char *fmt, ...)
{
	va_list args;
	char buffer[1024];

	va_start(args, fmt);
	int n = vsnprintf(buffer, sizeof buffer, fmt, args);
	va_end(args);
	if (n < 0) {
		fprintf(stderr, "Buffer overflow as input string is too large\n");
		abort();
	}
	raviX_buffer_add_string(mb, buffer);
}
void raviX_buffer_add_vfstring(membuff_t *mb, const char *fmt, va_list args)
{
	char buffer[1024];
	int n = vsnprintf(buffer, sizeof buffer, fmt, args);
	if (n < 0) {
		fprintf(stderr, "Buffer overflow as input string is too large\n");
		abort();
	}
	raviX_buffer_add_string(mb, buffer);
}
void raviX_buffer_add_bool(membuff_t *mb, bool value)
{
	if (value)
		raviX_buffer_add_string(mb, "true");
	else
		raviX_buffer_add_string(mb, "false");
}
void raviX_buffer_add_int(membuff_t *mb, int value)
{
	char temp[100];
	snprintf(temp, sizeof temp, "%d", value);
	raviX_buffer_add_string(mb, temp);
}
void raviX_buffer_add_longlong(membuff_t *mb, int64_t value)
{
	char temp[100];
	snprintf(temp, sizeof temp, "%" PRId64 "", value);
	raviX_buffer_add_string(mb, temp);
}
void raviX_buffer_add_char(membuff_t *mb, char c)
{
	char temp[2] = {c, '\0'};
	raviX_buffer_add_string(mb, temp);
}
