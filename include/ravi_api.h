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
	Proto *main_proto; /* Main proto that will represent Lua chunk, must be set by Ravi */

	/* ------------------------ Following apis are for manipulating Ravi/Lua data structures ---------------- */
	/* Create a new proto - this will become a child of the parent */
	Proto *(*lua_newProto) (void *context, Proto *parent);
	/* Add a string constant to Proto and return its index. Ravi may attach information to string->userdata */
	int (*lua_newStringConstant) (void *context, Proto *proto, struct string_object *string);
	/* Add an upvalue. If the upvalue refers to a local variable in parent proto then idx should contain
	 * the register for the local variable and instack should be true, else idx should have the index of
	 * upvalue in parent proto and instack should be false.
	 */
	int (*lua_addUpValue) (void *context, Proto *f, struct string_object* name, unsigned idx, int instack,
		     unsigned typecode, struct string_object* usertype);
	/* Set the given function as the target for the Proto, i.e. this is the JIT function */
	void (*lua_setProtoFunction)(void* context, Proto* p, lua_CFunction func);
	/* Mark the function as receiving variable arguments ... (not yet supported) */
	void (*lua_setVarArg)(void *context, Proto* p);
	/* Set the initial stack size of the function */
	void (*lua_setMaxStackSize)(void *context, Proto *p, unsigned max_stack_size);
	/* Set the number of regular arguments that wwill be accepted by the function */
	void (*lua_setNumParams)(void *context, Proto *p, unsigned num_params);

	/* ------------------------ Following apis are for compiling code -------------------------------- */
	/* Initialize the compiler */
	void (*init_C_compiler)(void *context);
	/* Compile the given C code and return a module */
	void *(*compile_C)(void *context, const char *C_src, unsigned len);
	/* Clean up compiler state */
	void (*finish_C_compiler)(void *context);

	/* Return a C function pointer by name
	 * module - as returned by compile_C().
	 */
	lua_CFunction(*get_compiled_function)(void *context, void *module, const char* name);

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
