#include <chibicc/chibicc.h>

#include <string.h>

static void create_allocator(C_MemoryAllocator *allocator) {
	allocator->arena = create_mspace(0, 0);
	allocator->realloc = mspace_realloc;
	allocator->calloc = mspace_calloc;
	allocator->free = mspace_free;
	allocator->create_arena = create_mspace;
	allocator->destroy_arena = destroy_mspace;
}

static void destroy_allocator(C_MemoryAllocator *allocator) {
	allocator->destroy_arena(allocator->arena);
}

static void printout(void *userdata, char *key, int keylen, void *val)
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
	C_MemoryAllocator allocator;
	create_allocator(&allocator);
	C_Parser parser = {0};
	C_parser_init(&parser, &allocator);
	C_Token *tok = C_tokenize_buffer(&parser, buffer);
	C_convert_pp_tokens(&parser, tok);
	C_Scope scope = {0};
	scope.vars.allocator = parser.memory_allocator;
	scope.tags.allocator = parser.memory_allocator;
	C_Obj *obj = C_parse(&scope, &parser, tok);
	hashmap_foreach(&scope.vars, printout, NULL);
	C_create_function(&scope, &parser, "dummy");
	hashmap_foreach(&scope.vars, printout, NULL);
	const char *snippet = "{ Str s; s.data = \"hello world\"; s.len = sizeof \"hello world\"; }\n";
	strncpy(buffer2, snippet, sizeof buffer2);
	tok = C_tokenize_buffer(&parser, buffer2);
	C_convert_pp_tokens(&parser, tok);
	parser.embedded_mode = true;
	C_Node *node = C_parse_compound_statement(&scope, &parser, tok);
	C_parser_destroy(&parser);
	destroy_allocator(&allocator);
	return 0;
}