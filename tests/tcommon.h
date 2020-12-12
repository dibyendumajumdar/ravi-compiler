#ifndef ravicomp_TCOMMON_H
#define ravicomp_TCOMMON_H

struct arguments {
	const char *filename;
	const char *code;
	unsigned typecheck : 1, linearize : 1, astdump : 1, irdump : 1, cfgdump : 1, codump : 1, simplify_ast : 1,
	    remove_unreachable_blocks: 1, gen_C: 1;
	const char *mainfunc; /* name of the main function in generated code, only applies if gen_C is on */
};
extern void parse_arguments(struct arguments *args, int argc, const char *argv[]);
extern void destroy_arguments(struct arguments *args);
extern const char *read_file(const char *filename);

#endif