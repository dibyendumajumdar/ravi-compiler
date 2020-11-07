/*
 * Convert the linear IR to C code that can be compiled and
 * executed by Ravi VM.
 */

#include "codegen.h"
#include "ravi_api.h"

#include <assert.h>
#include <stddef.h>

/*
 * Only 64-bits supported right now
 * Following must be kept in sync with changes in the actual header files
 */

static const char Lua_header[] =
    //"typedef __SIZE_TYPE__ size_t;\n"
    "typedef unsigned long long size_t;\n"
    "typedef long long ptrdiff_t;\n"
    "typedef long long intptr_t;\n"
    "typedef long long int64_t;\n"
    "typedef unsigned long long uint64_t;\n"
    "typedef int int32_t;\n"
    "typedef unsigned int uint32_t;\n"
    "typedef short int16_t;\n"
    "typedef unsigned short uint16_t;\n"
    "typedef char int8_t;\n"
    "typedef unsigned char uint8_t;\n"
    "typedef size_t lu_mem;\n"
    "typedef ptrdiff_t l_mem;\n"
    "typedef unsigned char lu_byte;\n"
    "typedef uint16_t LuaType;\n"
    "#define NULL ((void *)0)\n"
    "typedef struct lua_State lua_State;\n"
    "#define LUA_TNONE		(-1)\n"
    "#define LUA_TNIL		0\n"
    "#define LUA_TBOOLEAN		1\n"
    "#define LUA_TLIGHTUSERDATA	2\n"
    "#define LUA_TNUMBER		3\n"
    "#define LUA_TSTRING		4\n"
    "#define LUA_TTABLE		5\n"
    "#define LUA_TFUNCTION		6\n"
    "#define LUA_TUSERDATA		7\n"
    "#define LUA_TTHREAD		8\n"
    "#define LUA_OK  0\n"
    "typedef enum {TM_INDEX,TM_NEWINDEX,TM_GC,\n"
    "	TM_MODE,TM_LEN,TM_EQ,TM_ADD,TM_SUB,TM_MUL,\n"
    "	TM_MOD,TM_POW,TM_DIV,TM_IDIV,TM_BAND,TM_BOR,\n"
    "	TM_BXOR,TM_SHL,TM_SHR,TM_UNM,TM_BNOT,TM_LT,\n"
    "	TM_LE,TM_CONCAT,TM_CALL,TM_N\n"
    "} TMS;\n"
    "typedef double lua_Number;\n"
    "typedef int64_t lua_Integer;\n"
    "typedef uint64_t lua_Unsigned;\n"
    "typedef int (*lua_CFunction) (lua_State *L);\n"
    "typedef union {\n"
    "	lua_Number n;\n"
    "	double u;\n"
    "	void *s;\n"
    "	lua_Integer i;\n"
    "	long l;\n"
    "} L_Umaxalign;\n"
    "#define lua_assert(c)		((void)0)\n"
    "#define check_exp(c,e)		(e)\n"
    "#define lua_longassert(c)	((void)0)\n"
    "#define luai_apicheck(l,e)	lua_assert(e)\n"
    "#define api_check(l,e,msg)	luai_apicheck(l,(e) && msg)\n"
    "#define UNUSED(x)	((void)(x))\n"
    "#define cast(t, exp)	((t)(exp))\n"
    "#define cast_void(i)	cast(void, (i))\n"
    "#define cast_byte(i)	cast(lu_byte, (i))\n"
    "#define cast_num(i)	cast(lua_Number, (i))\n"
    "#define cast_int(i)	cast(int, (i))\n"
    "#define cast_uchar(i)	cast(unsigned char, (i))\n"
    "#define l_castS2U(i)	((lua_Unsigned)(i))\n"
    "#define l_castU2S(i)	((lua_Integer)(i))\n"
    "#define l_noret		void\n"
    "typedef unsigned int Instruction;\n"
    "#define luai_numidiv(L,a,b)     ((void)L, l_floor(luai_numdiv(L,a,b)))\n"
    "#define luai_numdiv(L,a,b)      ((a)/(b))\n"
    "#define luai_nummod(L,a,b,m)  \\\n"
    "  { (m) = l_mathop(fmod)(a,b); if ((m)*(b) < 0) (m) += (b); }\n"
    "#define LUA_TLCL	(LUA_TFUNCTION | (0 << 4))\n"
    "#define LUA_TLCF	(LUA_TFUNCTION | (1 << 4))\n"
    "#define LUA_TCCL	(LUA_TFUNCTION | (2 << 4))\n"
    "#define RAVI_TFCF	(LUA_TFUNCTION | (4 << 4))\n"
    "#define LUA_TSHRSTR	(LUA_TSTRING | (0 << 4))\n"
    "#define LUA_TLNGSTR	(LUA_TSTRING | (1 << 4))\n"
    "#define LUA_TNUMFLT	(LUA_TNUMBER | (0 << 4))\n"
    "#define LUA_TNUMINT	(LUA_TNUMBER | (1 << 4))\n"
    "#define RAVI_TIARRAY (LUA_TTABLE | (1 << 4))\n"
    "#define RAVI_TFARRAY (LUA_TTABLE | (2 << 4))\n"
    "#define BIT_ISCOLLECTABLE	(1 << 15)\n"
    "#define ctb(t)			((t) | BIT_ISCOLLECTABLE)\n"
    "typedef struct GCObject GCObject;\n"
    "#define CommonHeader	GCObject *next; lu_byte tt; lu_byte marked\n"
    "struct GCObject {\n"
    "  CommonHeader;\n"
    "};\n"
    "typedef union Value {\n"
    "  GCObject *gc;\n"
    "  void *p;\n"
    "  int b;\n"
    "  lua_CFunction f;\n"
    "  lua_Integer i;\n"
    "  lua_Number n;\n"
    "} Value;\n"
    "#define TValuefields	Value value_; LuaType tt_\n"
    "typedef struct lua_TValue {\n"
    "  TValuefields;\n"
    "} TValue;\n"
    "#define NILCONSTANT	{NULL}, LUA_TNIL\n"
    "#define val_(o)		((o)->value_)\n"
    "#define rttype(o)	((o)->tt_)\n"
    "#define novariant(x)	((x) & 0x0F)\n"
    "#define ttype(o)	(rttype(o) & 0x7F)\n"
    "#define ttnov(o)	(novariant(rttype(o)))\n"
    "#define checktag(o,t)		(rttype(o) == (t))\n"
    "#define checktype(o,t)		(ttnov(o) == (t))\n"
    "#define ttisnumber(o)		checktype((o), LUA_TNUMBER)\n"
    "#define ttisfloat(o)		checktag((o), LUA_TNUMFLT)\n"
    "#define ttisinteger(o)		checktag((o), LUA_TNUMINT)\n"
    "#define ttisnil(o)		checktag((o), LUA_TNIL)\n"
    "#define ttisboolean(o)		checktag((o), LUA_TBOOLEAN)\n"
    "#define ttislightuserdata(o)	checktag((o), LUA_TLIGHTUSERDATA)\n"
    "#define ttisstring(o)		checktype((o), LUA_TSTRING)\n"
    "#define ttisshrstring(o)	checktag((o), ctb(LUA_TSHRSTR))\n"
    "#define ttislngstring(o)	checktag((o), ctb(LUA_TLNGSTR))\n"
    "#define ttistable(o)		checktype((o), LUA_TTABLE)\n"
    "#define ttisiarray(o)    checktag((o), ctb(RAVI_TIARRAY))\n"
    "#define ttisfarray(o)    checktag((o), ctb(RAVI_TFARRAY))\n"
    "#define ttisarray(o)     (ttisiarray(o) || ttisfarray(o))\n"
    "#define ttisLtable(o)    checktag((o), ctb(LUA_TTABLE))\n"
    "#define ttisfunction(o)		checktype(o, LUA_TFUNCTION)\n"
    "#define ttisclosure(o)		((rttype(o) & 0x1F) == LUA_TFUNCTION)\n"
    "#define ttisCclosure(o)		checktag((o), ctb(LUA_TCCL))\n"
    "#define ttisLclosure(o)		checktag((o), ctb(LUA_TLCL))\n"
    "#define ttislcf(o)		checktag((o), LUA_TLCF)\n"
    "#define ttisfcf(o) (ttype(o) == RAVI_TFCF)\n"
    "#define ttisfulluserdata(o)	checktag((o), ctb(LUA_TUSERDATA))\n"
    "#define ttisthread(o)		checktag((o), ctb(LUA_TTHREAD))\n"
    "#define ttisdeadkey(o)		checktag((o), LUA_TDEADKEY)\n"
    "#define ivalue(o)	check_exp(ttisinteger(o), val_(o).i)\n"
    "#define fltvalue(o)	check_exp(ttisfloat(o), val_(o).n)\n"
    "#define nvalue(o)	check_exp(ttisnumber(o), \\\n"
    "	(ttisinteger(o) ? cast_num(ivalue(o)) : fltvalue(o)))\n"
    "#define gcvalue(o)	check_exp(iscollectable(o), val_(o).gc)\n"
    "#define pvalue(o)	check_exp(ttislightuserdata(o), val_(o).p)\n"
    "#define tsvalue(o)	check_exp(ttisstring(o), gco2ts(val_(o).gc))\n"
    "#define uvalue(o)	check_exp(ttisfulluserdata(o), gco2u(val_(o).gc))\n"
    "#define clvalue(o)	check_exp(ttisclosure(o), gco2cl(val_(o).gc))\n"
    "#define clLvalue(o)	check_exp(ttisLclosure(o), gco2lcl(val_(o).gc))\n"
    "#define clCvalue(o)	check_exp(ttisCclosure(o), gco2ccl(val_(o).gc))\n"
    "#define fvalue(o)	check_exp(ttislcf(o), val_(o).f)\n"
    "#define fcfvalue(o) check_exp(ttisfcf(o), val_(o).p)\n"
    "#define hvalue(o)	check_exp(ttistable(o), gco2t(val_(o).gc))\n"
    "#define arrvalue(o) check_exp(ttisarray(o), gco2array(val_(o).gc))\n"
    "#define arrvalue(o) check_exp(ttisarray(o), gco2array(val_(o).gc))\n"
    "#define bvalue(o)	check_exp(ttisboolean(o), val_(o).b)\n"
    "#define thvalue(o)	check_exp(ttisthread(o), gco2th(val_(o).gc))\n"
    "#define deadvalue(o)	check_exp(ttisdeadkey(o), cast(void *, val_(o).gc))\n"
    "#define l_isfalse(o)	(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))\n"
    "#define iscollectable(o)	(rttype(o) & BIT_ISCOLLECTABLE)\n"
    "#define righttt(obj)		(ttype(obj) == gcvalue(obj)->tt)\n"
    "#define checkliveness(L,obj) \\\n"
    "	lua_longassert(!iscollectable(obj) || \\\n"
    "		(righttt(obj) && (L == NULL || !isdead(G(L),gcvalue(obj)))))\n"
    "#define settt_(o,t)	((o)->tt_=(t))\n"
    "#define setfltvalue(obj,x) \\\n"
    "  { TValue *io=(obj); val_(io).n=(x); settt_(io, LUA_TNUMFLT); }\n"
    "#define chgfltvalue(obj,x) \\\n"
    "  { TValue *io=(obj); lua_assert(ttisfloat(io)); val_(io).n=(x); }\n"
    "#define setivalue(obj,x) \\\n"
    "  { TValue *io=(obj); val_(io).i=(x); settt_(io, LUA_TNUMINT); }\n"
    "#define chgivalue(obj,x) \\\n"
    "  { TValue *io=(obj); lua_assert(ttisinteger(io)); val_(io).i=(x); }\n"
    "#define setnilvalue(obj) settt_(obj, LUA_TNIL)\n"
    "#define setfvalue(obj,x) \\\n"
    "  { TValue *io=(obj); val_(io).f=(x); settt_(io, LUA_TLCF); }\n"
    "#define setfvalue_fastcall(obj, x, tag) \\\n"
    "{ \\\n"
    "    TValue *io = (obj);   \\\n"
    "    lua_assert(tag >= 1 && tag < 0x80); \\\n"
    "    val_(io).p = (x);     \\\n"
    "    settt_(io, ((tag << 8) | RAVI_TFCF)); \\\n"
    "}\n"
    "#define setpvalue(obj,x) \\\n"
    "  { TValue *io=(obj); val_(io).p=(x); settt_(io, LUA_TLIGHTUSERDATA); }\n"
    "#define setbvalue(obj,x) \\\n"
    "  { TValue *io=(obj); val_(io).b=(x); settt_(io, LUA_TBOOLEAN); }\n"
    "#define setgcovalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); GCObject *i_g=(x); \\\n"
    "    val_(io).gc = i_g; settt_(io, ctb(i_g->tt)); }\n"
    "#define setsvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); TString *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(x_->tt)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define setuvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); Udata *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TUSERDATA)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define setthvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); lua_State *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTHREAD)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define setclLvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); LClosure *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TLCL)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define setclCvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); CClosure *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TCCL)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define sethvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); Table *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTABLE)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define setiarrayvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); Table *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(RAVI_TIARRAY)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define setfarrayvalue(L,obj,x) \\\n"
    "  { TValue *io = (obj); Table *x_ = (x); \\\n"
    "    val_(io).gc = obj2gco(x_); settt_(io, ctb(RAVI_TFARRAY)); \\\n"
    "    checkliveness(L,io); }\n"
    "#define setdeadvalue(obj)	settt_(obj, LUA_TDEADKEY)\n"
    "#define setobj(L,obj1,obj2) \\\n"
    // NOTE we cannot use aggregate assign so following assigns by field but assumes
    // n covers all value types
    "	{ TValue *io1=(obj1); const TValue *io2=(obj2); io1->tt_ = io2->tt_; val_(io1).n = val_(io2).n; \\\n"
    "	  (void)L; checkliveness(L,io1); }\n"
    "#define setobjs2s	setobj\n"
    "#define setobj2s	setobj\n"
    "#define setsvalue2s	setsvalue\n"
    "#define sethvalue2s	sethvalue\n"
    "#define setptvalue2s	setptvalue\n"
    "#define setobjt2t	setobj\n"
    "#define setobj2n	setobj\n"
    "#define setsvalue2n	setsvalue\n"
    "#define setobj2t	setobj\n"
    "typedef TValue *StkId;\n"
    "typedef struct TString {\n"
    "	CommonHeader;\n"
    "	lu_byte extra;\n"
    "	lu_byte shrlen;\n"
    "	unsigned int hash;\n"
    "	union {\n"
    "		size_t lnglen;\n"
    "		struct TString *hnext;\n"
    "	} u;\n"
    "} TString;\n"
    "typedef union UTString {\n"
    "	L_Umaxalign dummy;\n"
    "	TString tsv;\n"
    "} UTString;\n"
    "#define getstr(ts)  \\\n"
    "  check_exp(sizeof((ts)->extra), cast(char *, (ts)) + sizeof(UTString))\n"
    "#define svalue(o)       getstr(tsvalue(o))\n"
    "#define tsslen(s)	((s)->tt == LUA_TSHRSTR ? (s)->shrlen : (s)->u.lnglen)\n"
    "#define vslen(o)	tsslen(tsvalue(o))\n"
    "typedef struct Udata {\n"
    "	CommonHeader;\n"
    "	LuaType ttuv_;\n"
    "	struct Table *metatable;\n"
    "	size_t len;\n"
    "	union Value user_;\n"
    "} Udata;\n"
    "typedef union UUdata {\n"
    "	L_Umaxalign dummy;\n"
    "	Udata uv;\n"
    "} UUdata;\n"
    "#define getudatamem(u)  \\\n"
    "  check_exp(sizeof((u)->ttuv_), (cast(char*, (u)) + sizeof(UUdata)))\n"
    "#define setuservalue(L,u,o) \\\n"
    "	{ const TValue *io=(o); Udata *iu = (u); \\\n"
    "	  iu->user_ = io->value_; iu->ttuv_ = rttype(io); \\\n"
    "	  checkliveness(L,io); }\n"
    "#define getuservalue(L,u,o) \\\n"
    "	{ TValue *io=(o); const Udata *iu = (u); \\\n"
    "	  io->value_ = iu->user_; settt_(io, iu->ttuv_); \\\n"
    "	  checkliveness(L,io); }\n"
    "typedef enum {\n"
    "	RAVI_TANY = 0,\n"
    "	RAVI_TNUMINT = 1,\n"
    "	RAVI_TNUMFLT,\n"
    "	RAVI_TARRAYINT,\n"
    "	RAVI_TARRAYFLT,\n"
    "	RAVI_TFUNCTION,\n"
    "	RAVI_TTABLE,\n"
    "	RAVI_TSTRING,\n"
    "	RAVI_TNIL,\n"
    "	RAVI_TBOOLEAN,\n"
    "	RAVI_TUSERDATA\n"
    "} ravitype_t;\n"
    "typedef struct Upvaldesc {\n"
    "	TString *name;\n"
    "	TString *usertype;\n"
    "	lu_byte ravi_type;\n"
    "	lu_byte instack;\n"
    "	lu_byte idx;\n"
    "} Upvaldesc;\n"
    "typedef struct LocVar {\n"
    "	TString *varname;\n"
    "	TString *usertype;\n"
    "	int startpc;\n"
    "	int endpc;\n"
    "	lu_byte ravi_type;\n"
    "} LocVar;\n"
    "typedef enum {\n"
    "	RAVI_JIT_NOT_COMPILED = 0,\n"
    "	RAVI_JIT_CANT_COMPILE = 1,\n"
    "	RAVI_JIT_COMPILED = 2\n"
    "} ravi_jit_status_t;\n"
    "typedef enum {\n"
    "	RAVI_JIT_FLAG_NONE = 0,\n"
    "	RAVI_JIT_FLAG_HASFORLOOP = 1\n"
    "} ravi_jit_flag_t;\n"
    "typedef struct RaviJITProto {\n"
    "	lu_byte jit_status;\n"
    "	lu_byte jit_flags;\n"
    "	unsigned short execution_count;\n"
    "	void *jit_data;\n"
    "	lua_CFunction jit_function;\n"
    "} RaviJITProto;\n"
    "typedef struct Proto {\n"
    "	CommonHeader;\n"
    "	lu_byte numparams;\n"
    "	lu_byte is_vararg;\n"
    "	lu_byte maxstacksize;\n"
    "	int sizeupvalues;\n"
    "	int sizek;\n"
    "	int sizecode;\n"
    "	int sizelineinfo;\n"
    "	int sizep;\n"
    "	int sizelocvars;\n"
    "	int linedefined;\n"
    "	int lastlinedefined;\n"
    "	TValue *k;\n"
    "	Instruction *code;\n"
    "	struct Proto **p;\n"
    "	int *lineinfo;\n"
    "	LocVar *locvars;\n"
    "	Upvaldesc *upvalues;\n"
    "	struct LClosure *cache;\n"
    "	TString  *source;\n"
    "	GCObject *gclist;\n"
    "	RaviJITProto ravi_jit;\n"
    "} Proto;\n"
    "typedef struct UpVal UpVal;\n"
    "#define ClosureHeader \\\n"
    "	CommonHeader; lu_byte nupvalues; GCObject *gclist\n"
    "typedef struct CClosure {\n"
    "	ClosureHeader;\n"
    "	lua_CFunction f;\n"
    "	TValue upvalue[1];\n"
    "} CClosure;\n"
    "typedef struct LClosure {\n"
    "	ClosureHeader;\n"
    "	struct Proto *p;\n"
    "	UpVal *upvals[1];\n"
    "} LClosure;\n"
    "typedef union Closure {\n"
    "	CClosure c;\n"
    "	LClosure l;\n"
    "} Closure;\n"
    "#define isLfunction(o)	ttisLclosure(o)\n"
    "#define getproto(o)	(clLvalue(o)->p)\n"
    "typedef union TKey {\n"
    "	struct {\n"
    "		TValuefields;\n"
    "		int next;\n"
    "	} nk;\n"
    "	TValue tvk;\n"
    "} TKey;\n"
    "#define setnodekey(L,key,obj) \\\n"
    "	{ TKey *k_=(key); const TValue *io_=(obj); \\\n"
    "	  k_->nk.value_ = io_->value_; k_->nk.tt_ = io_->tt_; \\\n"
    "	  (void)L; checkliveness(L,io_); }\n"
    "typedef struct Node {\n"
    "	TValue i_val;\n"
    "	TKey i_key;\n"
    "} Node;\n"
    "typedef enum RaviArrayModifer {\n"
    " RAVI_ARRAY_SLICE = 1,\n"
    " RAVI_ARRAY_FIXEDSIZE = 2,\n"
    " RAVI_ARRAY_ALLOCATED = 4,\n"
    " RAVI_ARRAY_ISFLOAT = 8\n"
    "} RaviArrayModifier;\n"
    "enum {\n"
    " RAVI_ARRAY_MAX_INLINE = 3,\n"
    "};\n"
    "typedef struct RaviArray {\n"
    " CommonHeader;\n"
    " lu_byte flags;\n"
    " unsigned int len;\n"
    " unsigned int size;\n"
    " union {\n"
    "  lua_Number numarray[RAVI_ARRAY_MAX_INLINE];\n"
    "  lua_Integer intarray[RAVI_ARRAY_MAX_INLINE];\n"
    "  struct RaviArray* parent;\n"
    " };\n"
    " char *data;\n"
    " struct Table *metatable;\n"
    "} RaviArray;\n"
    "typedef struct Table {\n"
    " CommonHeader;\n"
    " lu_byte flags;\n"
    " lu_byte lsizenode;\n"
    " unsigned int sizearray;\n"
    " TValue *array;\n"
    " Node *node;\n"
    " Node *lastfree;\n"
    " struct Table *metatable;\n"
    " GCObject *gclist;\n"
    " unsigned int hmask;\n"
    "} Table;\n"
    "typedef struct Mbuffer {\n"
    "	char *buffer;\n"
    "	size_t n;\n"
    "	size_t buffsize;\n"
    "} Mbuffer;\n"
    "typedef struct stringtable {\n"
    "	TString **hash;\n"
    "	int nuse;\n"
    "	int size;\n"
    "} stringtable;\n"
    "struct lua_Debug;\n"
    "typedef intptr_t lua_KContext;\n"
    "typedef int(*lua_KFunction)(struct lua_State *L, int status, lua_KContext ctx);\n"
    "typedef void *(*lua_Alloc)(void *ud, void *ptr, size_t osize,\n"
    "	size_t nsize);\n"
    "typedef void(*lua_Hook)(struct lua_State *L, struct lua_Debug *ar);\n"
    "typedef struct CallInfo {\n"
    "	StkId func;\n"
    "	StkId	top;\n"
    "	struct CallInfo *previous, *next;\n"
    "	union {\n"
    "		struct {\n"
    "			StkId base;\n"
    "			const Instruction *savedpc;\n"
    "		} l;\n"
    "		struct {\n"
    "			lua_KFunction k;\n"
    "			ptrdiff_t old_errfunc;\n"
    "			lua_KContext ctx;\n"
    "		} c;\n"
    "	} u;\n"
    "	ptrdiff_t extra;\n"
    "	short nresults;\n"
    "	unsigned short callstatus;\n"
    "	unsigned short stacklevel;\n"
    "	lu_byte jitstatus;\n"
    "   lu_byte magic;\n"
    "} CallInfo;\n"
    "#define CIST_OAH	(1<<0)\n"
    "#define CIST_LUA	(1<<1)\n"
    "#define CIST_HOOKED	(1<<2)\n"
    "#define CIST_FRESH	(1<<3)\n"
    "#define CIST_YPCALL	(1<<4)\n"
    "#define CIST_TAIL	(1<<5)\n"
    "#define CIST_HOOKYIELD	(1<<6)\n"
    "#define CIST_LEQ	(1<<7)\n"
    "#define CIST_FIN	(1<<8)\n"
    "#define isLua(ci)	((ci)->callstatus & CIST_LUA)\n"
    "#define isJITed(ci) ((ci)->jitstatus)\n"
    "#define setoah(st,v)	((st) = ((st) & ~CIST_OAH) | (v))\n"
    "#define getoah(st)	((st) & CIST_OAH)\n"
    "typedef struct global_State global_State;\n"
    "struct lua_State {\n"
    "	CommonHeader;\n"
    "	lu_byte status;\n"
    "	StkId top;\n"
    "	global_State *l_G;\n"
    "	CallInfo *ci;\n"
    "	const Instruction *oldpc;\n"
    "	StkId stack_last;\n"
    "	StkId stack;\n"
    "	UpVal *openupval;\n"
    "	GCObject *gclist;\n"
    "	struct lua_State *twups;\n"
    "	struct lua_longjmp *errorJmp;\n"
    "	CallInfo base_ci;\n"
    "	volatile lua_Hook hook;\n"
    "	ptrdiff_t errfunc;\n"
    "	int stacksize;\n"
    "	int basehookcount;\n"
    "	int hookcount;\n"
    "	unsigned short nny;\n"
    "	unsigned short nCcalls;\n"
    "	lu_byte hookmask;\n"
    "	lu_byte allowhook;\n"
    "	unsigned short nci;\n"
    "   lu_byte magic;\n"
    "};\n"
    "#define G(L)	(L->l_G)\n"
    "union GCUnion {\n"
    "	GCObject gc;\n"
    "	struct TString ts;\n"
    "	struct Udata u;\n"
    "	union Closure cl;\n"
    "	struct Table h;\n"
    "   struct RaviArray arr;\n"
    "	struct Proto p;\n"
    "	struct lua_State th;\n"
    "};\n"
    "struct UpVal {\n"
    "	TValue *v;\n"
