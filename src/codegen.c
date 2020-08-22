/*
 * Convert the linear IR to C code that can be compiled and
 * executed by Ravi VM.
 */

#include "codegen.h"

#include <assert.h>
#include <stddef.h>

#define GOTO_ON_ERROR 1

/*
 * Only 64-bits supported right now
 * Following must be kept in sync with changes in the actual header files
 */

static const char Lua_header[] =
    //"typedef __SIZE_TYPE__ size_t;\n"
    "typedef long long size_t;\n"
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
    "#define tonumberns(o,n) \\\n"
    "	(ttisfloat(o) ? ((n) = fltvalue(o), 1) : \\\n"
    "	(ttisinteger(o) ? ((n) = cast_num(ivalue(o)), 1) : 0))\n"
    "#define intop(op,v1,v2) l_castU2S(l_castS2U(v1) op l_castS2U(v2))\n"
    "#define nan (0./0.)\n"
    "#define inf (1./0.)\n"
    "#define luai_numunm(L,a)        (-(a))\n";

struct function {
	struct proc *proc;
	char fname[30];
	membuff_t prologue;
	membuff_t body;
};

/* readonly statics */
static const char *int_var_prefix = "i_";
static const char *flt_var_prefix = "f_";
static struct pseudo NIL_pseudo = {.type = PSEUDO_NIL};

static void emit_vars(const char *type, const char *prefix, struct pseudo_generator *gen, membuff_t *mb)
{
	if (gen->next_reg == 0)
		return;
	for (unsigned i = 0; i < gen->next_reg; i++) {
		if (i == 0) {
			raviX_buffer_add_fstring(mb, "%s ", type);
		}
		if (i > 0) {
			raviX_buffer_add_string(mb, ", ");
		}
		raviX_buffer_add_fstring(mb, "%s%d", prefix, i);
	}
	raviX_buffer_add_string(mb, ";\n");
}

static void emit_varname(struct pseudo *pseudo, membuff_t *mb)
{
	if (pseudo->type == PSEUDO_TEMP_INT) {
		raviX_buffer_add_fstring(mb, "%s%d", int_var_prefix, pseudo->regnum);
	} else if (pseudo->type == PSEUDO_TEMP_FLT) {
		raviX_buffer_add_fstring(mb, "%s%d", flt_var_prefix, pseudo->regnum);
	}
}

static void initfn(struct function *fn, struct proc *proc)
{
	fn->proc = proc;
	snprintf(fn->fname, sizeof fn->fname, "__ravifunc_%d", proc->id);
	raviX_buffer_init(&fn->prologue, 4096);
	raviX_buffer_init(&fn->body, 4096);
	raviX_buffer_add_fstring(&fn->prologue, "extern int %s(lua_State *L);\n", fn->fname);
	raviX_buffer_add_fstring(&fn->prologue, "int %s(lua_State *L) {\n", fn->fname);
	raviX_buffer_add_string(&fn->prologue, "int error_code = 0;\n");
	raviX_buffer_add_string(&fn->prologue, "int result = 0;\n");
	raviX_buffer_add_string(&fn->prologue, "CallInfo *ci = L->ci;\n");
	raviX_buffer_add_string(&fn->prologue, "LClosure *cl = clLvalue(ci->func);\n");
	raviX_buffer_add_string(&fn->prologue, "TValue *k = cl->p->k;\n");
	raviX_buffer_add_string(&fn->prologue, "StkId base = ci->u.l.base;\n");
	emit_vars("lua_Integer", int_var_prefix, &proc->temp_int_pseudos, &fn->prologue);
	emit_vars("lua_Number", flt_var_prefix, &proc->temp_flt_pseudos, &fn->prologue);
}

static void cleanup(struct function *fn)
{
	raviX_buffer_free(&fn->prologue);
	raviX_buffer_free(&fn->body);
}

