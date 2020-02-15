#ifndef ravicomp_LUA_DEFS_H
#define ravicomp_LUA_DEFS_H

#include <limits.h>

typedef long long lua_Integer;
typedef unsigned long long lua_Unsigned;
typedef double lua_Number;
typedef unsigned char lu_byte;

/*
** grep "ORDER OPR" if you change these enums  (ORDER OP)
*/
typedef enum BinOpr {
	OPR_ADD, OPR_SUB, OPR_MUL, OPR_MOD, OPR_POW,
	OPR_DIV,
	OPR_IDIV,
	OPR_BAND, OPR_BOR, OPR_BXOR,
	OPR_SHL, OPR_SHR,
	OPR_CONCAT,
	OPR_EQ, OPR_LT, OPR_LE,
	OPR_NE, OPR_GT, OPR_GE,
	OPR_AND, OPR_OR,
	OPR_NOBINOPR
} BinOpr;

/** RAVI change */
typedef enum UnOpr { OPR_MINUS, OPR_BNOT, OPR_NOT, OPR_LEN, OPR_TO_INTEGER,
	OPR_TO_NUMBER, OPR_TO_INTARRAY, OPR_TO_NUMARRAY, OPR_TO_TABLE, OPR_TO_STRING,
	OPR_TO_CLOSURE, OPR_TO_TYPE, OPR_NOUNOPR } UnOpr;

#define LUA_ENV "_ENV"
#define LUA_MAXINTEGER INT_MAX
#endif
