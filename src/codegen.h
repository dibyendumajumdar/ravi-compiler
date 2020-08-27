#ifndef ravicomp_CODEGEN_H
#define ravicomp_CODEGEN_H

#include "ravi_compiler.h"
#include "ravi_api.h"
#include "linearizer.h"

RAVICOMP_EXPORT int raviX_generate_C(struct linearizer_state *linearizer, membuff_t *mb, struct Ravi_CompilerInterface *ravi_interface);
RAVICOMP_EXPORT void raviX_generate_C_tofile(struct linearizer_state *linearizer, FILE *fp);

#endif