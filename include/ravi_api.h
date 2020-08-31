#ifndef ravicomp_RAVIAPI_H
#define ravicomp_RAVIAPI_H

#include "ravicomp_export.h"
#include "ravi_compiler.h"

#include <stdlib.h>

/*
 * To ensure a loose coupling between Ravi and this library we need an API definition
 * that this library can use to create various data structures needed by Ravi.
 */

typedef struct lua_State lua_State;
typedef struct Proto Proto; /* Function prototype in Ravi/Lua */
//typedef struct LClosure LClosure; /* A Lua Closure object */

typedef int (*lua_CFunction)(lua_State* L);

struct Ravi_CompilerInterface {
	void *context;
	const char *source; /* Source to be compiled */
	size_t source_len; /* Size of source */
	const char *source_name; /* Name of the source */
	Proto *main_proto; /* Main proto that will represent Lua chunk */

	/* Create new proto - this will become a child of the parent */
	Proto *(*lua_newProto) (void *context, Proto *parent);

	/* Add a string constant to Proto and return its index, information may be added to string->userdata */
	int (*lua_newStringConstant) (void *context, Proto *proto, struct string_object *string);

	/* Add an upvalue. If the upvalue refers to a local variable in parent proto then idx should contain
	 * the register for the local variable and instack should be true, else idx should have the index of
	 * upvalue in parent proto and instack should be false.
	 */
	int (*add_upvalue) (void *context, Proto *f, struct string_object* name, unsigned idx, int instack,
		     unsigned typecode, struct string_object* usertype);

	/* Compile the C code and return a module */
	void (*init_C_compiler)(void *context);
	void *(*compile_C)(void *context, const char *C_src, unsigned len);
	void (*finish_C_compiler)(void *context);

	/* Return a C function pointer by name */
	lua_CFunction(*get_compiled_function)(void *context, void *module, const char* name);

	/* Set the given function */
	void (*lua_setProtoFunction)(void* context, Proto* p, lua_CFunction func);
	/* Mark the function as var arg */
	void (*lua_setVarArg)(void *context, Proto* p);
};

RAVICOMP_EXPORT int raviX_compile(struct Ravi_CompilerInterface *compiler_interface);

#endif
