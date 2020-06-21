#ifndef ravicomp_TCOMMON_H
#define ravicomp_TCOMMON_H

struct arguments {
	const char *filename;
	const char *code;
	unsigned typecheck : 1, linearize : 1, astdump : 1, irdump : 1, cfgdump: 1;
};
extern void parse_arguments(struct arguments *args, int argc, const char *argv[]);
extern const char *read_file(const char *filename);

#endif