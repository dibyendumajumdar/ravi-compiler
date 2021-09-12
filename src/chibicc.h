#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _WIN32
#include <glob.h>
#include <libgen.h>
#include <strings.h>
#include <stdnoreturn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define RAVI_EXTENSIONS

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#ifndef __GNUC__
# define __attribute__(x)
#endif
#ifdef _WIN32
# define noreturn
# define strncasecmp strnicmp
# define strndup _strndup
#endif

typedef struct C_Type C_Type;
typedef struct C_Node C_Node;
typedef struct C_Member C_Member;
typedef struct C_Relocation C_Relocation;
typedef struct C_Hideset C_Hideset;
typedef struct C_parser C_parser;

//
// strings.c
//

typedef struct {
  char **data;
  int capacity;
  int len;
} StringArray;

void strarray_push(StringArray *arr, char *s);
char *format(char *fmt, ...) __attribute__((format(printf, 1, 2)));

//
// tokenize.c
//

// Token
typedef enum {
  TK_IDENT,   // Identifiers
  TK_PUNCT,   // Punctuators
  TK_KEYWORD, // Keywords
  TK_STR,     // String literals
  TK_NUM,     // Numeric literals
  TK_PP_NUM,  // Preprocessing numbers
  TK_EOF,     // End-of-file markers
} C_TokenKind;

typedef struct {
  char *name;
  int file_no;
  char *contents;

  // For #line directive
  char *display_name;
  int line_delta;
} C_File;

// Token type
typedef struct Token Token;
struct Token {
	C_TokenKind kind;   // Token kind
  Token *next;      // Next token
  int64_t val;      // If kind is TK_NUM, its value
  long double fval; // If kind is TK_NUM, its value
  char *loc;        // Token location
  int len;          // Token length
  C_Type *ty;         // Used if TK_NUM or TK_STR
  char *str;        // String literal contents including terminating '\0'

  C_File *file;       // Source location
  char *filename;   // Filename
  int line_no;      // Line number
  int line_delta;   // Line number
  bool at_bol;      // True if this token is at beginning of line
  bool has_space;   // True if this token follows a space character
  C_Hideset *hideset; // For macro expansion
  Token *origin;    // If this is expanded from a macro, the original token
};

noreturn void error(char *fmt, ...) __attribute__((format(printf, 1, 2)));
noreturn void error_at(C_parser *tokenizer, char *loc, char *fmt, ...) __attribute__((format(printf, 2, 3)));
noreturn void error_tok(C_parser *tokenizer, Token *tok, char *fmt, ...) __attribute__((format(printf, 2, 3)));
void warn_tok(C_parser *tokenizer, Token *tok, char *fmt, ...) __attribute__((format(printf, 2, 3)));
bool equal(Token *tok, char *op);
Token *skip(C_parser *parser, Token *tok, char *op);
bool consume(Token **rest, Token *tok, char *str);
void convert_pp_tokens(C_parser *tokenizer, Token *tok);
C_File **get_input_files(C_parser *tokenizer);
C_File *new_file(char *name, int file_no, char *contents);
Token *tokenize_string_literal(C_parser *tokenizer, Token *tok, C_Type *basety);
Token *tokenize(C_parser *tokenizer, C_File *file);
Token *tokenize_file(C_parser *tokenizer, char *filename);
Token *tokenize_buffer(C_parser *tokenizer, char *p);

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__)

//
// preprocess.c
//

char *search_include_paths(char *filename);
void init_macros(void);
void define_macro(char *name, char *buf);
void undef_macro(char *name);
Token *preprocess(Token *tok);

//
// parse.c
//

// Variable or function
typedef struct Obj Obj;
struct Obj {
  Obj *next;
  char *name;    // Variable name
  C_Type *ty;      // Type
  Token *tok;    // representative token
  bool is_local; // local or global/function
  int align;     // alignment

  // Local variable
  int offset;

  // Global variable or function
  bool is_function;
  bool is_definition;
  bool is_static;

  // Global variable
  bool is_tentative;
  bool is_tls;
  char *init_data;
  C_Relocation *rel;

  // Function
  bool is_inline;
  Obj *params;
  C_Node *body;
  Obj *locals;
  Obj *va_area;
  Obj *alloca_bottom;
  int stack_size;

  // Static inline function
  bool is_live;
  bool is_root;
  StringArray refs;
};

