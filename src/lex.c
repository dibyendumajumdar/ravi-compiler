/**
 * The lexer is basically a hacked version of Lua 5.3 lexer.
 * Copyright (C) 1994-2019 Lua.org, PUC-Rio.
 */

#include "ravi_ast.h"

#include <limits.h>
#include <locale.h>
#include <math.h>

enum { EOZ = -1 }; /* end of stream */
#define cast(t, v) ((t)v)
#define cast_int(v) cast(int, v)
#define cast_uchar(c) cast(unsigned char, c)
#define cast_num(n) cast(lua_Number, n)
#define l_castU2S(i) ((lua_Integer)(i))
static inline int zgetc(LexState *z) { return z->n-- > 0 ? cast_uchar(*z->p++) : EOZ; }
static inline void next(LexState *ls) { ls->current = zgetc(ls); }
static inline bool currIsNewline(LexState *ls) { return ls->current == '\n' || ls->current == '\r'; }

#define lua_getlocaledecpoint() (localeconv()->decimal_point[0])

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
/* ORDER RESERVED */
static const char *const luaX_tokens[] = {
    "and",    "break",	  "do",	      "else",	 "elseif",     "end",	    "false",  "for",	  "function",
    "goto",   "if",	  "in",	      "local",	 "defer",      "nil",	    "not",    "or",	  "repeat",
    "return", "then",	  "true",     "until",	 "while",      "//",	    "..",     "...",	  "==",
    ">=",     "<=",	  "~=",	      "<<",	 ">>",	       "::",	    "<eof>",  "<number>", "<integer>",
    "<name>", "<string>", "@integer", "@number", "@integer[]", "@number[]", "@table", "@string",  "@closure"};
static inline int is_reserved(const char *s)
{
	for (int i = 0; i < ARRAY_SIZE(luaX_tokens); i++) {
		if (strcmp(luaX_tokens[i], s) == 0)
			return i;
	}
	return -1;
}

enum { ALPHABIT = 0, DIGITBIT = 1, PRINTBIT = 2, SPACEBIT = 3, XDIGITBIT = 4 };

#define MASK(B) (1 << (B))

