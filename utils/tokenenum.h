#include "tokenmacro.h"
enum TokenType {
	TOK_OFS = 256,
#define TKENUM1(name)		TOK_##name,
#define TKENUM2(name, sym)	TOK_##name,
#define TKENUM3(name, sym)	TOK_##name,
	TKDEF(TKENUM1, TKENUM2, TKENUM3)
#undef TKENUM1
#undef TKENUM2
#undef TKENUM3
	FIRST_RESERVED = TOK_OFS +1,
	LAST_RESERVED = TOK_while - TOK_OFS
};