#include <ravi_ast.h>

int main(int argc, const char *argv[]) {

	struct compiler_state *container = raviX_new_ast_container();
	int rc = 0;

	const char *s1 = "Test String";
	const char *s2 = raviX_create_string(container, s1, strlen(s1));
	if (strcmp(s1, s2) != 0)
		rc++;
	if (s1 == s2)
		rc++;
	const char *s3 = raviX_create_string(container, s1, strlen(s1));
	if (s3 != s2)
		rc++;
	if (strcmp(s1, s3) != 0)
		rc++;
	raviX_destroy_ast_container(container);

	return rc;
}