#ifdef RAVI_DEFER_STATEMENT
    "       unsigned int refcount;\n"
    "       unsigned int flags;\n"
#else
    "	lu_mem refcount;\n"
#endif
    "	union {\n"
    "		struct {\n"
    "			UpVal *next;\n"
    "			int touched;\n"
    "		} open;\n"
    "		TValue value;\n"
    "	} u;\n"
    "};\n"
    "#define cast_u(o)	cast(union GCUnion *, (o))\n"
    "#define gco2ts(o)  \\\n"
    "	check_exp(novariant((o)->tt) == LUA_TSTRING, &((cast_u(o))->ts))\n"
    "#define gco2u(o)  check_exp((o)->tt == LUA_TUSERDATA, &((cast_u(o))->u))\n"
    "#define gco2lcl(o)  check_exp((o)->tt == LUA_TLCL, &((cast_u(o))->cl.l))\n"
    "#define gco2ccl(o)  check_exp((o)->tt == LUA_TCCL, &((cast_u(o))->cl.c))\n"
    "#define gco2cl(o)  \\\n"
    "	check_exp(novariant((o)->tt) == LUA_TFUNCTION, &((cast_u(o))->cl))\n"
    "#define gco2t(o)  check_exp((o)->tt == LUA_TTABLE, &((cast_u(o))->h))\n"
    "#define gco2array(o)  check_exp(((o)->tt == RAVI_TIARRAY || (o)->tt == RAVI_TFARRAY), &((cast_u(o))->arr))\n"
    "#define gco2p(o)  check_exp((o)->tt == LUA_TPROTO, &((cast_u(o))->p))\n"
    "#define gco2th(o)  check_exp((o)->tt == LUA_TTHREAD, &((cast_u(o))->th))\n"
    "#define obj2gco(v) \\\n"
    "	check_exp(novariant((v)->tt) < LUA_TDEADKEY, (&(cast_u(v)->gc)))\n"
    "#define LUA_FLOORN2I		0\n"
    "#define tonumber(o,n) \\\n"
    "  (ttisfloat(o) ? (*(n) = fltvalue(o), 1) : luaV_tonumber_(o,n))\n"
    "#define tointeger(o,i) \\\n"
    "  (ttisinteger(o) ? (*(i) = ivalue(o), 1) : luaV_tointeger(o,i,LUA_FLOORN2I))\n"
    "extern int luaV_tonumber_(const TValue *obj, lua_Number *n);\n"
    "extern int luaV_tointeger(const TValue *obj, lua_Integer *p, int mode);\n"
