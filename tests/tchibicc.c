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
	Token *tok = tokenize_buffer(buffer);
	convert_pp_tokens(tok);
	Obj *obj = parse(tok);
	return 0;
}