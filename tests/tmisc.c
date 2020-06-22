#include <allocate.h>
#include <ravi_compiler.h>

#include <string.h>

int test_stringset()
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

int test_memalloc()
{
	int arry[5] = {1, 2, 3, 4, 5}; // 5 is extra sentinel
	raviX_del_array_element(arry, sizeof arry[0], 4, 1, 2);
	if (arry[0] != 1 || arry[1] != 4 || arry[2] != 0 || arry[3] != 0 || arry[4] != 5)
		return 1;
	double darry[5] = {1.1, 2.2, 3.3, 4.4, 5.5}; // 5.5 is extra sentinel
	raviX_del_array_element(darry, sizeof darry[0], 4, 1, 2);
	if (darry[0] != 1.1 || darry[1] != 4.4 || darry[2] != 0.0 || darry[3] != 0.0 || darry[4] != 5.5)
		return 1;
	return 0;
}

int main(int argc, const char *argv[])
{
	int rc = test_stringset();
	rc += test_memalloc();
	if (rc == 0)
		printf("Ok\n");
	else
		printf("FAILED\n");
	return rc != 0 ? 1 : 0;
}