#ifdef RAVI_DEFER_STATEMENT
    "extern int luaF_close (lua_State *L, StkId level, int status);\n"
#else
    "extern void luaF_close (lua_State *L, StkId level);\n"
#endif
    "extern int luaD_poscall (lua_State *L, CallInfo *ci, StkId firstResult, int nres);\n"
    "extern void luaD_growstack (lua_State *L, int n);\n"
    "extern int luaV_equalobj(lua_State *L, const TValue *t1, const TValue *t2);\n"
    "extern int luaV_lessthan(lua_State *L, const TValue *l, const TValue *r);\n"
    "extern int luaV_lessequal(lua_State *L, const TValue *l, const TValue *r);\n"
    "extern void luaV_gettable (lua_State *L, const TValue *t, TValue *key, StkId val);\n"
    "extern void luaV_settable (lua_State *L, const TValue *t, TValue *key, StkId val);\n"
    "extern int luaV_execute(lua_State *L);\n"
    "extern int luaD_precall (lua_State *L, StkId func, int nresults, int op_call);\n"
    "extern void raviV_op_newtable(lua_State *L, CallInfo *ci, TValue *ra, int b, int c);\n"
    "extern void raviV_op_newarrayint(lua_State *L, CallInfo *ci, TValue *ra);\n"
    "extern void raviV_op_newarrayfloat(lua_State *L, CallInfo *ci, TValue *ra);\n"
    "extern void luaO_arith (lua_State *L, int op, const TValue *p1, const TValue *p2, TValue *res);\n"
    "extern void raviV_op_setlist(lua_State *L, CallInfo *ci, TValue *ra, int b, int c);\n"
    "extern void raviV_op_concat(lua_State *L, CallInfo *ci, int a, int b, int c);\n"
    "extern void raviV_op_closure(lua_State *L, CallInfo *ci, LClosure *cl, int a, int Bx);\n"
    "extern void raviV_op_vararg(lua_State *L, CallInfo *ci, LClosure *cl, int a, int b);\n"
    "extern void luaV_objlen (lua_State *L, StkId ra, const TValue *rb);\n"
    "extern int luaV_forlimit(const TValue *obj, lua_Integer *p, lua_Integer step, int *stopnow);\n"
    "extern void raviV_op_setupval(lua_State *L, LClosure *cl, TValue *ra, int b);\n"
    "extern void raviV_op_setupvali(lua_State *L, LClosure *cl, TValue *ra, int b);\n"
    "extern void raviV_op_setupvalf(lua_State *L, LClosure *cl, TValue *ra, int b);\n"
    "extern void raviV_op_setupvalai(lua_State *L, LClosure *cl, TValue *ra, int b);\n"
    "extern void raviV_op_setupvalaf(lua_State *L, LClosure *cl, TValue *ra, int b);\n"
    "extern void raviV_op_setupvalt(lua_State *L, LClosure *cl, TValue *ra, int b);\n"
    "extern void raise_error(lua_State *L, int errorcode);\n"
    "extern void raise_error_with_info(lua_State *L, int errorcode, const char *info);\n"
    "extern void luaD_call (lua_State *L, StkId func, int nResults);\n"
    "extern void raviH_set_int(lua_State *L, RaviArray *t, lua_Unsigned key, lua_Integer value);\n"
    "extern void raviH_set_float(lua_State *L, RaviArray *t, lua_Unsigned key, lua_Number value);\n"
    "extern int raviV_check_usertype(lua_State *L, TString *name, const TValue *o);\n"
    "extern void luaT_trybinTM (lua_State *L, const TValue *p1, const TValue *p2, TValue *res, TMS event);\n"
    "extern void raviV_gettable_sskey(lua_State *L, const TValue *t, TValue *key, TValue *val);\n"
    "extern void raviV_settable_sskey(lua_State *L, const TValue *t, TValue *key, TValue *val);\n"
    "extern void raviV_gettable_i(lua_State *L, const TValue *t, TValue *key, TValue *val);\n"
    "extern void raviV_settable_i(lua_State *L, const TValue *t, TValue *key, TValue *val);\n"
#ifdef RAVI_DEFER_STATEMENT
    "extern void raviV_op_defer(lua_State *L, TValue *ra);\n"
#endif
    "extern lua_Integer luaV_shiftl(lua_Integer x, lua_Integer y);\n"
    "extern void ravi_dump_value(lua_State *L, const struct lua_TValue *v);\n"
    "extern void raviV_op_bnot(lua_State *L, TValue *ra, TValue *rb);\n"
    "#define R(i) (base + i)\n"
    "#define K(i) (k + i)\n"
    "#define S(i) (stackbase + i)\n"
    "#define stackoverflow(L, n) (((int)(L->top - L->stack) + (n) + 5) >= L->stacksize)\n"
    "#define savestack(L,p)		((char *)(p) - (char *)L->stack)\n"
    "#define restorestack(L,n)	((TValue *)((char *)L->stack + (n)))\n"
    "#define tonumberns(o,n) \\\n"
    "	(ttisfloat(o) ? ((n) = fltvalue(o), 1) : \\\n"
    "	(ttisinteger(o) ? ((n) = cast_num(ivalue(o)), 1) : 0))\n"
    "#define intop(op,v1,v2) l_castU2S(l_castS2U(v1) op l_castS2U(v2))\n"
    "#define nan (0./0.)\n"
    "#define inf (1./0.)\n"
    "#define luai_numunm(L,a)        (-(a))\n";

struct function {
	struct proc *proc;
	buffer_t prologue;
	buffer_t body;
	struct Ravi_CompilerInterface *api;
};

/* readonly statics */
static const char *int_var_prefix = "i_";
static const char *flt_var_prefix = "f_";
// static struct pseudo NIL_pseudo = {.type = PSEUDO_NIL};

enum errorcode {
	Error_integer_expected,
	Error_number_expected,
	Error_integer_array_expected,
	Error_number_array_expected,
	Error_table_expected,
	Error_upval_needs_integer,
	Error_upval_needs_number,
	Error_upval_needs_integer_array,
	Error_upval_needs_number_array,
	Error_upval_needs_table,
	Error_for_limit_must_be_number,
	Error_for_step_must_be_number,
	Error_for_initial_value_must_be_number,
	Error_array_out_of_bounds,
	Error_string_expected,
	Error_closure_expected,
	Error_type_mismatch,
};

static inline struct pseudo *get_operand(struct instruction *insn, unsigned idx)
{
	return (struct pseudo *)ptrlist_nth_entry((struct ptr_list *)insn->operands, idx);
}

static inline struct pseudo *get_first_operand(struct instruction *insn)
{
	return (struct pseudo *)ptrlist_first((struct ptr_list *)insn->operands);
}

static inline struct pseudo *get_last_operand(struct instruction *insn)
{
	return (struct pseudo *)ptrlist_last((struct ptr_list *)insn->operands);
}

static inline struct pseudo *get_target(struct instruction *insn, unsigned idx)
{
	return (struct pseudo *)ptrlist_nth_entry((struct ptr_list *)insn->targets, idx);
}

static inline struct pseudo *get_first_target(struct instruction *insn)
{
	return (struct pseudo *)ptrlist_first((struct ptr_list *)insn->targets);
}

static inline struct pseudo *get_last_target(struct instruction *insn)
{
	return (struct pseudo *)ptrlist_last((struct ptr_list *)insn->targets);
}

static inline unsigned get_num_operands(struct instruction *insn)
{
	return ptrlist_size((const struct ptr_list *)insn->operands);
}

static inline unsigned get_num_targets(struct instruction *insn)
{
	return ptrlist_size((const struct ptr_list *)insn->targets);
}

/**
 * Helper to generate a list of primitive C variables representing temp int/float values.
 */
static void emit_vars(const char *type, const char *prefix, struct pseudo_generator *gen, buffer_t *mb)
{
	if (gen->next_reg == 0)
		return;
	for (unsigned i = 0; i < gen->next_reg; i++) {
		if (i == 0) {
			raviX_buffer_add_fstring(mb, "%s ", type);
		}
		if (i > 0) {
			raviX_buffer_add_string(mb, " = 0, ");
		}
		raviX_buffer_add_fstring(mb, "%s%d", prefix, i);
	}
	raviX_buffer_add_string(mb, " = 0;\n");
}

