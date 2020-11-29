#ifndef ravicomp_MEMBUF_H
#define ravicomp_MEMBUF_H

#include "ravi_compiler.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

extern void raviX_buffer_add_bool(buffer_t *mb, bool value);
extern void raviX_buffer_add_int(buffer_t *mb, int value);
extern void raviX_buffer_add_longlong(buffer_t *mb, int64_t value);
extern void raviX_buffer_add_char(buffer_t *mb, char c);

/* Following add and remove raw bytes */

/* Unchecked - user must first resize */
static inline void raviX_buffer_addc(buffer_t *mb, int c)
{
	mb->buf[mb->pos++] = (char)c;
	assert(mb->pos < mb->capacity);
}
static inline void raviX_buffer_remove(buffer_t *mb, int i) { mb->pos -= i; }

#endif
