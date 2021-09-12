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
	C_Token *tok = tokenize_buffer(&parser, buffer);
	C_convert_pp_tokens(&parser, tok);
	C_Scope scope = {0};
	C_Obj *obj = parse(&scope, &parser, tok);
	hashmap_foreach(&scope.vars, printout);
	create_function(&scope, &parser, "dummy");
	hashmap_foreach(&scope.vars, printout);
	const char *snippet = "{ Str s; s.data = \"hello world\"; s.len = sizeof \"hello world\"; }\n";
	strncpy(buffer2, snippet, sizeof buffer2);
	tok = tokenize_buffer(&parser, buffer2);
	C_convert_pp_tokens(&parser, tok);
	parser.allow_partial_parsing = true;
	C_Node *node = parse_compound_statement(&scope, &parser, tok);
	return 0;
}