static void emit_varname(const struct pseudo *pseudo, buffer_t *mb)
{
	if (pseudo->type == PSEUDO_TEMP_INT) {
		raviX_buffer_add_fstring(mb, "%s%d", int_var_prefix, pseudo->regnum);
	} else if (pseudo->type == PSEUDO_TEMP_FLT) {
		raviX_buffer_add_fstring(mb, "%s%d", flt_var_prefix, pseudo->regnum);
	} else {
		fprintf(stderr, "Unexpected pseudo type %d\n", pseudo->type);
		assert(0);
	}
}

static void initfn(struct function *fn, struct proc *proc, struct Ravi_CompilerInterface *api)
{
	fn->proc = proc;
	fn->api = api;
	/* Set a name that can be used later to retrieve the compiled code */
	snprintf(proc->funcname, sizeof proc->funcname, "__ravifunc_%d", proc->id);
	raviX_buffer_init(&fn->prologue, 4096);
	raviX_buffer_init(&fn->body, 4096);
	raviX_buffer_add_fstring(&fn->prologue, "extern int %s(lua_State *L);\n", proc->funcname);
	raviX_buffer_add_fstring(&fn->prologue, "int %s(lua_State *L) {\n", proc->funcname);
	raviX_buffer_add_string(&fn->prologue, "int error_code = 0;\n");
	raviX_buffer_add_string(&fn->prologue, "int result = 0;\n");
	raviX_buffer_add_string(&fn->prologue, "CallInfo *ci = L->ci;\n");
	raviX_buffer_add_string(&fn->prologue, "LClosure *cl = clLvalue(ci->func);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue *k = cl->p->k;\n");
	raviX_buffer_add_string(&fn->prologue, "StkId base = ci->u.l.base;\n");
	emit_vars("lua_Integer", int_var_prefix, &proc->temp_int_pseudos, &fn->prologue);
	emit_vars("lua_Number", flt_var_prefix, &proc->temp_flt_pseudos, &fn->prologue);
	// Following are temp dummy regs
	// In ops like luaV_settable we may use up to two variables
	raviX_buffer_add_string(&fn->prologue, "TValue ival0; settt_(&ival0, LUA_TNUMINT);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue fval0; settt_(&fval0, LUA_TNUMFLT);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue bval0; settt_(&bval0, LUA_TBOOLEAN);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue ival1; settt_(&ival1, LUA_TNUMINT);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue fval1; settt_(&fval1, LUA_TNUMFLT);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue bval1; settt_(&bval1, LUA_TBOOLEAN);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue nilval; setnilvalue(&nilval);\n");
}

static void cleanup(struct function *fn)
{
	raviX_buffer_free(&fn->prologue);
	raviX_buffer_free(&fn->body);
}

static void emit_reload_base(struct function *fn) { raviX_buffer_add_string(&fn->body, "base = ci->u.l.base;\n"); }

static inline unsigned num_locals(struct proc *proc) { return proc->local_pseudos.next_reg; }

static inline unsigned num_temps(struct proc *proc) { return proc->temp_pseudos.next_reg; }

static unsigned compute_max_stack_size(struct proc *proc)
{
	/* max stack size is number of Lua vars plus any temps + some space */
	/* TODO this is probably incorrect */
	return num_locals(proc) + num_temps(proc);
}

/**
 * Computes the register offset from base. Input pseudo must be a local variable,
 * or temp register or range register (on Lua stack)
 */
static unsigned compute_register_from_base(struct function *fn, const struct pseudo *pseudo)
{
	switch (pseudo->type) {
	case PSEUDO_TEMP_ANY:
	case PSEUDO_RANGE: // Compute starting register
	case PSEUDO_RANGE_SELECT:
		// All temps start after the locals
		return pseudo->regnum + num_locals(fn->proc);
	case PSEUDO_SYMBOL:
		if (pseudo->symbol->symbol_type == SYM_LOCAL) {
			return pseudo->regnum;
		}
		// fallthrough
	default:
		assert(false);
		return (unsigned)-1;
	}
}

// Check if two pseudos point to the same register
// note we cannot easily check PSEUDO_LUASTACK type because there may
// be var args between CI->func and base. So stackbase may not be base-1 always.
static bool refers_to_same_register(struct function *fn, struct pseudo *src, struct pseudo *dst)
{
	static bool reg_pseudos[] = {
	    [PSEUDO_SYMBOL] = true,    /* An object of type lua_symbol representing local var or upvalue */
	    [PSEUDO_TEMP_FLT] = false, /* A floating point temp - may also be used for locals that don't escape */
	    [PSEUDO_TEMP_INT] = false, /* An integer temp - may also be used for locals that don't escape */
	    [PSEUDO_TEMP_ANY] = true,  /* A temp of any type - will always be on Lua stack */
	    [PSEUDO_CONSTANT] = false, /* A literal value */
	    [PSEUDO_PROC] = false,     /* A proc / function */
	    [PSEUDO_NIL] = false,
	    [PSEUDO_TRUE] = false,
	    [PSEUDO_FALSE] = false,
	    [PSEUDO_BLOCK] = false,	  /* Points to a basic block, used as targets for jumps */
	    [PSEUDO_RANGE] = true,	  /* Represents a range of registers from a certain starting register */
	    [PSEUDO_RANGE_SELECT] = true, /* Picks a certain register from a range */
	    /* TODO we need a type for var args */
	    [PSEUDO_LUASTACK] = true /* Specifies a Lua stack position - not used by linearizer - for use by codegen */
	};
	if (!reg_pseudos[src->type] || !reg_pseudos[dst->type])
		return false;
	if (src->type == PSEUDO_LUASTACK || dst->type == PSEUDO_LUASTACK) {
		return src->type == dst->type && src->stackidx == dst->stackidx;
	}
	if (src->type == PSEUDO_SYMBOL && dst->type != PSEUDO_SYMBOL)
		// a temp reg can never equate local reg
		return false;
	if (src->type == PSEUDO_SYMBOL && dst->type == PSEUDO_SYMBOL) {
		// up-values are not registers
		if (src->symbol->symbol_type != SYM_LOCAL || dst->symbol->symbol_type != SYM_LOCAL) {
			return false;
		}
	}
	return compute_register_from_base(fn, src) == compute_register_from_base(fn, dst);
}

/* 
Outputs accessor for a pseudo so that the accessor is always of type
TValue *. Thus for constants, we need to use a temp stack variable of type TValue.
The issue is what happens if we need two values at the same time and both are constants
of the same type. This is where the discriminator comes in - to help differentiate.
*/
static int emit_reg_accessor(struct function *fn, const struct pseudo *pseudo, unsigned discriminator)
{
	if (pseudo->type == PSEUDO_LUASTACK) {
		// Note pseudo->stackidx is relative to ci->func
		// But ci->func is not always base-1 because of var args
		// Therefore we need a different way to compute these
		raviX_buffer_add_fstring(&fn->body, "S(%d)", pseudo->stackidx);
	} else if (pseudo->type == PSEUDO_TEMP_ANY || pseudo->type == PSEUDO_RANGE ||
		   pseudo->type == PSEUDO_RANGE_SELECT) {
		// we put all temps on Lua stack after the locals
		raviX_buffer_add_fstring(&fn->body, "R(%d)", compute_register_from_base(fn, pseudo));
	} else if (pseudo->type == PSEUDO_SYMBOL) {
		if (pseudo->symbol->symbol_type == SYM_LOCAL) {
			raviX_buffer_add_fstring(&fn->body, "R(%d)", pseudo->regnum);
		} else if (pseudo->symbol->symbol_type == SYM_UPVALUE) {
			raviX_buffer_add_fstring(&fn->body, "cl->upvals[%d]->v", pseudo->regnum);
		} else {
			fn->api->error_message(fn->api->context, "Unexpected pseudo symbol type");
			assert(0);
			return -1;
		}
	} else if (pseudo->type == PSEUDO_CONSTANT) {
		if (pseudo->constant->type == RAVI_TSTRING) {
			/* TODO we discard const below as we need to update the string constant but this is not
			 * nice */
			unsigned k = fn->api->lua_newStringConstant(fn->api->context, (Proto *)fn->proc->userdata,
								    (struct string_object *)pseudo->constant->s);
			raviX_buffer_add_fstring(&fn->body, "K(%d)", k);
		} else if (pseudo->constant->type == RAVI_TNUMINT) {
			raviX_buffer_add_fstring(&fn->body, "&ival%u; ival%u.value_.i = %lld", discriminator, discriminator, pseudo->constant->i);
		} else if (pseudo->constant->type == RAVI_TNUMFLT) {
			raviX_buffer_add_fstring(&fn->body, "&fval%u; fval%u.value_.n = %g", discriminator, discriminator, pseudo->constant->n);
		} else if (pseudo->constant->type == RAVI_TNIL) {
			raviX_buffer_add_string(&fn->body, "&nilval");
		} else if (pseudo->constant->type == RAVI_TBOOLEAN) {
			raviX_buffer_add_fstring(&fn->body, "&bval%u; bval%u.value_.b = %d", discriminator, discriminator, (int)pseudo->constant->i);
		} else {
			fn->api->error_message(fn->api->context, "Unexpected pseudo constant type");
			assert(0);
			return -1;
		}
	} else if (pseudo->type == PSEUDO_TEMP_FLT) {
		raviX_buffer_add_fstring(&fn->body, "&fval%u; fval%u.value_.n = ", discriminator, discriminator);
		emit_varname(pseudo, &fn->body);
	} else if (pseudo->type == PSEUDO_TEMP_INT) {
		raviX_buffer_add_fstring(&fn->body, "&ival%u; ival%u.value_.i = ", discriminator, discriminator);
		emit_varname(pseudo, &fn->body);
	} else if (pseudo->type == PSEUDO_NIL) {
		raviX_buffer_add_string(&fn->body, "&nilval");
	} else if (pseudo->type == PSEUDO_TRUE) {
		raviX_buffer_add_fstring(&fn->body, "&bval%u; bval%u.value_.b = 1", discriminator, discriminator);
	} else if (pseudo->type == PSEUDO_FALSE) {
		raviX_buffer_add_fstring(&fn->body, "&bval%u; bval%u.value_.b = 0", discriminator, discriminator);
	} else {
		fn->api->error_message(fn->api->context, "Unexpected pseudo type");
		assert(0);
		return -1;
	}
	return 0;
}

/*copy floating point value to a temporary float */
static int emit_move_flttemp(struct function *fn, struct pseudo *src, struct pseudo *dst)
{
	if (src->type == PSEUDO_CONSTANT) {
		if (src->constant->type == RAVI_TNUMFLT) {
			emit_varname(dst, &fn->body);
			raviX_buffer_add_fstring(&fn->body, " = %.16g;\n", src->constant->n);
		} else {
			// FIXME can we have int value?
			assert(0);
			return -1;
		}
	} else if (src->type == PSEUDO_TEMP_FLT) {
		emit_varname(dst, &fn->body);
		raviX_buffer_add_string(&fn->body, " = ");
		emit_varname(src, &fn->body);
		raviX_buffer_add_string(&fn->body, ";\n");
	} else if (src->type == PSEUDO_LUASTACK || src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL) {
		raviX_buffer_add_string(&fn->body, "{\nTValue *reg = ");
		emit_reg_accessor(fn, src, 0);
		raviX_buffer_add_string(&fn->body, ";\n");
		emit_varname(dst, &fn->body);
		raviX_buffer_add_string(&fn->body, " = fltvalue(reg);\n}\n");
	} else {
		assert(0);
		return -1;
	}
	return 0;
}