// Global variable can be initialized either by a constant expression
// or a pointer to another global variable. This struct represents the
// latter.
typedef struct C_Relocation C_Relocation;
struct C_Relocation {
	C_Relocation *next;
  int offset;
  char **label;
  long addend;
};

// AST node
typedef enum {
  ND_NULL_EXPR, // Do nothing
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // unary -
  ND_MOD,       // %
  ND_BITAND,    // &
  ND_BITOR,     // |
  ND_BITXOR,    // ^
  ND_SHL,       // <<
  ND_SHR,       // >>
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_COND,      // ?:
  ND_COMMA,     // ,
  ND_MEMBER,    // . (struct member access)
  ND_ADDR,      // unary &
  ND_DEREF,     // unary *
  ND_NOT,       // !
  ND_BITNOT,    // ~
  ND_LOGAND,    // &&
  ND_LOGOR,     // ||
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_FOR,       // "for" or "while"
  ND_DO,        // "do"
  ND_SWITCH,    // "switch"
  ND_CASE,      // "case"
  ND_BLOCK,     // { ... }
  ND_GOTO,      // "goto"
  ND_GOTO_EXPR, // "goto" labels-as-values
  ND_LABEL,     // Labeled statement
  ND_LABEL_VAL, // [GNU] Labels-as-values
  ND_FUNCALL,   // Function call
  ND_EXPR_STMT, // Expression statement
  ND_STMT_EXPR, // Statement expression
  ND_VAR,       // Variable
  ND_VLA_PTR,   // VLA designator
  ND_NUM,       // Integer
  ND_CAST,      // Type cast
  ND_MEMZERO,   // Zero-clear a stack variable
  ND_ASM,       // "asm"
  ND_CAS,       // Atomic compare-and-swap
  ND_EXCH,      // Atomic exchange
} NodeKind;

// AST node type
struct C_Node {
  NodeKind kind; // C_Node kind
  C_Node *next;    // Next node
  C_Type *ty;      // Type, e.g. int or pointer to int
  Token *tok;    // Representative token

  C_Node *lhs;     // Left-hand side
  C_Node *rhs;     // Right-hand side

  // "if" or "for" statement
  C_Node *cond;
  C_Node *then;
  C_Node *els;
  C_Node *init;
  C_Node *inc;

  // "break" and "continue" labels
  char *brk_label;
  char *cont_label;

  // Block or statement expression
  C_Node *body;

  // Struct member access
  C_Member *member;

  // Function call
  C_Type *func_ty;
  C_Node *args;
  bool pass_by_stack;
  Obj *ret_buffer;

  // Goto or labeled statement, or labels-as-values
  char *label;
  char *unique_label;
  C_Node *goto_next;

  // Switch
  C_Node *case_next;
  C_Node *default_case;

  // Case
  long begin;
  long end;

  // "asm" string literal
  char *asm_str;

  // Atomic compare-and-swap
  C_Node *cas_addr;
  C_Node *cas_old;
  C_Node *cas_new;

  // Atomic op= operators
  Obj *atomic_addr;
  C_Node *atomic_expr;

  // Variable
  Obj *var;

  // Numeric literal
  int64_t val;
  long double fval;
};

typedef struct {
  char *key;
  int keylen;
  void *val;
} HashEntry;

typedef struct {
  HashEntry *buckets;
  int capacity;
  int used;
} HashMap;

// Represents a block scope.
typedef struct Scope Scope;
struct Scope {
  Scope *next;

  // C has two block scopes; one is for variables/typedefs and
  // the other is for struct/union/enum tags.
  HashMap vars;
  HashMap tags;
};

struct C_parser {
  int file_no;
  // Input file
  C_File *current_file;

  // A list of all input files.
  C_File **input_files;

  // True if the current position is at the beginning of a line
  bool at_bol;

  // True if the current position follows a space character
  bool has_space;

  // All local variable instances created during parsing are
  // accumulated to this list.
  Obj *locals;

  // Likewise, global variables are accumulated to this list.
  Obj *globals;

  Scope *scope; // = &(Scope){0};

  // Points to the function object the parser is currently parsing.
  Obj *current_fn;

  // Lists of all goto statements and labels in the curent function.
  C_Node *gotos;
  C_Node *labels;

  // Current "goto" and "continue" jump targets.
  char *brk_label;
  char *cont_label;

  // Points to a node representing a switch if we are parsing
  // a switch statement. Otherwise, NULL.
  C_Node *current_switch;

  Obj *builtin_alloca;

#ifdef RAVI_EXTENSIONS
  bool allow_partial_parsing;
#endif
};

