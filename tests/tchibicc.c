#include <chibicc.h>

#include <string.h>

static void printout(char *key, int keylen, void *val)
{
	printf("name: %d %.*s\n", keylen, keylen, key);
}

int main(int argc, const char *argv[])
{
	char buffer[100] = {0};
	char buffer2[100] = {0};
	const char *code = "" \
 	"typedef struct {\n" \
	" char *data;\n" \
	" int len;\n" \
	"} Str;\n";
	strncpy(buffer, code, sizeof buffer);
	C_parser parser = {0};
	C_parser_init(&parser);
	C_Token *tok = C_tokenize_buffer(&parser, buffer);
	C_convert_pp_tokens(&parser, tok);
	C_Scope scope = {0};
	scope.vars.arena = parser.arena;
	scope.tags.arena = parser.arena;
	C_Obj *obj = C_parse(&scope, &parser, tok);
	hashmap_foreach(&scope.vars, printout);
	C_create_function(&scope, &parser, "dummy");
	hashmap_foreach(&scope.vars, printout);
	const char *snippet = "{ Str s; s.data = \"hello world\"; s.len = sizeof \"hello world\"; }\n";
	strncpy(buffer2, snippet, sizeof buffer2);
	tok = C_tokenize_buffer(&parser, buffer2);
	C_convert_pp_tokens(&parser, tok);
	parser.embedded_mode = true;
	C_Node *node = C_parse_compound_statement(&scope, &parser, tok);
	C_parser_destroy(&parser);
	return 0;
}