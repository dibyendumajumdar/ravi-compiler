#include <chibicc.h>

#include <string.h>

int main(int argc, const char *argv[])
{
	char buffer[100] = {0};
	const char *code = "" \
 	"int main() {\n" \
	" return 0;\n" \
	"}\n";
	strncpy(buffer, code, sizeof buffer);
	C_parser parser = {0};
	Token *tok = tokenize_buffer(&parser, buffer);
	convert_pp_tokens(&parser, tok);
	Scope scope = {0};
	Obj *obj = parse(&scope, &parser, tok);
	return 0;
}