static struct pseudo *get_target_value(struct instruction *insn, unsigned i)
{
	if (insn->targets == NULL)
		return NULL;
	return (struct pseudo *)ptrlist_nth_entry((struct ptr_list *)insn->targets, i);
}

/* access a pseudo that is on Lua stack or is an upvalue */
static void emit_reg_accessor(struct function *fn, const struct pseudo *pseudo)
{
	if (pseudo->type == PSEUDO_LUASTACK) {
		// Note pseudo->stackidx is relative to base and may be negative
		raviX_buffer_add_fstring(&fn->body, "R(%d)", pseudo->stackidx);
	} else if (pseudo->type == PSEUDO_TEMP_ANY) {
		// Note we put all temps on Lua stack after the locals
		raviX_buffer_add_fstring(&fn->body, "R(%d)", pseudo->regnum + fn->proc->local_pseudos.next_reg);
	} else if (pseudo->type == PSEUDO_SYMBOL) {
		if (pseudo->symbol->symbol_type == SYM_LOCAL) {
			raviX_buffer_add_fstring(&fn->body, "R(%d)", pseudo->regnum);
		} else if (pseudo->symbol->symbol_type == SYM_UPVALUE) {
			raviX_buffer_add_fstring(&fn->body, "cl->upvals[%d]->v", pseudo->regnum);
		} else {
			assert(0);
		}
	} else {
		assert(0);
	}
}

/*copy floating point value to a temporary float */
static void emit_move_flttemp(struct function *fn, struct pseudo *src, struct pseudo *dst)
{
	if (src->type == PSEUDO_CONSTANT) {
		if (src->constant->type == RAVI_TNUMFLT) {
			emit_varname(dst, &fn->body);
			raviX_buffer_add_fstring(&fn->body, " = %.16g;\n", src->constant->n);
		} else {
			// FIXME can we have int value?
			assert(0);
		}
	} else if (src->type == PSEUDO_LUASTACK || src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL) {
		raviX_buffer_add_string(&fn->body, "{\nTValue *reg = ");
		emit_reg_accessor(fn, src);
		raviX_buffer_add_string(&fn->body, ";\n");
		emit_varname(dst, &fn->body);
		raviX_buffer_add_string(&fn->body, " = fltvalue(reg);\n}\n");
	} else {
		assert(0);
	}
}

/*copy integer value to temporary int */
static void emit_move_inttemp(struct function *fn, struct pseudo *src, struct pseudo *dst)
{
	if (src->type == PSEUDO_CONSTANT) {
		if (src->constant->type == RAVI_TNUMFLT) {
			emit_varname(dst, &fn->body);
			raviX_buffer_add_fstring(&fn->body, " = %lld;\n", src->constant->i);
		} else {
			// FIXME can we have float value?
			assert(0);
		}
	} else if (src->type == PSEUDO_LUASTACK || src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL) {
		raviX_buffer_add_string(&fn->body, "{\nTValue *reg = ");
		emit_reg_accessor(fn, src);
		raviX_buffer_add_string(&fn->body, ";\n");
		emit_varname(dst, &fn->body);
		raviX_buffer_add_string(&fn->body, " = ivalue(reg);\n}\n");
	} else {
		assert(0);
	}
}

