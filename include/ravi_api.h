#ifndef ravicomp_RAVIAPI_H
#define ravicomp_RAVIAPI_H

#include "ravicomp_export.h"

#include <stdlib.h>

/*
 * To ensure a loose coupling between Ravi and this library we need an API definition
 * that this library can use to create various data structures needed by Ravi.
 */

//typedef struct lua_State lua_State;
typedef struct Proto Proto; /* Function prototype in Ravi/Lua */
//typedef struct LClosure LClosure; /* A Lua Closure object */

struct Ravi_CompilerInterface {
	void *context;
	const char *source; /* Source to be compiled */
	size_t source_len; /* Size of source */
	const char *source_name; /* Name of the source */

	/* Create new proto - if parent is not NULL then this will become a child of the parent */
	Proto *(*lua_newProto) (void *context, Proto *parent);

	/* Create a Lua Closure object with specified number of upvalues */
	//LClosure *(*lua_newLuaClosure) (lua_State *L, int number_of_upvalues);

	/* Add a string constant to Proto and return its index */
	int (*lua_newStringConstant) (void *context, Proto *proto, const char *s, unsigned len);

	/* Compile the C code for the given proto, and C source */
	void (*lua_compileProto)(void *context, Proto *proto, const char *C_src, unsigned len);
};

RAVICOMP_EXPORT Proto *raviX_compile(struct Ravi_CompilerInterface *compiler_interface);

#endif