/*copy integer value to temporary int */
static int emit_move_inttemp(struct function *fn, struct pseudo *src, struct pseudo *dst)
{
	if (src->type == PSEUDO_CONSTANT) {
		if (src->constant->type == RAVI_TNUMINT) {
			emit_varname(dst, &fn->body);
			raviX_buffer_add_fstring(&fn->body, " = %lld;\n", src->constant->i);
		} else {
			// FIXME can we have float value?
			assert(0);
			return -1;
		}
	} else if (src->type == PSEUDO_TEMP_INT) {
		emit_varname(dst, &fn->body);
		raviX_buffer_add_string(&fn->body, " = ");
		emit_varname(src, &fn->body);
		raviX_buffer_add_string(&fn->body, ";\n");
	} else if (src->type == PSEUDO_LUASTACK || src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL) {
		raviX_buffer_add_string(&fn->body, "{\nTValue *reg = ");
		emit_reg_accessor(fn, src, 0);
		raviX_buffer_add_string(&fn->body, ";\n");
		emit_varname(dst, &fn->body);
		raviX_buffer_add_string(&fn->body, " = ivalue(reg);\n}\n");
	} else {
		assert(0);
		return -1;
	}
	return 0;
}

/* copy a value from source pseudo to destination pseudo.*/
static int emit_move(struct function *fn, struct pseudo *src, struct pseudo *dst)
{
	if (dst->type == PSEUDO_TEMP_FLT) {
		emit_move_flttemp(fn, src, dst);
	} else if (dst->type == PSEUDO_TEMP_INT) {
		emit_move_inttemp(fn, src, dst);
	} else if (dst->type == PSEUDO_TEMP_ANY || dst->type == PSEUDO_SYMBOL || dst->type == PSEUDO_LUASTACK) {
		if (src->type == PSEUDO_LUASTACK || src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL || src->type == PSEUDO_RANGE_SELECT) {
			// Only emit a move if we are not referencing the same register
			if (!refers_to_same_register(fn, src, dst)) {
				raviX_buffer_add_string(&fn->body, "{\nconst TValue *src_reg = ");
				emit_reg_accessor(fn, src, 0);
				raviX_buffer_add_string(&fn->body, ";\nTValue *dst_reg = ");
				emit_reg_accessor(fn, dst, 0);
				// FIXME - check value assignment approach
				raviX_buffer_add_string(
				    &fn->body,
				    ";\ndst_reg->tt_ = src_reg->tt_;\ndst_reg->value_.n = src_reg->value_.n;\n}\n");
			}
		} else if (src->type == PSEUDO_TEMP_INT) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst, 0);
			raviX_buffer_add_string(&fn->body, ";\nsetivalue(dst_reg, ");
			emit_varname(src, &fn->body);
			raviX_buffer_add_string(&fn->body, ");\n}\n");
		} else if (src->type == PSEUDO_TEMP_FLT) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst, 0);
			raviX_buffer_add_string(&fn->body, ";\nsetfltvalue(dst_reg, ");
			emit_varname(src, &fn->body);
			raviX_buffer_add_string(&fn->body, ");\n}\n");
		} else if (src->type == PSEUDO_TRUE || src->type == PSEUDO_FALSE) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst, 0);
			raviX_buffer_add_fstring(&fn->body, ";\nsetbvalue(dst_reg, %d);\n}\n",
						 src->type == PSEUDO_TRUE ? 1 : 0);
		} else if (src->type == PSEUDO_NIL) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst, 0);
			raviX_buffer_add_string(&fn->body, ";\nsetnilvalue(dst_reg);\n}\n");
		} else if (src->type == PSEUDO_CONSTANT) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst, 0);
			raviX_buffer_add_string(&fn->body, ";\n");
			if (src->constant->type == RAVI_TNUMINT) {
				raviX_buffer_add_fstring(&fn->body, "setivalue(dst_reg, %lld);\n", src->constant->i);
			} else if (src->constant->type == RAVI_TNUMFLT) {
				raviX_buffer_add_fstring(&fn->body, "setfltvalue(dst_reg, %g);\n", src->constant->n);
			} else if (src->constant->type == RAVI_TBOOLEAN) {
				raviX_buffer_add_fstring(&fn->body, "setbvalue(dst_reg, %i);\n", (int)src->constant->i);
			} else if (src->constant->type == RAVI_TNIL) {
				raviX_buffer_add_string(&fn->body, "setnilvalue(dst_reg);\n");
			} else if (src->constant->type == RAVI_TSTRING) {
				raviX_buffer_add_string(&fn->body, "TValue *src_reg = ");
				emit_reg_accessor(fn, src, 0);
				raviX_buffer_add_string(&fn->body, ";\n");
				raviX_buffer_add_string(
				    &fn->body,
				    "dst_reg->tt_ = src_reg->tt_; dst_reg->value_.gc = src_reg->value_.gc;\n");
			} else {
				assert(0);
				return -1;
			}
			raviX_buffer_add_string(&fn->body, "}\n");
		} else {
			/* range pseudos not supported yet */
			assert(0);
			return -1;
		}
	} else {
		assert(0);
		return -1;
	}
	return 0;
}

static int emit_jump(struct function *fn, struct pseudo *pseudo)
{
	assert(pseudo->type == PSEUDO_BLOCK);
	raviX_buffer_add_fstring(&fn->body, "goto L%d;\n", pseudo->block->index);
	return 0;
}

static int emit_op_cbr(struct function *fn, struct instruction *insn)
{
	assert(insn->opcode == op_cbr);
	struct pseudo *cond_pseudo = get_operand(insn, 0);
	if (cond_pseudo->type == PSEUDO_FALSE || cond_pseudo->type == PSEUDO_NIL) {
		emit_jump(fn, get_target(insn, 1));
	} else if (cond_pseudo->type == PSEUDO_TRUE || cond_pseudo->type == PSEUDO_CONSTANT) {
		emit_jump(fn, get_target(insn, 0));
	} else if (cond_pseudo->type == PSEUDO_TEMP_FLT || cond_pseudo->type == PSEUDO_TEMP_INT) {
		raviX_buffer_add_string(&fn->body, "{");
		raviX_buffer_add_string(&fn->body, " if (");
		emit_varname(cond_pseudo, &fn->body);
		raviX_buffer_add_fstring(&fn->body, " != 0) goto L%d;", get_target(insn, 0)->block->index);
		raviX_buffer_add_fstring(&fn->body, " else goto L%d; ", get_target(insn, 1)->block->index);
		raviX_buffer_add_string(&fn->body, "}\n");
	} else if (cond_pseudo->type == PSEUDO_TEMP_ANY || cond_pseudo->type == PSEUDO_SYMBOL) {
		raviX_buffer_add_string(&fn->body, "{\nconst TValue *src_reg = ");
		emit_reg_accessor(fn, cond_pseudo, 0);
		raviX_buffer_add_fstring(&fn->body, ";\nif (!l_isfalse(src_reg)) goto L%d;\n",
					 get_target(insn, 0)->block->index);
		raviX_buffer_add_fstring(&fn->body, "else goto L%d;\n", get_target(insn, 1)->block->index);
		raviX_buffer_add_string(&fn->body, "}\n");
	} else {
		assert(0);
		return -1;
	}
	return 0;
}

static int emit_op_br(struct function *fn, struct instruction *insn)
{
	assert(insn->opcode == op_br);
	return emit_jump(fn, get_target(insn, 0));
}

static int emit_op_mov(struct function *fn, struct instruction *insn)
{
	assert(insn->opcode == op_mov || insn->opcode == op_movi || insn->opcode == op_movf);
	return emit_move(fn, get_operand(insn, 0), get_target(insn, 0));
}

static int emit_op_ret(struct function *fn, struct instruction *insn)
{
	// TODO Only call luaF_close if needed (i.e. some variable escaped)
#ifdef RAVI_DEFER_STATEMENT
	if (ptrlist_size((const struct ptr_list *)fn->proc->procs) > 0) {
		raviX_buffer_add_string(&fn->body, "{\nluaF_close(L, base, LUA_OK);\n");
		raviX_buffer_add_string(&fn->body, "base = ci->u.l.base;\n");
		raviX_buffer_add_string(&fn->body, "}\n");
	}
#else
	if (ptrlist_size((const struct ptr_list *)fn->proc->procs) > 0) {
		raviX_buffer_add_string(&fn->body, "luaF_close(L, base);\n");
	}
#endif
	raviX_buffer_add_string(&fn->body, "{\n");
	/* Results are copied to stack position given by ci->func and above.
	 * stackbase is set here so S(n) refers to (stackbase+n)
	 */
	raviX_buffer_add_string(&fn->body, " TValue *stackbase = ci->func;\n");
	raviX_buffer_add_string(&fn->body, " int wanted = ci->nresults;\n");
	raviX_buffer_add_string(&fn->body, " result = wanted == -1 ? 0 : 1;\n"); /* see OP_RETURN impl in JIT */
	int n = get_num_operands(insn);
	if (n > 0) {
		struct pseudo *last_operand = get_operand(insn, n - 1);
		/* the last operand might be a range pseudo */
		if (last_operand->type == PSEUDO_RANGE) {
			raviX_buffer_add_string(&fn->body, " if (wanted == -1) {\n");
			raviX_buffer_add_string(&fn->body, "  TValue *start_vararg = ");
			struct pseudo tmp = {.type = PSEUDO_TEMP_ANY, .regnum = last_operand->regnum};
			emit_reg_accessor(fn, &tmp, 0);
			raviX_buffer_add_string(&fn->body, " ;\n");
			raviX_buffer_add_fstring(&fn->body, "  wanted = (L->top - start_vararg) + %d;\n", n - 1);
			raviX_buffer_add_string(&fn->body, " }\n");
		} else {
			raviX_buffer_add_fstring(&fn->body, " if (wanted == -1) wanted = %d;\n", n);
		}
	} else {
		raviX_buffer_add_string(&fn->body, " if (wanted == -1) wanted = 0;\n");
	}
	struct pseudo *pseudo;
	int i = 0;
	raviX_buffer_add_string(&fn->body, " int j = 0;\n");
	FOR_EACH_PTR(insn->operands, pseudo)
	{
		if (pseudo->type != PSEUDO_RANGE) {
			struct pseudo dummy_dest = {.type = PSEUDO_LUASTACK,
						    .stackidx = i}; /* will go to stackbase[i] */
			raviX_buffer_add_fstring(&fn->body, " if (%d < wanted) {\n", i);
			/* FIXME last argument might be a range pseudo */
			emit_move(fn, pseudo, &dummy_dest);
			raviX_buffer_add_string(&fn->body, " }\n");
			raviX_buffer_add_fstring(&fn->body, " j++;\n");
			i++;
		} else {
			/* copy values starting at the range to L->top */
			// raviX_buffer_add_fstring(&fn->body, " j = %d;\n", i);
			raviX_buffer_add_fstring(&fn->body, " {\n int reg = %d;\n", pseudo->regnum);
			raviX_buffer_add_string(&fn->body, "  while (j < wanted) {\n");
			raviX_buffer_add_string(&fn->body, "   TValue *dest_reg = S(j);\n");
			raviX_buffer_add_string(&fn->body, "   TValue *src_reg = R(reg);\n");
			raviX_buffer_add_string(
			    &fn->body, "   dest_reg->tt_ = src_reg->tt_; dest_reg->value_.gc = src_reg->value_.gc;\n");
			raviX_buffer_add_string(&fn->body, "   j++, reg++;\n");
			raviX_buffer_add_string(&fn->body, "  }\n");
			raviX_buffer_add_string(&fn->body, " }\n");
		}
	}
	END_FOR_EACH_PTR(pseudo);
	/* Set any excess results to nil */
	raviX_buffer_add_string(&fn->body, " while (j < wanted) {\n");
	{
		raviX_buffer_add_string(&fn->body, "  setnilvalue(S(j));\n");
		raviX_buffer_add_string(&fn->body, "  j++;\n");
	}
	raviX_buffer_add_string(&fn->body, " }\n");
	/* FIXME the rule for L->top needs to be checked */
	raviX_buffer_add_string(&fn->body, " L->top = S(0) + wanted;\n");
	raviX_buffer_add_string(&fn->body, " L->ci = ci->previous;\n");
	raviX_buffer_add_string(&fn->body, "}\n");
	emit_jump(fn, get_target(insn, 0));
	return 0;
}