/* copy a value from source pseudo to destination pseudo.*/
static void emit_move(struct function *fn, struct pseudo *src, struct pseudo *dst)
{
	if (dst->type == PSEUDO_TEMP_FLT) {
		emit_move_flttemp(fn, src, dst);
	} else if (dst->type == PSEUDO_TEMP_INT) {
		emit_move_inttemp(fn, src, dst);
	} else if (dst->type == PSEUDO_TEMP_ANY || dst->type == PSEUDO_SYMBOL || dst->type == PSEUDO_LUASTACK) {
		if (src->type == PSEUDO_LUASTACK || src->type == PSEUDO_TEMP_ANY || src->type == PSEUDO_SYMBOL) {
			raviX_buffer_add_string(&fn->body, "{\nconst TValue *src_reg = ");
			emit_reg_accessor(fn, src);
			raviX_buffer_add_string(&fn->body, ";\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst);
			raviX_buffer_add_string(&fn->body, ";\n*dst_reg = *src_reg;\n}\n");
		} else if (src->type == PSEUDO_TEMP_INT) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst);
			raviX_buffer_add_string(&fn->body, ";\nsetivalue(dst_reg, ");
			emit_varname(src, &fn->body);
			raviX_buffer_add_string(&fn->body, ");\n}\n");
		} else if (src->type == PSEUDO_TEMP_FLT) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst);
			raviX_buffer_add_string(&fn->body, ";\nsetfltvalue(dst_reg, ");
			emit_varname(src, &fn->body);
			raviX_buffer_add_string(&fn->body, ");\n}\n");
		} else if (src->type == PSEUDO_TRUE || src->type == PSEUDO_FALSE) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst);
			raviX_buffer_add_fstring(&fn->body, ";\nsetbvalue(dst_reg, %d);\n}\n",
						 src->type == PSEUDO_TRUE ? 1 : 0);
		} else if (src->type == PSEUDO_NIL) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst);
			raviX_buffer_add_string(&fn->body, ";\nsetnilvalue(dst_reg);\n}\n");
		} else if (src->type == PSEUDO_CONSTANT) {
			raviX_buffer_add_string(&fn->body, "{\nTValue *dst_reg = ");
			emit_reg_accessor(fn, dst);
			raviX_buffer_add_string(&fn->body, ";\n");
			if (src->constant->type == RAVI_TNUMINT) {
				raviX_buffer_add_fstring(&fn->body, "setivalue(dst_reg, %lld);\n", src->constant->i);
			} else if (src->constant->type == RAVI_TNUMFLT) {
				raviX_buffer_add_fstring(&fn->body, "setfltvalue(dst_reg, %g);\n", src->constant->n);
			} else if (src->constant->type == RAVI_TBOOLEAN) {
				raviX_buffer_add_fstring(&fn->body, "setbvalue(dst_reg, %i);\n", (int)src->constant->i);
			} else if (src->constant->type == RAVI_TNIL) {
				raviX_buffer_add_fstring(&fn->body, "setnilvalue(dst_reg);\n", (int)src->constant->i);
			} else if (src->constant->type == RAVI_TSTRING) {
				// FIXME issue #32 We should add string constants to the proto
				assert(0);
			} else {
				assert(0);
			}
		} else {
			assert(0);
		}
	}
}

/* dest_value must be TValue * */
static void emit_assign(struct function *fn, const char *dest_value, struct pseudo *pseudo)
{
	if (pseudo->type == PSEUDO_NIL) {
		raviX_buffer_add_fstring(&fn->body, "setnilvalue(%s);\n", dest_value);
	} else if (pseudo->type == PSEUDO_TRUE) {
		raviX_buffer_add_fstring(&fn->body, "setbvalue(%s, 1);\n", dest_value);
	} else if (pseudo->type == PSEUDO_FALSE) {
		raviX_buffer_add_fstring(&fn->body, "setbvalue(%s, 0);\n", dest_value);
	}
}

static void emit_jump(struct function *fn, struct pseudo *pseudo)
{
	assert(pseudo->type == PSEUDO_BLOCK);
	raviX_buffer_add_fstring(&fn->body, "goto L%d;\n", pseudo->block->index);
}

