#ifndef ravicomp_RAVIAPI_H
#define ravicomp_RAVIAPI_H

#include "ravicomp_export.h"
#include "ravi_compiler.h"

#include <stdlib.h>

/*
 * To ensure a loose coupling between Ravi and the Compiler library we need an API definition
 * that the Compiler library can use to create various data structures needed by Ravi.
 *
 * Below is the interface between Ravi and the compiler. The methods defined in this
 * interface must be provide by Ravi so that the compiler can update the data structures needed
 * to create objects inside Ravi.
 *
 * The Compiler builds appropriate structures within Ravi/Lua while compiling. This apprach is
 * however not very sited to ahead of time compilation, so we should probably rethink how this
 * works. For AOT a better approach would be to construct the whle structure in the compiler
 * and hand it back - but this means having knowledge of Ravi/Lua internals.
 */

typedef struct lua_State lua_State;
typedef struct Proto Proto; /* Function prototype in Ravi/Lua */

typedef int (*lua_CFunction)(lua_State* L);

struct Ravi_CompilerInterface {
	void *context; /* Ravi supplied context */
	const char *source; /* Source code to be compiled - managed by Ravi */
	size_t source_len; /* Size of source code */
	const char *source_name; /* Name of the source */

	const char* generated_code;  /* Output of the compiler */

	/* ------------------------ Debugging and error handling ----------------------------------------- */
	void (*debug_message)(void *context, const char *filename, long long line, const char *message);
	void (*error_message)(void *context, const char *message);
};

/**
 * This is the API exposed by the Compiler itself. This functin is invoked by
 * Ravi when it is necessary to compile some Ravi code.
 * @param compiler_interface The interface expected by the compiler must be setup
 * @return 0 for success, non-zero for failure
 */
RAVICOMP_EXPORT int raviX_compile(struct Ravi_CompilerInterface *compiler_interface);

#endif