static int emit_op_loadglobal(struct function *fn, struct instruction *insn)
{
	const char *fname = "luaV_gettable";
	if (insn->opcode == op_tget_ikey) {
		fname = "raviV_gettable_i";
	} else if (insn->opcode == op_tget_skey) {
		fname = "raviV_gettable_sskey";
	}
	struct pseudo *env = get_operand(insn, 0);
	struct pseudo *varname = get_operand(insn, 1);
	struct pseudo *dst = get_target(insn, 0);
	if (varname->type == PSEUDO_CONSTANT && varname->constant->type == RAVI_TSTRING) {
		if (varname->constant->s->len < 40) {
			fname = "raviV_gettable_sskey";
		}
	}
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " TValue *tab = ");
	emit_reg_accessor(fn, env, 0);
	raviX_buffer_add_string(&fn->body, ";\n TValue *name = ");
	emit_reg_accessor(fn, varname, 0);
	raviX_buffer_add_string(&fn->body, ";\n TValue *dst = ");
	emit_reg_accessor(fn, dst, 1);
	raviX_buffer_add_fstring(&fn->body, ";\n %s(L, tab, name, dst);\n ", fname);
	emit_reload_base(fn);
	raviX_buffer_add_string(&fn->body, "}\n");
	return 0;
}

static int emit_op_storeglobal(struct function *fn, struct instruction *insn)
{
	// FIXME what happens if key and value are both constants
	// Our pseudo reg will break I think
	const char *fname = "luaV_settable";
	if (insn->opcode == op_tput_ikey) {
		fname = "raviV_settable_i";
	}
	else if (insn->opcode == op_tput_skey) {
		fname = "raviV_settable_sskey";
	}
	struct pseudo *env = get_target(insn, 0);
	struct pseudo *varname = get_target(insn, 1);
	struct pseudo *src = get_operand(insn, 0);
	if (varname->type == PSEUDO_CONSTANT && varname->constant->type == RAVI_TSTRING) {
		if (varname->constant->s->len < 40) {
			fname = "raviV_settable_sskey";
		}
	}
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " TValue *tab = ");
	emit_reg_accessor(fn, env, 0);
	raviX_buffer_add_string(&fn->body, ";\n TValue *name = ");
	emit_reg_accessor(fn, varname, 0);
	raviX_buffer_add_string(&fn->body, ";\n TValue *src = ");
	emit_reg_accessor(fn, src, 1);
	raviX_buffer_add_fstring(&fn->body, ";\n %s(L, tab, name, src);\n ", fname);
	emit_reload_base(fn);
	raviX_buffer_add_string(&fn->body, "}\n");
	return 0;
}

// From implementation point of view the main work is copy the registers to the
// right place. If we assume that at any time there is a 'fixed' stack size for the
// functions regular variables and temps and that when we call functions, we need
// to put the function followed by arguments on top of this 'fixed' stack.
// However the complication is that some of the arguments of the function may come
// from a previous function call and therefore may be occupying the same space!
// For example:
// local x = f()
// g(x, h())
// Here the return values from h() will be on the stack above the fixed stack space
// and g() expects x, followed by all the return values from h().
// But the nature of the byte code execution is that the return values of h()
// will be at the top of the fixed stack and will have offsets less than  the
// parameter positions of g() because when we call g() we will at least have the
// function value at the position of the first result from h(). Suppose the h() return values
// are at stack[10], stack[11], stack[12], etc.
// Then when we call g() we will put stack[10] = g, stack[11] = x,
// and stack[12] = stack[10], etc. To do this correctly we need to copy the
// last argument first.
static int emit_op_call(struct function *fn, struct instruction *insn)
{
	assert(get_num_targets(insn) == 2);
	unsigned int n = get_num_operands(insn);
	// target register is where results should end up after the call
	// so it also tells us where we need to place the new frame
	// Note that this is typically a range starting at a register
	unsigned target_register = get_target(insn, 0)->regnum;
	// Number of values expected by the caller
	// If -1 it means all available values
	int nresults = (int)get_target(insn, 1)->constant->i;
	// I think it is okay to just use n as the check because if L->top was set
	// then n will be on top of that
	raviX_buffer_add_fstring(
	    &fn->body, " if (stackoverflow(L,%d)) { luaD_growstack(L, %d); base = ci->u.l.base; }\n", n + 1, n + 1);
	if (n > 1) {
		// We have function arguments (as n=0 is the function itself)
		struct pseudo *last_arg = get_operand(insn, n - 1);
		if (last_arg->type == PSEUDO_RANGE) {
			// If last argument is a range that tells us that we need
			// to copy all available values from the register to L->top
			// But first check whether copy is necessary
			// suppose n = 2
			// then,
			// target_register[0] will have function
			// target_register[1] will have arg 1
			unsigned copy_to = target_register + n - 1;
			if (last_arg->regnum != copy_to) {
				raviX_buffer_add_string(&fn->body, "{\n");
				raviX_buffer_add_string(&fn->body, " TValue *src_base = ");
				emit_reg_accessor(fn, last_arg, 0);
				raviX_buffer_add_string(&fn->body, ";\n");
				raviX_buffer_add_string(&fn->body, " TValue *dest_base = ");
				struct pseudo tmp = {.type = PSEUDO_TEMP_ANY, .regnum = copy_to};
				emit_reg_accessor(fn, &tmp, 0);
				raviX_buffer_add_string(&fn->body, ";\n TValue *src = L->top-1;\n");
				raviX_buffer_add_string(&fn->body, " L->top = dest_base + (L->top-src_base);\n");
				raviX_buffer_add_string(&fn->body, " TValue *dest = L->top-1;\n");
				raviX_buffer_add_string(&fn->body, " while (src >= src_base) {\n");
				raviX_buffer_add_string(&fn->body,
							"  dest->tt_ = src->tt_; dest->value_.gc = src->value_.gc;\n");
				raviX_buffer_add_string(&fn->body, "  src--;\n");
				raviX_buffer_add_string(&fn->body, "  dest--;\n");
				raviX_buffer_add_string(&fn->body, " }\n");
				raviX_buffer_add_string(&fn->body, "}\n");
			} else {
				// L->top stays where it is ...
			}
			n--; // discard the last arg
		} else {
			// L->top must be just past the last arg
			raviX_buffer_add_string(&fn->body, " L->top = ");
			emit_reg_accessor(fn, get_target(insn, 0), 0);
			raviX_buffer_add_fstring(&fn->body, " + %d;\n", n);
		}
	}
	// Copy the rest of the args
	for (int j = n - 1; j >= 0; j--) {
		struct pseudo tmp = {.type = PSEUDO_TEMP_ANY, .regnum = target_register + j};
		emit_move(fn, get_operand(insn, j), &tmp);
	}
	// Call the function
	raviX_buffer_add_string(&fn->body, "{\n TValue *ra = ");
	emit_reg_accessor(fn, get_target(insn, 0), 0);
	raviX_buffer_add_fstring(&fn->body, ";\n int result = luaD_precall(L, ra, %d, 1);\n", nresults);
	raviX_buffer_add_string(&fn->body, " if (result) {\n");
	raviX_buffer_add_fstring(&fn->body, "  if (result == 1 && %d >= 0)\n", nresults);
	raviX_buffer_add_string(&fn->body, "   L->top = ci->top;\n");
	raviX_buffer_add_string(&fn->body, " }\n");
	raviX_buffer_add_string(&fn->body, " else {  /* Lua function */\n");
	raviX_buffer_add_string(&fn->body, "  result = luaV_execute(L);\n");
	raviX_buffer_add_string(&fn->body, "  if (result) L->top = ci->top;\n");
	raviX_buffer_add_string(&fn->body, " }\n");
	raviX_buffer_add_string(&fn->body, " base = ci->u.l.base;\n");
	raviX_buffer_add_string(&fn->body, "}\n");
	return 0;
}

/*
* Output a C stack variable representing int/float value or constant
*/
static void emit_varname_or_constant(struct function *fn, struct pseudo *pseudo)
{
	if (pseudo->type == PSEUDO_CONSTANT) {
		if (pseudo->constant->type == RAVI_TNUMINT) {
			raviX_buffer_add_fstring(&fn->body, "%lld", pseudo->constant->i);
		} else if (pseudo->constant->type == RAVI_TNUMFLT) {
			raviX_buffer_add_fstring(&fn->body, "%.16g", pseudo->constant->n);
		} else {
			assert(0);
		}
	} else if (pseudo->type == PSEUDO_TEMP_INT || pseudo->type == PSEUDO_TEMP_FLT) {
		emit_varname(pseudo, &fn->body);
	} else {
		assert(0);
	}
}

static int emit_comp_ii(struct function *fn, struct instruction *insn)
{
	raviX_buffer_add_string(&fn->body, "{ ");
	struct pseudo *target = get_target(insn, 0);
	if (target->type == PSEUDO_TEMP_FLT || target->type == PSEUDO_TEMP_INT) {
		emit_varname(target, &fn->body);
		raviX_buffer_add_string(&fn->body, " = ");
	} else {
		raviX_buffer_add_string(&fn->body, "TValue *dst_reg = ");
		emit_reg_accessor(fn, target, 0);
		raviX_buffer_add_string(&fn->body, "; setbvalue(dst_reg, ");
	}
	const char *oper = NULL;
	switch (insn->opcode) {
	case op_eqii:
	case op_eqff:
		oper = "==";
		break;
	case op_ltii:
	case op_ltff:
		oper = "<";
		break;
	case op_leii:
	case op_leff:
		oper = "<=";
		break;
	default:
		assert(0);
		return -1;
	}
	emit_varname_or_constant(fn, get_operand(insn, 0));
	raviX_buffer_add_fstring(&fn->body, " %s ", oper);
	emit_varname_or_constant(fn, get_operand(insn, 1));
	if (target->type == PSEUDO_TEMP_FLT || target->type == PSEUDO_TEMP_INT) {
		raviX_buffer_add_string(&fn->body, "; }\n");
	} else {
		raviX_buffer_add_string(&fn->body, "); }\n");
	}
	return 0;
}

static int emit_bin_ii(struct function *fn, struct instruction *insn)
{
	// FIXME - needs to also work with typed function params
	raviX_buffer_add_string(&fn->body, "{ ");
	struct pseudo *target = get_target(insn, 0);
	if (target->type == PSEUDO_TEMP_FLT || target->type == PSEUDO_TEMP_INT) {
		emit_varname(target, &fn->body);
		raviX_buffer_add_string(&fn->body, " = ");
	} else {
		raviX_buffer_add_string(&fn->body, "TValue *dst_reg = ");
		emit_reg_accessor(fn, target, 0);
		if (insn->opcode == op_addff || insn->opcode == op_subff || insn->opcode == op_mulff ||
		    insn->opcode == op_divff) {
			raviX_buffer_add_string(&fn->body, "; setfltvalue(dst_reg, ");
		} else {
			raviX_buffer_add_string(&fn->body, "; setivalue(dst_reg, ");
		}
	}
	const char *oper = NULL;
	switch (insn->opcode) {
	case op_addff:
	case op_addii:
		oper = "+";
		break;

	case op_subff:
	case op_subii:
		oper = "-";
		break;

	case op_mulff:
	case op_mulii:
		oper = "*";
		break;

	case op_divff:
	case op_divii:
		oper = "/";
		break;

	case op_bandii:
		oper = "&";
		break;

	case op_borii:
		oper = "|";
		break;

	case op_bxorii:
		oper = "^";
		break;
	default:
		assert(0);
		return -1;
	}
	emit_varname_or_constant(fn, get_operand(insn, 0));
	raviX_buffer_add_fstring(&fn->body, " %s ", oper);
	emit_varname_or_constant(fn, get_operand(insn, 1));
	if (target->type == PSEUDO_TEMP_FLT || target->type == PSEUDO_TEMP_INT) {
		raviX_buffer_add_string(&fn->body, "; }\n");
	} else {
		raviX_buffer_add_string(&fn->body, "); }\n");
	}
	return 0;
}

