#ifndef ravicomp_MEMBUF_H
#define ravicomp_MEMBUF_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	char *buf;
	size_t allocated_size;
	size_t pos;
} membuff_t;

extern void membuff_init(membuff_t *mb, size_t initial_size);
extern void membuff_rewindpos(membuff_t *mb);
extern void membuff_resize(membuff_t *mb, size_t new_size);
extern void membuff_free(membuff_t *mb);
extern void membuff_add_string(membuff_t *mb, const char *str);
extern void membuff_add_fstring(membuff_t *mb, const char *str, ...);
extern void membuff_add_vfstring(membuff_t *mb, const char *fmt, va_list args);
extern void membuff_add_bool(membuff_t *mb, bool value);
extern void membuff_add_int(membuff_t *mb, int value);
extern void membuff_add_longlong(membuff_t *mb, int64_t value);
extern void membuff_add_char(membuff_t *mb, char c);

/* strncpy() with guaranteed 0 termination */
extern void ravi_string_copy(char *buf, const char *src, size_t buflen);

typedef struct Mbuffer {
	char *buffer;
	size_t n;
	size_t buffsize;
} Mbuffer;

static inline void luaZ_initbuffer(Mbuffer *buff)
{
	buff->buffer = NULL;
	buff->buffsize = 0;
	buff->n = 0;
}

static inline char *luaZ_buffer(Mbuffer *buff) { return buff->buffer; }
static inline size_t luaZ_sizebuffer(Mbuffer *buff) { return buff->buffsize; }
static inline size_t luaZ_bufflen(Mbuffer *buff) { return buff->n; }
static inline void luaZ_addc(Mbuffer *buff, int c) { buff->buffer[buff->n++] = (char)c; }
static inline void luaZ_buffremove(Mbuffer *buff, int i) { buff->n -= i; }
static inline void luaZ_resetbuffer(Mbuffer *buff) { buff->n = 0; }
static inline void luaZ_resizebuffer(Mbuffer *buff, size_t size)
{
	if (size == 0) {
		free(buff->buffer);
		luaZ_initbuffer(buff);
	} else {
		buff->buffer = realloc(buff->buffer, size);
		buff->buffsize = size;
	}
}
static inline void luaZ_freebuffer(Mbuffer *buff) { luaZ_resizebuffer(buff, 0); }

#endif
