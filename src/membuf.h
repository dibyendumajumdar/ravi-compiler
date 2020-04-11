#ifndef ravicomp_MEMBUF_H
#define ravicomp_MEMBUF_H

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "port.h"

typedef struct {
	char *buf;
	size_t allocated_size;
	size_t pos;
} membuff_t;

extern void raviX_buffer_init(membuff_t *mb, size_t initial_size);
extern void raviX_buffer_resize(membuff_t *mb, size_t new_size);
extern void raviX_buffer_free(membuff_t *mb);
static inline char *raviX_buffer_buffer(membuff_t *mb) { return mb->buf; }
static inline size_t raviX_buffer_size(membuff_t *mb) { return mb->allocated_size; }
static inline size_t raviX_buffer_len(membuff_t *mb) { return mb->pos; }
static inline void raviX_buffer_reset(membuff_t *mb) { mb->pos = 0; }

/* following convert input to string before adding */
extern void raviX_buffer_add_string(membuff_t *mb, const char *str);
extern void raviX_buffer_add_fstring(membuff_t *mb, const char *str, ...) FORMAT_ATTR(2);
extern void raviX_buffer_add_vfstring(membuff_t *mb, const char *fmt, va_list args);
extern void raviX_buffer_add_bool(membuff_t *mb, bool value);
extern void raviX_buffer_add_int(membuff_t *mb, int value);
extern void raviX_buffer_add_longlong(membuff_t *mb, int64_t value);
extern void raviX_buffer_add_char(membuff_t *mb, char c);

/* Following add and remove raw bytes */

/* Unchecked - user must first resize */
static inline void raviX_buffer_addc(membuff_t *mb, int c)
{
	mb->buf[mb->pos++] = (char)c;
	assert(mb->pos < mb->allocated_size);
}
static inline void raviX_buffer_remove(membuff_t *mb, int i)
{
	mb->pos -= i;
}

/* strncpy() replacement with guaranteed 0 termination */
extern void raviX_string_copy(char *buf, const char *src, size_t buflen);

#endif