static int emit_op_arrayget_ikey(struct function *fn, struct instruction *insn)
{
	const char *array_type = insn->opcode == op_iaget_ikey ? "lua_Integer *" : "lua_Number *";
	const char *setterfunc = insn->opcode == op_iaget_ikey ? "setivalue" : "setfltvalue";
	unsigned type = insn->opcode == op_iaget_ikey ? PSEUDO_TEMP_INT : PSEUDO_TEMP_FLT;
	struct pseudo *arr = get_operand(insn, 0);
	struct pseudo *key = get_operand(insn, 1);
	struct pseudo *dst = get_target(insn, 0);
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " RaviArray *arr = arrvalue(");
	emit_reg_accessor(fn, arr, 0);
	raviX_buffer_add_string(&fn->body, ");\n lua_Unsigned ukey = (lua_Unsigned) ");
	if (key->type == PSEUDO_CONSTANT) {
		raviX_buffer_add_fstring(&fn->body, "%lld", key->constant->i);
	} else if (key->type == PSEUDO_TEMP_INT) {
		emit_varname(key, &fn->body);
	} else if (key->type == PSEUDO_SYMBOL) {
		// this must be an integer
		raviX_buffer_add_string(&fn->body, "ivalue(");
		emit_reg_accessor(fn, key, 0);
		raviX_buffer_add_string(&fn->body, ")");
	} else {
		assert(0);
		return -1;
	}
	raviX_buffer_add_string(&fn->body, ";\n");
	raviX_buffer_add_fstring(&fn->body, " %siptr = (%s)arr->data;\n ",
				 array_type, array_type);
	if (dst->type == type) {
		emit_varname(dst, &fn->body);
		raviX_buffer_add_string(&fn->body, " = iptr[ukey];\n");
	} else if (dst->type == PSEUDO_TEMP_ANY || dst->type == PSEUDO_SYMBOL || dst->type == PSEUDO_LUASTACK) {
		raviX_buffer_add_string(&fn->body, "TValue *dest_reg = ");
		emit_reg_accessor(fn, dst, 0);
		raviX_buffer_add_fstring(&fn->body, "; %s(dest_reg, iptr[ukey]);\n", setterfunc);
	} else {
		assert(0);
		return -1;
	}
	raviX_buffer_add_string(&fn->body, "}\n");
	return 0;
}

static int emit_op_arrayput_val(struct function *fn, struct instruction *insn)
{
	const char *array_type = insn->opcode == op_iaput_ival ? "lua_Integer *" : "lua_Number *";
	const char *getterfunc = insn->opcode == op_iaput_ival ? "ivalue" : "fltvalue";
	const char *setterfunc = insn->opcode == op_iaput_ival ? "raviH_set_int" : "raviH_set_float";
	unsigned type = insn->opcode == op_iaput_ival ? PSEUDO_TEMP_INT : PSEUDO_TEMP_FLT;
	struct pseudo *arr = get_target(insn, 0);
	struct pseudo *key = get_target(insn, 1);
	struct pseudo *src = get_operand(insn, 0);
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " RaviArray *arr = arrvalue(");
	emit_reg_accessor(fn, arr, 0);
	raviX_buffer_add_string(&fn->body, ");\n lua_Unsigned ukey = (lua_Unsigned) ");
	if (key->type == PSEUDO_CONSTANT) {
		raviX_buffer_add_fstring(&fn->body, "%lld", key->constant->i);
	} else if (key->type == PSEUDO_TEMP_INT) {
		emit_varname(key, &fn->body);
	} else if (key->type == PSEUDO_SYMBOL) {
		// this must be an integer
		raviX_buffer_add_string(&fn->body, "ivalue(");
		emit_reg_accessor(fn, key, 0);
		raviX_buffer_add_string(&fn->body, ")");
	} else {
		assert(0);
		return -1;
	}
	raviX_buffer_add_string(&fn->body, ";\n");
	raviX_buffer_add_fstring(&fn->body, " %siptr = (%s)arr->data;\n ", array_type, array_type);
	raviX_buffer_add_string(&fn->body, "if (ukey < (lua_Unsigned)(arr->len)) {\n");
	raviX_buffer_add_string(&fn->body, " iptr[ukey] = ");
	if (src->type == type) {
		emit_varname(src, &fn->body);
	} else if (src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL || src->type == PSEUDO_LUASTACK) {
		raviX_buffer_add_fstring(&fn->body, "%s(", getterfunc);
		emit_reg_accessor(fn, src, 0);
		raviX_buffer_add_string(&fn->body, ")");
	} else if (src->type == PSEUDO_CONSTANT) {
		if (src->constant->type == RAVI_TNUMINT) {
			raviX_buffer_add_fstring(&fn->body, "%lld", src->constant->i);
		}
		else {
			raviX_buffer_add_fstring(&fn->body, "%g", src->constant->n);
		}
	} else {
		assert(0);
		return -1;
	}
	raviX_buffer_add_string(&fn->body, ";\n} else {\n");
	raviX_buffer_add_fstring(&fn->body, " %s(L, arr, ukey, ", setterfunc);
	if (src->type == PSEUDO_TEMP_INT) {
		emit_varname(src, &fn->body);
	} else if (src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL || src->type == PSEUDO_LUASTACK) {
		raviX_buffer_add_fstring(&fn->body, "%s(", getterfunc);
		emit_reg_accessor(fn, src, 0);
		raviX_buffer_add_string(&fn->body, ")");
	} else if (src->type == PSEUDO_CONSTANT) {
		if (src->constant->type == RAVI_TNUMINT) {
			raviX_buffer_add_fstring(&fn->body, "%lld", src->constant->i);
		}
		else {
			raviX_buffer_add_fstring(&fn->body, "%g", src->constant->n);
		}
	}
	raviX_buffer_add_string(&fn->body, ");\n");
	raviX_buffer_add_string(&fn->body, "}\n}\n");
	return 0;
}

static int emit_op_totype(struct function *fn, struct instruction *insn)
{
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " TValue *ra = ");
	emit_reg_accessor(fn, get_first_target(insn), 0);
	if (insn->opcode == op_toiarray) {
		raviX_buffer_add_string(&fn->body, ";\n if (!ttisiarray(ra)) {\n");
		raviX_buffer_add_fstring(&fn->body, "  error_code = %d;\n", Error_integer_array_expected);
	} else if (insn->opcode == op_tofarray) {
		raviX_buffer_add_string(&fn->body, ";\n if (!ttisfarray(ra)) {\n");
		raviX_buffer_add_fstring(&fn->body, "  error_code = %d;\n", Error_number_array_expected);
	} else if (insn->opcode == op_totable) {
		raviX_buffer_add_string(&fn->body, ";\n if (!ttisLtable(ra)) {\n");
		raviX_buffer_add_fstring(&fn->body, "  error_code = %d;\n", Error_table_expected);
	} else if (insn->opcode == op_toclosure) {
		raviX_buffer_add_string(&fn->body, ";\n if (!ttisclosure(ra)) {\n");
		raviX_buffer_add_fstring(&fn->body, "  error_code = %d;\n", Error_closure_expected);
	} else if (insn->opcode == op_tostring) {
		raviX_buffer_add_string(&fn->body, ";\n if (!ttisstring(ra)) {\n");
		raviX_buffer_add_fstring(&fn->body, "  error_code = %d;\n", Error_string_expected);
	} else if (insn->opcode == op_toint) {
		raviX_buffer_add_string(&fn->body, ";\n if (!ttisinteger(ra)) {\n");
		raviX_buffer_add_fstring(&fn->body, "  error_code = %d;\n", Error_integer_expected);
	} else {
		assert(0);
		return -1;
	}
	raviX_buffer_add_string(&fn->body, "  goto Lraise_error;\n");
	raviX_buffer_add_string(&fn->body, " }\n}\n");
	return 0;
}

static int emit_op_toflt(struct function *fn, struct instruction *insn)
{
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " TValue *ra = ");
	emit_reg_accessor(fn, get_first_target(insn), 0);
	raviX_buffer_add_string(&fn->body, ";\n lua_Number n = 0;\n");
	raviX_buffer_add_string(&fn->body, " if (ttisnumber(ra)) { n = (ttisinteger(ra) ? (double) ivalue(ra) : "
					   "fltvalue(ra)); setfltvalue(ra, n); }\n");
	raviX_buffer_add_string(&fn->body, " else {\n");
	raviX_buffer_add_fstring(&fn->body, "  error_code = %d;\n", Error_number_expected);
	raviX_buffer_add_string(&fn->body, "  goto Lraise_error;\n");
	raviX_buffer_add_string(&fn->body, " }\n}\n");
	return 0;
}

static int emit_op_tousertype(struct function *fn, struct instruction *insn)
{
	struct pseudo *typename = get_first_operand(insn);
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " TValue *ra = ");
	emit_reg_accessor(fn, get_first_target(insn), 0);
	raviX_buffer_add_string(&fn->body, ";\n if (!ttisnil(ra)) {\n");
	raviX_buffer_add_string(&fn->body, "  TValue *rb = ");
	emit_reg_accessor(fn, typename, 0);
	raviX_buffer_add_string(&fn->body, ";\n");
	raviX_buffer_add_string(&fn->body,
				"  if (!ttisshrstring(rb) || !raviV_check_usertype(L, tsvalue(rb), ra)) {\n");
	raviX_buffer_add_fstring(&fn->body, "   error_code = %d;\n", Error_type_mismatch);
	raviX_buffer_add_string(&fn->body, "   goto Lraise_error;\n");
	raviX_buffer_add_string(&fn->body, "  }\n");
	raviX_buffer_add_string(&fn->body, " }\n");
	raviX_buffer_add_string(&fn->body, "}\n");
	return 0;
}

static int emit_op_newtable(struct function *fn, struct instruction *insn) {
	struct pseudo *target_pseudo = get_first_target(insn);
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " TValue *ra = ");
	emit_reg_accessor(fn, target_pseudo, 0);
	raviX_buffer_add_string(&fn->body, ";\n raviV_op_newtable(L, ci, ra, 0, 0);\n");
	emit_reload_base(fn);
	raviX_buffer_add_string(&fn->body, "}\n");
	return 0;
}

static int emit_op_newarray(struct function *fn, struct instruction *insn) {
	struct pseudo *target_pseudo = get_first_target(insn);
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, " TValue *ra = ");
	emit_reg_accessor(fn, target_pseudo, 0);
	raviX_buffer_add_fstring(&fn->body, ";\n %s(L, ci, ra);\n",
				 insn->opcode == op_newfarray ? "raviV_op_newarrayfloat" : "raviV_op_newarrayint");
	emit_reload_base(fn);
	raviX_buffer_add_string(&fn->body, "}\n");
	return 0;
}

static int emit_op_closure(struct function *fn, struct instruction *insn) {
	struct pseudo *closure_pseudo = get_first_operand(insn);
	struct pseudo *target_pseudo = get_first_target(insn);

	assert(closure_pseudo->type == PSEUDO_PROC);
	struct proc *proc = closure_pseudo->proc;
	struct proc *parent_proc = proc->parent;
	struct proc *cursor;
	int parent_index = -1;
	int i = 0;
	FOR_EACH_PTR(parent_proc->procs, cursor) {
		if (cursor->id == proc->id) {
			assert(cursor == proc);
			parent_index = i;
			break;
		}
		i++;
	} END_FOR_EACH_PTR(cursor);
	if (parent_index == -1) {
		assert(0);
		return -1;
	}
	unsigned reg = compute_register_from_base(fn, target_pseudo);
	raviX_buffer_add_fstring(&fn->body, "raviV_op_closure(L, ci, cl, %d, %d);\n", reg, parent_index);
	emit_reload_base(fn);
	return 0;
}