const lu_byte luai_ctype_[UCHAR_MAX + 2] = {
    0x00,											    /* EOZ */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,						    /* 0. */
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 1. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, /* 2. */
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, /* 3. */
    0x16, 0x16, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x05, /* 4. */
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, /* 5. */
    0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x05, 0x04, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x05, /* 6. */
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, /* 7. */
    0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 8. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 9. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* b. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* c. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* d. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* e. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* f. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*
** add 1 to char to allow index -1 (EOZ)
*/
#define testprop(c, p) (luai_ctype_[(c) + 1] & (p))

/*
** 'lalpha' (Lua alphabetic) and 'lalnum' (Lua alphanumeric) both include '_'
*/
static inline bool lislalpha(int c) { return testprop(c, MASK(ALPHABIT)); }
static inline bool lislalnum(int c) { return testprop(c, (MASK(ALPHABIT) | MASK(DIGITBIT))); }
static inline bool lisdigit(int c) { return testprop(c, MASK(DIGITBIT)); }
static inline bool lisspace(int c) { return testprop(c, MASK(SPACEBIT)); }
static inline bool lisprint(int c) { return testprop(c, MASK(PRINTBIT)); }
static inline bool lisxdigit(int c) { return testprop(c, MASK(XDIGITBIT)); }

/*
** this 'ltolower' only works for alphabetic characters
*/
static inline int ltolower(int c) { return ((c) | ('A' ^ 'a')); }

#define lua_str2number(s, p) ((lua_Number)strtod((s), (p)))
static void save(LexState *ls, int c);

void luaX_token2str(LexState *ls, int token)
{
	if (token < FIRST_RESERVED) { /* single-byte symbols? */
		assert(token == cast_uchar(token));
		membuff_add_fstring(&ls->container->error_message, "'%c'", token);
	} else {
		const char *s = luaX_tokens[token - FIRST_RESERVED];
		if (token < TK_EOS) /* fixed format (symbols and reserved words)? */
			membuff_add_fstring(&ls->container->error_message, "'%s'", s);
		else /* names, strings, and numerals */
			membuff_add_string(&ls->container->error_message, s);
	}
}

static void txtToken(LexState *ls, int token)
{
	switch (token) {
	case TK_NAME:
	case TK_STRING:
	case TK_FLT:
	case TK_INT:
		save(ls, '\0');
		membuff_add_fstring(&ls->container->error_message, "'%s'", luaZ_buffer(ls->buff));
	default:
		luaX_token2str(ls, token);
	}
}
static void lexerror(LexState *ls, const char *msg, int token)
{
	membuff_add_fstring(&ls->container->error_message, "%s(%d): %s", ls->source, ls->linenumber, msg);
	if (token) {
		membuff_add_string(&ls->container->error_message, " near ");
		txtToken(ls, token);
	}
	longjmp(ls->container->env, 1);
}

void raviX_syntaxerror(LexState *ls, const char *msg) { lexerror(ls, msg, ls->t.token); }

static void save(LexState *ls, int c)
{
	Mbuffer *b = ls->buff;
	if (luaZ_bufflen(b) + 1 > luaZ_sizebuffer(b)) {
		size_t newsize;
		if (luaZ_sizebuffer(b) >= INT_MAX / 2)
			lexerror(ls, "lexical element too long", 0);
		size_t oldsize = luaZ_sizebuffer(b);
		if (oldsize == 0)
			newsize = 32;
		else
			newsize = oldsize * 2;
		luaZ_resizebuffer(b, newsize);
	}
	luaZ_addc(b, c);
}

static inline void save_and_next(LexState *ls)
{
	save(ls, ls->current);
	next(ls);
}

void luaX_init(LexState *ls)
{
	int i;
	raviX_create_string(ls->container, LUA_ENV, strlen(LUA_ENV)); /* create env name */
	for (i = 0; i < NUM_RESERVED; i++) {
		raviX_create_string(ls->container, luaX_tokens[i], strlen(luaX_tokens[i]));
	}
}

/*
** creates a new string and anchors it in scanner's table so that
** it will not be collected until the end of the compilation
** (by that time it should be anchored somewhere)
*/
const char *luaX_newstring(LexState *ls, const char *str, size_t l)
{
	return raviX_create_string(ls->container, str, l);
}

/*
** increment line number and skips newline sequence (any of
** \n, \r, \n\r, or \r\n)
*/
static void inclinenumber(LexState *ls)
{
	int old = ls->current;
	assert(currIsNewline(ls));
	next(ls); /* skip '\n' or '\r' */
	if (currIsNewline(ls) && ls->current != old)
		next(ls); /* skip '\n\r' or '\r\n' */
	if (++ls->linenumber >= INT_MAX)
		lexerror(ls, "chunk has too many lines", 0);
}

void raviX_setinput(struct ast_container *container, LexState *ls, const char *buf, size_t buflen, const char *source)
{
	ls->container = container;
	ls->t.token = 0;
	ls->buf = buf;
	ls->bufsize = buflen;
	ls->n = ls->bufsize;
	ls->p = ls->buf;
	ls->current = zgetc(ls);
	ls->lookahead.token = TK_EOS; /* no look-ahead token */
	ls->linenumber = 1;
	ls->lastline = 1;
	ls->source = source;
	ls->envn = raviX_create_string(ls->container, LUA_ENV, strlen(LUA_ENV)); /* get env name */
	ls->buff = &container->buff;
}

/*
** =======================================================
** LEXICAL ANALYZER
** =======================================================
*/

static int check_next1(LexState *ls, int c)
{
	if (ls->current == c) {
		next(ls);
		return 1;
	} else
		return 0;
}

static int check_save_next1(LexState *ls, int c)
{
	if (ls->current == c) {
		save_and_next(ls);
		return 1;
	} else
		return 0;
}

/*
** Check whether current char is in set 'set' (with two chars) and
** saves it
*/
static int check_next2(LexState *ls, const char *set)
{
	assert(set[2] == '\0');
	if (ls->current == set[0] || ls->current == set[1]) {
		save_and_next(ls);
		return 1;
	} else
		return 0;
}

int luaO_hexavalue(int c)
{
	if (lisdigit(c))
		return c - '0';
	else
		return (ltolower(c) - 'a') + 10;
}

static int isneg(const char **s)
{
	if (**s == '-') {
		(*s)++;
		return 1;
	} else if (**s == '+')
		(*s)++;
	return 0;
}

/*
** {==================================================================
** Lua's implementation for 'lua_strx2number'
** ===================================================================
*/

#if !defined(lua_strx2number)

/* maximum number of significant digits to read (to avoid overflows
   even with single floats) */
#define MAXSIGDIG 30

/*
** convert an hexadecimal numeric string to a number, following
** C99 specification for 'strtod'
*/
static lua_Number lua_strx2number(const char *s, char **endptr)
{
	int dot = lua_getlocaledecpoint();
	lua_Number r = 0.0;  /* result (accumulator) */
	int sigdig = 0;	     /* number of significant digits */
	int nosigdig = 0;    /* number of non-significant digits */
	int e = 0;	     /* exponent correction */
	int neg;	     /* 1 if number is negative */
	int hasdot = 0;	     /* true after seen a dot */
	*endptr = (char *)s; /* nothing is valid yet */
	while (lisspace(cast_uchar(*s)))
		s++;						  /* skip initial spaces */
	neg = isneg(&s);					  /* check signal */
	if (!(*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X'))) /* check '0x' */
		return 0.0;					  /* invalid format (no '0x') */
	for (s += 2;; s++) {					  /* skip '0x' and read numeral */
		if (*s == dot) {
			if (hasdot)
				break; /* second dot? stop loop */
			else
				hasdot = 1;
		} else if (lisxdigit(cast_uchar(*s))) {
			if (sigdig == 0 && *s == '0') /* non-significant digit (zero)? */
				nosigdig++;
			else if (++sigdig <= MAXSIGDIG) /* can read it without overflow? */
				r = (r * cast_num(16.0)) + luaO_hexavalue(*s);
			else
				e++; /* too many digits; ignore, but still count for exponent */
			if (hasdot)
				e--; /* decimal digit? correct exponent */
		} else
			break; /* neither a dot nor a digit */
	}
	if (nosigdig + sigdig == 0)   /* no digits? */
		return 0.0;	      /* invalid format */
	*endptr = (char *)s;	      /* valid up to here */
	e *= 4;			      /* each digit multiplies/divides value by 2^4 */
	if (*s == 'p' || *s == 'P') { /* exponent part? */
		int exp1 = 0;	      /* exponent value */
		int neg1;	      /* exponent signal */
		s++;		      /* skip 'p' */
		neg1 = isneg(&s);     /* signal */
		if (!lisdigit(cast_uchar(*s)))
			return 0.0;		 /* invalid; must have at least one digit */
		while (lisdigit(cast_uchar(*s))) /* read exponent */
			exp1 = exp1 * 10 + *(s++) - '0';
		if (neg1)
			exp1 = -exp1;
		e += exp1;
		*endptr = (char *)s; /* valid up to here */
	}
	if (neg)
		r = -r;
	return (lua_Number)ldexp(r, e);
}

#endif
/* }====================================================== */

/* maximum length of a numeral */
#if !defined(L_MAXLENNUM)
#define L_MAXLENNUM 200
#endif

static const char *l_str2dloc(const char *s, lua_Number *result, int mode)
{
	char *endptr;
	*result = (mode == 'x') ? lua_strx2number(s, &endptr) /* try to convert */
				: lua_str2number(s, &endptr);
	if (endptr == s)
		return NULL; /* nothing recognized? */
	while (lisspace(cast_uchar(*endptr)))
		endptr++;			  /* skip trailing spaces */
	return (*endptr == '\0') ? endptr : NULL; /* OK if no trailing characters */
}

/*
** Convert string 's' to a Lua number (put in 'result'). Return NULL
** on fail or the address of the ending '\0' on success.
** 'pmode' points to (and 'mode' contains) special things in the string:
** - 'x'/'X' means an hexadecimal numeral
** - 'n'/'N' means 'inf' or 'nan' (which should be rejected)
** - '.' just optimizes the search for the common case (nothing special)
** This function accepts both the current locale or a dot as the radix
** mark. If the convertion fails, it may mean number has a dot but
** locale accepts something else. In that case, the code copies 's'
** to a buffer (because 's' is read-only), changes the dot to the
** current locale radix mark, and tries to convert again.
*/
static const char *l_str2d(const char *s, lua_Number *result)
{
	const char *endptr;
	const char *pmode = strpbrk(s, ".xXnN");
	int mode = pmode ? ltolower(cast_uchar(*pmode)) : 0;
	if (mode == 'n') /* reject 'inf' and 'nan' */
		return NULL;
	endptr = l_str2dloc(s, result, mode); /* try to convert */
	if (endptr == NULL) {		      /* failed? may be a different locale */
		char buff[L_MAXLENNUM + 1];
		const char *pdot = strchr(s, '.');
		if (strlen(s) > L_MAXLENNUM || pdot == NULL)
			return NULL;			  /* string too long or no dot; fail */
		strcpy(buff, s);			  /* copy string to buffer */
		buff[pdot - s] = lua_getlocaledecpoint(); /* correct decimal point */
		endptr = l_str2dloc(buff, result, mode);  /* try again */
		if (endptr != NULL)
			endptr = s + (endptr - buff); /* make relative to 's' */
	}
	return endptr;
}

#define MAXBY10 cast(lua_Unsigned, LUA_MAXINTEGER / 10)
#define MAXLASTD cast_int(LUA_MAXINTEGER % 10)

static const char *l_str2int(const char *s, lua_Integer *result)
{
	lua_Unsigned a = 0;
	int empty = 1;
	int neg;
	while (lisspace(cast_uchar(*s)))
		s++; /* skip initial spaces */
	neg = isneg(&s);
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { /* hex? */
		s += 2;					   /* skip '0x' */
		for (; lisxdigit(cast_uchar(*s)); s++) {
			a = a * 16 + luaO_hexavalue(*s);
			empty = 0;
		}
	} else { /* decimal */
		for (; lisdigit(cast_uchar(*s)); s++) {
			int d = *s - '0';
			if (a >= MAXBY10 && (a > MAXBY10 || d > MAXLASTD + neg)) /* overflow? */
				return NULL;					 /* do not accept it (as integer) */
			a = a * 10 + d;
			empty = 0;
		}
	}
	while (lisspace(cast_uchar(*s)))
		s++; /* skip trailing spaces */
	if (empty || *s != '\0')
		return NULL; /* something wrong in the numeral */
	else {
		*result = l_castU2S((neg) ? 0u - a : a);
		return s;
	}
}

struct konst {
	uint8_t type;
	union {
		lua_Integer i;
		lua_Number n;
	};
};

size_t luaO_str2num(const char *s, struct konst *o)
{
	lua_Integer i;
	lua_Number n;
	const char *e;
	if ((e = l_str2int(s, &i)) != NULL) { /* try as an integer */
		o->i = i;
		o->type = 1;
	} else if ((e = l_str2d(s, &n)) != NULL) { /* else try as a float */
		o->n = n;
		o->type = 2;
	} else
		return 0;   /* conversion failed */
	return (e - s) + 1; /* success; return string size */
}

/* LUA_NUMBER */
/*
** this function is quite liberal in what it accepts, as 'luaO_str2num'
** will reject ill-formed numerals.
*/
static int read_numeral(LexState *ls, SemInfo *seminfo)
{
	struct konst obj;
	const char *expo = "Ee";
	int first = ls->current;
	assert(lisdigit(ls->current));
	save_and_next(ls);
	if (first == '0' && check_next2(ls, "xX")) /* hexadecimal? */
		expo = "Pp";
	for (;;) {
		if (check_next2(ls, expo))     /* exponent part? */
			check_next2(ls, "-+"); /* optional exponent sign */
		if (lisxdigit(ls->current))
			save_and_next(ls);
		else if (ls->current == '.')
			save_and_next(ls);
		else
			break;
	}
	save(ls, '\0');
	if (luaO_str2num(luaZ_buffer(ls->buff), &obj) == 0) /* format error? */
		lexerror(ls, "malformed number", TK_FLT);
	if (obj.type == 1) {
		seminfo->i = obj.i;
		return TK_INT;
	} else {
		assert(obj.type == 2);
		seminfo->r = obj.n;
		return TK_FLT;
	}
}

/*
** skip a sequence '[=*[' or ']=*]'; if sequence is well formed, return
** its number of '='s; otherwise, return a negative number (-1 iff there
** are no '='s after initial bracket)
*/
static int skip_sep(LexState *ls)
{
	int count = 0;
	int s = ls->current;
	assert(s == '[' || s == ']');
	save_and_next(ls);
	while (ls->current == '=') {
		save_and_next(ls);
		count++;
	}
	return (ls->current == s) ? count : (-count) - 1;
}

static void read_long_string(LexState *ls, SemInfo *seminfo, int sep)
{
	int line = ls->linenumber; /* initial line (for error message) */
	save_and_next(ls);	   /* skip 2nd '[' */
	if (currIsNewline(ls))	   /* string starts with a newline? */
		inclinenumber(ls); /* skip it */
	for (;;) {
		switch (ls->current) {
		case EOZ: { /* error */
			const char *what = (seminfo ? "string" : "comment");
			const char *msg = "";
			//			    luaO_pushfstring(ls->L, "unfinished long %s (starting at line %d)",
			//what, line);
			lexerror(ls, msg, TK_EOS);
			break; /* to avoid warnings */
		}
		case ']': {
			if (skip_sep(ls) == sep) {
				save_and_next(ls); /* skip 2nd ']' */
				goto endloop;
			}
			break;
		}
		case '\n':
		case '\r': {
			save(ls, '\n');
			inclinenumber(ls);
			if (!seminfo)
				luaZ_resetbuffer(ls->buff); /* avoid wasting space */
			break;
		}
		default: {
			if (seminfo)
				save_and_next(ls);
			else
				next(ls);
		}
		}
	}
endloop:
	if (seminfo)
		seminfo->ts =
		    luaX_newstring(ls, luaZ_buffer(ls->buff) + (2 + sep), luaZ_bufflen(ls->buff) - 2 * (2 + sep));
}

static void esccheck(LexState *ls, int c, const char *msg)
{
	if (!c) {
		if (ls->current != EOZ)
			save_and_next(ls); /* add current to buffer for error message */
		lexerror(ls, msg, TK_STRING);
	}
}

static int gethexa(LexState *ls)
{
	save_and_next(ls);
	esccheck(ls, lisxdigit(ls->current), "hexadecimal digit expected");
	return luaO_hexavalue(ls->current);
}

static int readhexaesc(LexState *ls)
{
	int r = gethexa(ls);
	r = (r << 4) + gethexa(ls);
	luaZ_buffremove(ls->buff, 2); /* remove saved chars from buffer */
	return r;
}

// static unsigned long readutf8esc (LexState *ls) {
//	unsigned long r;
//	int i = 4;  /* chars to be removed: '\', 'u', '{', and first digit */
//	save_and_next(ls);  /* skip 'u' */
//	esccheck(ls, ls->current == '{', "missing '{'");
//	r = gethexa(ls);  /* must have at least one digit */
//	while ((save_and_next(ls), lisxdigit(ls->current))) {
//		i++;
//		r = (r << 4) + luaO_hexavalue(ls->current);
//		esccheck(ls, r <= 0x10FFFF, "UTF-8 value too large");
//	}
//	esccheck(ls, ls->current == '}', "missing '}'");
//	next(ls);  /* skip '}' */
//	luaZ_buffremove(ls->buff, i);  /* remove saved chars from buffer */
//	return r;
//}
//
//
// static void utf8esc (LexState *ls) {
//	char buff[UTF8BUFFSZ];
//	int n = luaO_utf8esc(buff, readutf8esc(ls));
//	for (; n > 0; n--)  /* add 'buff' to string */
//		save(ls, buff[UTF8BUFFSZ - n]);
//}

static int readdecesc(LexState *ls)
{
	int i;
	int r = 0;					   /* result accumulator */
	for (i = 0; i < 3 && lisdigit(ls->current); i++) { /* read up to 3 digits */
		r = 10 * r + ls->current - '0';
		save_and_next(ls);
	}
	esccheck(ls, r <= UCHAR_MAX, "decimal escape too large");
	luaZ_buffremove(ls->buff, i); /* remove read digits from buffer */
	return r;
}

static void read_string(LexState *ls, int del, SemInfo *seminfo)
{
	save_and_next(ls); /* keep delimiter (for error messages) */
	while (ls->current != del) {
		switch (ls->current) {
		case EOZ:
			lexerror(ls, "unfinished string", TK_EOS);
			break; /* to avoid warnings */
		case '\n':
		case '\r':
			lexerror(ls, "unfinished string", TK_STRING);
			break;		   /* to avoid warnings */
		case '\\': {		   /* escape sequences */
			int c;		   /* final character to be saved */
			save_and_next(ls); /* keep '\\' for error messages */
			switch (ls->current) {
			case 'a':
				c = '\a';
				goto read_save;
			case 'b':
				c = '\b';
				goto read_save;
			case 'f':
				c = '\f';
				goto read_save;
			case 'n':
				c = '\n';
				goto read_save;
			case 'r':
				c = '\r';
				goto read_save;
			case 't':
				c = '\t';
				goto read_save;
			case 'v':
				c = '\v';
				goto read_save;
			case 'x':
				c = readhexaesc(ls);
				goto read_save;
			// TODO - FIXME
			// case 'u': utf8esc(ls);  goto no_save;
			case '\n':
			case '\r':
				inclinenumber(ls);
				c = '\n';
				goto only_save;
			case '\\':
			case '\"':
			case '\'':
				c = ls->current;
				goto read_save;
			case EOZ:
				goto no_save;		      /* will raise an error next loop */
			case 'z': {			      /* zap following span of spaces */
				luaZ_buffremove(ls->buff, 1); /* remove '\\' */
				next(ls);		      /* skip the 'z' */
				while (lisspace(ls->current)) {
					if (currIsNewline(ls))
						inclinenumber(ls);
					else
						next(ls);
				}
				goto no_save;
			}
			default: {
				esccheck(ls, lisdigit(ls->current), "invalid escape sequence");
				c = readdecesc(ls); /* digital escape '\ddd' */
				goto only_save;
			}
			}
		read_save:
			next(ls);
		/* go through */
		only_save:
			luaZ_buffremove(ls->buff, 1); /* remove '\\' */
			save(ls, c);
		/* go through */
		no_save:
			break;
		}
		default:
			save_and_next(ls);
		}
	}
	save_and_next(ls); /* skip delimiter */
	seminfo->ts = luaX_newstring(ls, luaZ_buffer(ls->buff) + 1, luaZ_bufflen(ls->buff) - 2);
}

/*
** RAVI extension: generate a token for the cast operators -
** @number, @number[], @integer, @integer[], @table
*/
static int casttoken(LexState *ls, SemInfo *seminfo)
{
	size_t n = luaZ_bufflen(ls->buff);
	const char *s = luaZ_buffer(ls->buff);
	int tok;

	/* @integer or @integer[] */
	if (strncmp(s, "@integer", n) == 0)
		tok = TK_TO_INTEGER;
	else if (strncmp(s, "@integer[]", n) == 0)
		tok = TK_TO_INTARRAY;
	/* @number or @number[] */
	else if (strncmp(s, "@number", n) == 0)
		tok = TK_TO_NUMBER;
	else if (strncmp(s, "@number[]", n) == 0)
		tok = TK_TO_NUMARRAY;
	/* @table */
	else if (strncmp(s, "@table", n) == 0)
		tok = TK_TO_TABLE;
	else if (strncmp(s, "@string", n) == 0)
		tok = TK_TO_STRING;
	else if (strncmp(s, "@closure", n) == 0)
		tok = TK_TO_CLOSURE;
	else {
		seminfo->ts = luaX_newstring(ls, s + 1, n - 1); /* omit @ */
		tok = '@';
	}
	luaZ_buffremove(ls->buff, n); /* rewind but buffer still holds the saved characters */
	return tok;
}

static int llex(LexState *ls, SemInfo *seminfo)
{
	luaZ_resetbuffer(ls->buff);
	for (;;) {
		switch (ls->current) {
		case '\n':
		case '\r': { /* line breaks */
			inclinenumber(ls);
			break;
		}
		case ' ':
		case '\f':
		case '\t':
		case '\v': { /* spaces */
			next(ls);
			break;
		}
		case '-': { /* '-' or '--' (comment) */
			next(ls);
			if (ls->current != '-')
				return '-';
			/* else is a comment */
			next(ls);
			if (ls->current == '[') { /* long comment? */
				int sep = skip_sep(ls);
				luaZ_resetbuffer(ls->buff); /* 'skip_sep' may dirty the buffer */
				if (sep >= 0) {
					read_long_string(ls, NULL, sep); /* skip long comment */
					luaZ_resetbuffer(ls->buff);	 /* previous call may dirty the buff. */
					break;
				}
			}
			/* else short comment */
			while (!currIsNewline(ls) && ls->current != EOZ)
				next(ls); /* skip until end of line (or end of file) */
			break;
		}
		case '[': { /* long string or simply '[' */
			int sep = skip_sep(ls);
			if (sep >= 0) {
				read_long_string(ls, seminfo, sep);
				return TK_STRING;
			} else if (sep != -1) /* '[=...' missing second bracket */
				lexerror(ls, "invalid long string delimiter", TK_STRING);
			return '[';
		}
		case '=': {
			next(ls);
			if (check_next1(ls, '='))
				return TK_EQ;
			else
				return '=';
		}
		case '<': {
			next(ls);
			if (check_next1(ls, '='))
				return TK_LE;
			else if (check_next1(ls, '<'))
				return TK_SHL;
			else
				return '<';
		}
		case '>': {
			next(ls);
			if (check_next1(ls, '='))
				return TK_GE;
			else if (check_next1(ls, '>'))
				return TK_SHR;
			else
				return '>';
		}
		case '/': {
			next(ls);
			if (check_next1(ls, '/'))
				return TK_IDIV;
			else
				return '/';
		}
		case '~': {
			next(ls);
			if (check_next1(ls, '='))
				return TK_NE;
			else
				return '~';
		}
		case ':': {
			next(ls);
			if (check_next1(ls, ':'))
				return TK_DBCOLON;
			else
				return ':';
		}
		case '"':
		case '\'': { /* short literal strings */
			read_string(ls, ls->current, seminfo);
			return TK_STRING;
		}
		case '.': { /* '.', '..', '...', or number */
			save_and_next(ls);
			if (check_next1(ls, '.')) {
				if (check_next1(ls, '.'))
					return TK_DOTS; /* '...' */
				else
					return TK_CONCAT; /* '..' */
			} else if (!lisdigit(ls->current))
				return '.';
			else
				return read_numeral(ls, seminfo);
		}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			return read_numeral(ls, seminfo);
		}
		case EOZ: {
			return TK_EOS;
		}
		case '@': {
			/* RAVI change: @ introduces a type assertion operator */
			save_and_next(ls);
			while (lislalnum(ls->current)) {
				save_and_next(ls);
			}
			check_save_next1(ls, '[');
			check_save_next1(ls, ']');
			return casttoken(ls, seminfo);
		}
		default: {
			if (lislalpha(ls->current)) { /* identifier or reserved word? */
				const char *ts;
				do {
					save_and_next(ls);
				} while (lislalnum(ls->current));
				ts = raviX_create_string(ls->container, luaZ_buffer(ls->buff), luaZ_bufflen(ls->buff));
				seminfo->ts = ts;
				int tok = is_reserved(ts);
				if (tok != -1) /* reserved word? */
					return tok + FIRST_RESERVED;
				else {
					return TK_NAME;
				}
			} else { /* single-char tokens (+ - / ...) */
				int c = ls->current;
				next(ls);
				return c;
			}
		}
		}
	}
}

void raviX_next(LexState *ls)
{
	ls->lastline = ls->linenumber;
	if (ls->lookahead.token != TK_EOS) {  /* is there a look-ahead token? */
		ls->t = ls->lookahead;	      /* use this one */
		ls->lookahead.token = TK_EOS; /* and discharge it */
	} else
		ls->t.token = llex(ls, &ls->t.seminfo); /* read next token */
}

int raviX_lookahead(LexState *ls)
{
	assert(ls->lookahead.token == TK_EOS);
	ls->lookahead.token = llex(ls, &ls->lookahead.seminfo);
	return ls->lookahead.token;
}
