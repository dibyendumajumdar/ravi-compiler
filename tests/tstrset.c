#include <ravi_compiler.h>

#include <string.h>

int main(int argc, const char *argv[])
{
	struct compiler_state *container = raviX_init_compiler();
	int rc = 0;

	const char *s1 = "local";
	const struct string_object *s2 = raviX_create_string(container, s1, (uint32_t)strlen(s1));
	if (strcmp(s1, s2->str) != 0)
		rc++;
	if (s1 == s2->str)
		rc++;
	const struct string_object *s3 = raviX_create_string(container, s1, (uint32_t)strlen(s1));
	if (s3 != s2)
		rc++;
	if (strcmp(s1, s3->str) != 0)
		rc++;
	if (s2->hash != s3->hash)
		rc++;
	raviX_destroy_compiler(container);

	return rc;
}