static int output_instruction(struct function *fn, struct instruction *insn)
{
	int rc = 0;
	switch (insn->opcode) {
	case op_ret:
		rc = emit_op_ret(fn, insn);
		break;
	case op_br:
		rc = emit_op_br(fn, insn);
		break;
	case op_cbr:
		rc = emit_op_cbr(fn, insn);
		break;
	case op_mov:
	case op_movi:
	case op_movf:
		rc = emit_op_mov(fn, insn);
		break;
	case op_loadglobal:
	case op_get:
	case op_get_skey:
	case op_get_ikey:
	case op_tget_skey:
	case op_tget_ikey:
		rc = emit_op_loadglobal(fn, insn);
		break;
	case op_storeglobal:
	case op_put:
	case op_put_skey:
	case op_put_ikey:
	case op_tput_skey:
	case op_tput_ikey:
		rc = emit_op_storeglobal(fn, insn);
		break;
	case op_call:
		rc = emit_op_call(fn, insn);
		break;

	case op_addff:
	case op_subff:
	case op_mulff:
	case op_divff:

	case op_addii:
	case op_subii:
	case op_mulii:
	case op_divii:
	case op_bandii:
	case op_borii:
	case op_bxorii:
		rc = emit_bin_ii(fn, insn);
		break;

		//	case	    op_shlii:
		//	case	    op_shrii:

	case op_eqii:
	case op_ltii:
	case op_leii:
	case op_eqff:
	case op_ltff:
	case op_leff:
		rc = emit_comp_ii(fn, insn);
		break;

		//	case	op_addfi:
		//	case	    op_subfi:
		//	case	    op_mulfi:
		//	case	    op_divfi:

		//	case	    op_subif:
		//	case	    op_divif:

		//	case	    op_sub:
		//	case	    op_mul:
		//	case	    op_div:
		//	case	    op_idiv:
		//	case	    op_band:
		//	case	    op_bor:
		//	case	    op_bxor:
		//	case	    op_shl:
		//	case	    op_shr:

		//	case	    op_eq:
		//	case	    op_lt:
		//	case	    op_le:

	case op_iaget_ikey:
	case op_faget_ikey:
		rc = emit_op_arrayget_ikey(fn, insn);
		break;

	case op_iaput_ival:
	case op_faput_fval:
		rc = emit_op_arrayput_val(fn, insn);
		break;

	case op_toiarray:
	case op_tofarray:
	case op_totable:
	case op_tostring:
	case op_toclosure:
	case op_toint:
		rc = emit_op_totype(fn, insn);
		break;

	case op_toflt:
		rc = emit_op_toflt(fn, insn);
		break;

	case op_totype:
		rc = emit_op_tousertype(fn, insn);
		break;

	case op_closure:
		rc = emit_op_closure(fn, insn);
		break;

	case op_newtable:
		rc = emit_op_newtable(fn, insn);
		break;

	case op_newiarray:
		rc = emit_op_newarray(fn, insn);
		break;

	case op_newfarray:
		rc = emit_op_newarray(fn, insn);
		break;

	default:
		fprintf(stderr, "Usupported opcode %s\n", raviX_opcode_name(insn->opcode));
		rc = -1;
	}
	return rc;
}

static int output_instructions(struct function *fn, struct instruction_list *list)
{
	struct instruction *insn;
	int rc = 0;
	FOR_EACH_PTR(list, insn)
	{
		rc = output_instruction(fn, insn);
		if (rc != 0)
			break;
	}
	END_FOR_EACH_PTR(insn)
	return rc;
}

static int output_basic_block(struct function *fn, struct basic_block *bb)
{
	int rc = 0;
	raviX_buffer_add_fstring(&fn->body, "L%d:\n", bb->index);
	if (bb->index == ENTRY_BLOCK) {
	} else if (bb->index == EXIT_BLOCK) {
	} else {
	}
	rc = output_instructions(fn, bb->insns);
	if (bb->index == EXIT_BLOCK) {
		raviX_buffer_add_string(&fn->body, "return result;\n");
		raviX_buffer_add_string(&fn->body, "Lraise_error:\n");
		raviX_buffer_add_string(&fn->body, "raise_error(L, error_code); /* does not return */\n");
		raviX_buffer_add_string(&fn->body, "return result;\n");
	}
	return rc;
}

static inline unsigned get_num_params(struct proc *proc)
{
	return ptrlist_size((const struct ptr_list *)proc->function_expr->function_expr.args);
}

/* Generate C code for each proc recursively */
static int generate_C_code(struct Ravi_CompilerInterface *ravi_interface, struct proc *proc, buffer_t *mb)
{
	int rc = 0;
	struct function fn;
	initfn(&fn, proc, ravi_interface);

	ravi_interface->lua_setMaxStackSize(ravi_interface->context, (Proto *)proc->userdata,
					    compute_max_stack_size(proc));
	ravi_interface->lua_setNumParams(ravi_interface->context, (Proto *)proc->userdata, get_num_params(proc));

	struct basic_block *bb;
	for (int i = 0; i < (int)proc->node_count; i++) {
		bb = proc->nodes[i];
		rc = output_basic_block(&fn, bb);
		if (rc != 0)
			break;
	}

	raviX_buffer_add_string(&fn.body, "}\n");
	raviX_buffer_add_string(mb, fn.prologue.buf);
	raviX_buffer_add_string(mb, fn.body.buf);
	cleanup(&fn);

	if (rc != 0)
		return rc;

	struct proc *childproc;
	FOR_EACH_PTR(proc->procs, childproc)
	{
		rc = generate_C_code(ravi_interface, childproc, mb);
		if (rc != 0)
			return rc;
	}
	END_FOR_EACH_PTR(childproc);
	return 0;
}

/* Traverse the proto hierarchy and assign each proto its compiled function */
static void assign_compiled_functions_to_protos(struct Ravi_CompilerInterface *ravi_interface, struct proc *proc,
						void *module)
{
	lua_CFunction f = ravi_interface->get_compiled_function(ravi_interface->context, module, proc->funcname);
	Proto *proto = (Proto *)proc->userdata;
	ravi_interface->lua_setProtoFunction(ravi_interface->context, proto, f);
	struct proc *childproc;
	FOR_EACH_PTR(proc->procs, childproc) { assign_compiled_functions_to_protos(ravi_interface, childproc, module); }
	END_FOR_EACH_PTR(childproc);
}

static inline struct ast_node *get_parent_function_of_upvalue(struct lua_symbol *symbol)
{
	struct ast_node *this_function = symbol->upvalue.target_function;
	struct ast_node *parent_function = this_function->function_expr.parent_function;
	return parent_function;
}

static unsigned get_upvalue_idx(struct proc *proc, struct lua_symbol *upvalue_symbol, bool *in_stack)
{
	*in_stack = false;
	/*
	 * If the upvalue refers to a local variable in parent proto then idx should contain
	 * the register for the local variable and instack should be true, else idx should have the index of
	 * upvalue in parent proto and instack should be false.
	 */
	struct lua_symbol *underlying = upvalue_symbol->upvalue.target_variable;
	if (underlying->symbol_type == SYM_LOCAL) {
		struct ast_node *local_function = underlying->variable.block->function;
		struct ast_node *parent_function = get_parent_function_of_upvalue(upvalue_symbol);
		if (parent_function == local_function) {
			/* Upvalue is a local in parent function */
			*in_stack = true;
			return underlying->variable.pseudo->regnum;
		}
	}
	/* Search for the upvalue in parent function */
	struct lua_symbol *sym;
	struct ast_node *this_function = upvalue_symbol->upvalue.target_function;
	FOR_EACH_PTR(this_function->function_expr.upvalues, sym)
	{
		if (sym->upvalue.target_variable == upvalue_symbol->upvalue.target_variable) {
			// Same variable
			return sym->upvalue.upvalue_index;
		}
	}
	END_FOR_EACH_PTR(sym);
	assert(0);
	return 0;
}

static void register_upvalues(struct Ravi_CompilerInterface *ravi_interface, struct proc *proc, Proto *proto)
{
	struct lua_symbol *sym;
	struct ast_node *this_function = proc->function_expr;
	FOR_EACH_PTR(this_function->function_expr.upvalues, sym)
	{
		bool in_stack = false;
		unsigned idx = get_upvalue_idx(proc, sym, &in_stack);
		/* discarding const below */
		ravi_interface->lua_addUpValue(ravi_interface->context, proto,
					       (struct string_object *)sym->upvalue.target_variable->variable.var_name,
					       idx, in_stack, sym->upvalue.value_type.type_code,
					       (struct string_object *)sym->upvalue.value_type.type_name);
	}
	END_FOR_EACH_PTR(sym);
}

/* Create protos for all the procs with the correct relationships. We do this recursively for all the
 * child procs. The top-level proto is precreated by the caller (actually it is precreated on the Ravi side).
 */
static void create_protos(struct Ravi_CompilerInterface *ravi_interface, struct proc *proc, Proto *proto)
{
	proc->userdata = proto;
	register_upvalues(ravi_interface, proc, proto);
	struct proc *child_proc;
	FOR_EACH_PTR(proc->procs, child_proc)
	{
		Proto *child_proto = ravi_interface->lua_newProto(ravi_interface->context, proto);
		create_protos(ravi_interface, child_proc, child_proto);
	}
	END_FOR_EACH_PTR(childproc);
}

static Proto *stub_newProto(void *context, Proto *parent) { return NULL; }
static int stub_newStringConstant(void *context, Proto *proto, struct string_object *s) { return 0; }
static void stub_init_C_compiler(void *context) {}
static void stub_finish_C_compiler(void *context) {}
static void *stub_compile_C(void *context, const char *C_src, unsigned len) { return NULL; }
static lua_CFunction stub_lua_getFunction(void *context, void *module, const char *name) { return NULL; }
static void stub_lua_setProtoFunction(void *context, Proto *p, lua_CFunction func) {}
static void stub_lua_setVarArg(void *context, Proto *p) {}
static int stub_lua_addUpValue(void *context, Proto *f, struct string_object *name, unsigned idx, int instack,
			       unsigned typecode, struct string_object *usertype)
{
	return 0;
}
static void stub_lua_setNumParams(void *context, Proto *p, unsigned num_params) {}
static void stub_lua_setMaxStackSize(void *context, Proto *p, unsigned max_stack_size) {}
static void debug_message(void *context, const char *filename, long long line, const char *message)
{
	fprintf(stdout, "%s:%lld: %s\n", filename, line, message);
}
static void error_message(void *context, const char *message) { fprintf(stdout, "ERROR: %s\n", message); }

static struct Ravi_CompilerInterface stub_compilerInterface = {.context = NULL,
							       .source_name = "input",
							       .source = NULL,
							       .source_len = 0,
							       .main_proto = NULL,
							       .init_C_compiler = stub_init_C_compiler,
							       .compile_C = stub_compile_C,
							       .finish_C_compiler = stub_finish_C_compiler,
							       .get_compiled_function = stub_lua_getFunction,
							       .lua_setProtoFunction = stub_lua_setProtoFunction,
							       .lua_newProto = stub_newProto,
							       .lua_newStringConstant = stub_newStringConstant,
							       .lua_setVarArg = stub_lua_setVarArg,
							       .lua_addUpValue = stub_lua_addUpValue,
							       .lua_setNumParams = stub_lua_setNumParams,
							       .lua_setMaxStackSize = stub_lua_setMaxStackSize,
							       .error_message = error_message,
							       .debug_message = debug_message};

/* Generate and compile C code */
int raviX_generate_C(struct linearizer_state *linearizer, buffer_t *mb, struct Ravi_CompilerInterface *ravi_interface)
{
	if (ravi_interface == NULL)
		ravi_interface = &stub_compilerInterface;

	raviX_create_string(linearizer->ast_container, "_ENV", 4);

	/* Add the common header portion */
	raviX_buffer_add_string(mb, Lua_header);

	/* Create protos for each proc we will compile */
	Proto *main_proto = ravi_interface->main_proto;
	/* We don't support var args yet */
	// ravi_interface->lua_setVarArg(ravi_interface->context, main_proto);
	/* Create all the child protos as we will need them to be there for code gen */
	create_protos(ravi_interface, linearizer->main_proc, main_proto);

	/* Recursively generate C code for procs */
	if (generate_C_code(ravi_interface, linearizer->main_proc, mb) != 0) {
		return -1;
	}
	/* Compile the generated code */
	ravi_interface->init_C_compiler(ravi_interface->context);
	void *module = ravi_interface->compile_C(ravi_interface->context, mb->buf, mb->pos);
	if (module != NULL) {
		/* Associate each proto with its compiled function */
		assign_compiled_functions_to_protos(ravi_interface, linearizer->main_proc, module);
	}
	ravi_interface->finish_C_compiler(ravi_interface->context);
	return module != NULL ? 0 : -1;
}

void raviX_generate_C_tofile(struct linearizer_state *linearizer, FILE *fp)
{
	buffer_t mb;
	raviX_buffer_init(&mb, 4096);
	raviX_generate_C(linearizer, &mb, NULL);
	fprintf(fp, "%s\n", mb.buf);
	raviX_buffer_free(&mb);
}
