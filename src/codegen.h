#ifndef ravicomp_CODEGEN_H
#define ravicomp_CODEGEN_H

#include "ravi_compiler.h"
#include "membuf.h"
#include "linearizer.h"

RAVICOMP_EXPORT void raviX_generate_C(struct linearizer_state *linearizer, membuff_t *mb);
RAVICOMP_EXPORT void raviX_generate_C_tofile(struct linearizer_state *linearizer, FILE *fp);

#endif