static void emit_op_ret(struct function *fn, struct instruction *insn)
{
	// TODO Only call luaF_close if needed
#ifdef RAVI_DEFER_STATEMENT
	raviX_buffer_add_string(&fn->body, "if (cl->p->sizep > 0) {\n luaF_close(L, base, LUA_OK);\n");
	raviX_buffer_add_string(&fn->body, " base = ci->u.l.base;\n");
	raviX_buffer_add_string(&fn->body, "}\n");
#else
	raviX_buffer_add_string(&fn->body, "if (cl->p->sizep > 0) luaF_close(L, base);\n");
#endif
	raviX_buffer_add_string(&fn->body, "{\n");
	raviX_buffer_add_string(&fn->body, "int wanted = ci->nresults;\n");
	raviX_buffer_add_fstring(&fn->body, "if (wanted == -1) wanted = %d;\n",
				 ptrlist_size((struct ptr_list *)insn->operands));
	raviX_buffer_add_string(&fn->body, "L->ci = ci->previous;\n");
	struct pseudo *pseudo;
	int i = 0;
	FOR_EACH_PTR(insn->operands, pseudo)
	{
		// First result should be copied to base[-1] so all offsets are adjusted by -1.
		struct pseudo dummy_dest = {.type = PSEUDO_LUASTACK, .stackidx = i - 1};
		raviX_buffer_add_fstring(&fn->body, "if (%d < wanted) {\n", i);
		emit_move(fn, pseudo, &dummy_dest);
		raviX_buffer_add_string(&fn->body, "}\n");
		i++;
	}
	END_FOR_EACH_PTR(pseudo);
	raviX_buffer_add_fstring(&fn->body, "if (%d < wanted) {\nint j = %d;\n", i, i);
	raviX_buffer_add_string(&fn->body, "while (j < wanted) {\n");
	{
		raviX_buffer_add_string(&fn->body, "setnilvalue(R(j-1));\n");
		raviX_buffer_add_string(&fn->body, "j++;\n");
	}
	raviX_buffer_add_string(&fn->body, "}\n");
	raviX_buffer_add_string(&fn->body, "}\n");
	raviX_buffer_add_string(&fn->body, "}\n");
	emit_jump(fn, get_target_value(insn, 0));
}

static void output_instruction(struct function *fn, struct instruction *insn)
{
	switch (insn->opcode) {
	case op_ret:
		emit_op_ret(fn, insn);
	}
}

static void output_instructions(struct function *fn, struct instruction_list *list)
{
	struct instruction *insn;
	FOR_EACH_PTR(list, insn) { output_instruction(fn, insn); }
	END_FOR_EACH_PTR(insn)
}

static void output_basic_block(struct function *fn, struct basic_block *bb)
{
	raviX_buffer_add_fstring(&fn->body, "L%d:\n", bb->index);
	if (bb->index == ENTRY_BLOCK) {
	} else if (bb->index == EXIT_BLOCK) {
	} else {
	}
	output_instructions(fn, bb->insns);
	if (bb->index == EXIT_BLOCK) {
		raviX_buffer_add_string(&fn->body, "return result;\n");
	}
}

static void output_proc(struct proc *proc, membuff_t *mb)
{
	struct function fn;
	initfn(&fn, proc);

	struct basic_block *bb;
	for (int i = 0; i < (int)proc->node_count; i++) {
		bb = proc->nodes[i];
		output_basic_block(&fn, bb);
	}

	raviX_buffer_add_string(&fn.body, "}\n");
	raviX_buffer_add_string(mb, fn.prologue.buf);
	raviX_buffer_add_string(mb, fn.body.buf);
	cleanup(&fn);
}

void raviX_generate_C(struct linearizer_state *linearizer, membuff_t *mb)
{
	// raviX_buffer_add_string(mb, Lua_header);
	output_proc(linearizer->main_proc, mb);
	struct proc *proc;
	FOR_EACH_PTR(linearizer->all_procs, proc)
	{
		if (proc == linearizer->main_proc)
			continue;
		output_proc(proc, mb);
	}
	END_FOR_EACH_PTR(proc)
}

void raviX_generate_C_tofile(struct linearizer_state *linearizer, FILE *fp)
{
	membuff_t mb;
	raviX_buffer_init(&mb, 4096);
	raviX_generate_C(linearizer, &mb);
	fprintf(fp, "%s\n", mb.buf);
	raviX_buffer_free(&mb);
}
