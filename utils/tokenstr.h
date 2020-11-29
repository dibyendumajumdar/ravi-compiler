#include "tokenmacro.h"
/* ORDER TokenType */
static const char *const luaX_tokens[] = {
#define TKSTR1(name)		#name,
#define TKSTR2(name, sym)	#sym,
#define TKSTR3(name, sym)	#sym #sym,
    TKDEF(TKSTR1, TKSTR2, TKSTR3)
#undef TKSTR1
#undef TKSTR2
#undef TKSTR3
};