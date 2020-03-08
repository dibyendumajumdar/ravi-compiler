/*
A compiler for Ravi and Lua 5.3. This is work in progress.
Once ready it will be used to create a new byte code generator for Ravi.

Copyright 2018-2020 Dibyendu Majumdar
*/

#ifndef _RAVI_COMPILER_H
#define _RAVI_COMPILER_H

#include <stdio.h>

#include "ravicomp_export.h"

struct compiler_state;
struct lexer_state;
struct linearizer_state;

RAVICOMP_EXPORT struct compiler_state *raviX_init_compiler();
RAVICOMP_EXPORT void raviX_destroy_compiler(struct compiler_state *container);

/* lexical analyser */
RAVICOMP_EXPORT struct lexer_state *raviX_init_lexer(struct compiler_state *container, const char *buf, size_t buflen,
						     const char *source);
RAVICOMP_EXPORT void raviX_next(struct lexer_state *ls);
RAVICOMP_EXPORT int raviX_lookahead(struct lexer_state *ls);
RAVICOMP_EXPORT void raviX_destroy_lexer(struct lexer_state *);

/* parser and ast builder */
RAVICOMP_EXPORT const char *raviX_create_string(struct compiler_state *container, const char *s, size_t len);
RAVICOMP_EXPORT int raviX_parse(struct compiler_state *container, const char *buffer, size_t buflen, const char *name);
RAVICOMP_EXPORT void raviX_output_ast(struct compiler_state *container, FILE *fp);
RAVICOMP_EXPORT int
raviX_ast_typecheck(struct compiler_state *container); /* Perform type checks and assign types to AST */

/* linear IR generator */
RAVICOMP_EXPORT struct linearizer_state *raviX_init_linearizer(struct compiler_state *container);
RAVICOMP_EXPORT void raviX_destroy_linearizer(struct linearizer_state *linearizer);
RAVICOMP_EXPORT int raviX_ast_linearize(struct linearizer_state *linearizer);
RAVICOMP_EXPORT void raviX_output_linearizer(struct linearizer_state *linearizer, FILE *fp);

/* utilies */
RAVICOMP_EXPORT const char *raviX_get_last_error(struct compiler_state *container);

#endif