C_Node *new_cast(C_parser *parser, C_Node *expr, C_Type *ty);
int64_t const_expr(C_parser *parser, Token **rest, Token *tok);
Obj *parse(Scope* globalScope, C_parser *parser, Token *tok);

#ifdef RAVI_EXTENSIONS
C_Node *parse_compound_statement(Scope *globalScope, C_parser *parser, Token *tok);
Obj *create_function(Scope *globalScope, C_parser *parser, char *name_str);
#endif

//
// type.c
//

typedef enum {
  TY_VOID,
  TY_BOOL,
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_FLOAT,
  TY_DOUBLE,
  TY_LDOUBLE,
  TY_ENUM,
  TY_PTR,
  TY_FUNC,
  TY_ARRAY,
  TY_VLA, // variable-length array
  TY_STRUCT,
  TY_UNION,
} TypeKind;

struct C_Type {
  TypeKind kind;
  int size;           // sizeof() value
  int align;          // alignment
  bool is_unsigned;   // unsigned or signed
  bool is_atomic;     // true if _Atomic
  C_Type *origin;       // for type compatibility check

  // Pointer-to or array-of type. We intentionally use the same member
  // to represent pointer/array duality in C.
  //
  // In many contexts in which a pointer is expected, we examine this
  // member instead of "kind" member to determine whether a type is a
  // pointer or not. That means in many contexts "array of T" is
  // naturally handled as if it were "pointer to T", as required by
  // the C spec.
  C_Type *base;

  // Declaration
  Token *name;
  Token *name_pos;

  // Array
  int array_len;

  // Variable-length array
  C_Node *vla_len; // # of elements
  Obj *vla_size; // sizeof() value

  // Struct
  C_Member *members;
  bool is_flexible;
  bool is_packed;

  // Function type
  C_Type *return_ty;
  C_Type *params;
  bool is_variadic;
  C_Type *next;
};

// Struct member
struct C_Member {
	C_Member *next;
  C_Type *ty;
  Token *tok; // for error message
  Token *name;
  int idx;
  int align;
  int offset;

  // Bitfield
  bool is_bitfield;
  int bit_offset;
  int bit_width;
};

extern C_Type *ty_void;
extern C_Type *ty_bool;

extern C_Type *ty_char;
extern C_Type *ty_short;
extern C_Type *ty_int;
extern C_Type *ty_long;

extern C_Type *ty_uchar;
extern C_Type *ty_ushort;
extern C_Type *ty_uint;
extern C_Type *ty_ulong;

extern C_Type *ty_float;
extern C_Type *ty_double;
extern C_Type *ty_ldouble;

bool is_integer(C_Type *ty);
bool is_flonum(C_Type *ty);
bool is_numeric(C_Type *ty);
bool is_compatible(C_Type *t1, C_Type *t2);
C_Type *copy_type(C_parser *parser, C_Type *ty);
C_Type *pointer_to(C_parser *parser, C_Type *base);
C_Type *func_type(C_parser *parser, C_Type *return_ty);
C_Type *array_of(C_parser *parser, C_Type *base, int size);
C_Type *vla_of(C_parser *parser, C_Type *base, C_Node *expr);
C_Type *enum_type(C_parser *parser);
C_Type *struct_type(C_parser *parser);
void add_type(C_parser *parser, C_Node *node);

//
// codegen.c
//

void codegen(Obj *prog, FILE *out);
// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static inline int align_to(int n, int align) {
	return (n + align - 1) / align * align;
}


//
// unicode.c
//

int encode_utf8(char *buf, uint32_t c);
uint32_t decode_utf8(C_parser *tokenizer, char **new_pos, char *p);
bool is_ident1(uint32_t c);
bool is_ident2(uint32_t c);
int display_width(C_parser *tokenizer, char *p, int len);

//
// hashmap.c
//

void *hashmap_get(HashMap *map, char *key);
void *hashmap_get2(HashMap *map, char *key, int keylen);
void hashmap_put(HashMap *map, char *key, void *val);
void hashmap_put2(HashMap *map, char *key, int keylen, void *val);
void hashmap_delete(HashMap *map, char *key);
void hashmap_delete2(HashMap *map, char *key, int keylen);
void hashmap_test(void);
void hashmap_foreach(HashMap *map, void (*f)(char *key, int keylen, void *val));

//
// main.c
//

bool file_exists(char *path);

extern StringArray include_paths;
extern bool opt_fpic;
extern bool opt_fcommon;
extern char *base_file;
