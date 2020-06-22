#include <allocate.h>
#include <ravi_compiler.h>

#include <string.h>
#include <bitset.h>

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

int test_bitmap()
{
	int status;
	bitmap_t b1, b2, b3, b4;

	b1 = raviX_bitmap_create();
	b2 = raviX_bitmap_create();
	b3 = raviX_bitmap_create();
	b4 = raviX_bitmap_create();
	status = raviX_bitmap_empty_p(b1);
	status &= raviX_bitmap_bit_count(b1) == 0;

	status &= raviX_bitmap_set_bit_p(b1, 1);
	status &= raviX_bitmap_set_bit_p(b1, 120);
	status &= !raviX_bitmap_set_bit_p(b1, 120);
	status &= !raviX_bitmap_empty_p(b1);
	status &= raviX_bitmap_bit_p(b1, 1);
	status &= raviX_bitmap_bit_p(b1, 120);
	status &= !raviX_bitmap_bit_p(b1, 42);

	status &= raviX_bitmap_clear_bit_p(b1, 120);
	status &= !raviX_bitmap_bit_p(b1, 120);
	status &= raviX_bitmap_set_bit_p(b1, 120);

	raviX_bitmap_copy(b2, b1);
	status &= raviX_bitmap_equal_p(b1, b2);
	status &= raviX_bitmap_intersect_p(b1, b2);
	status &= !raviX_bitmap_equal_p(b1, b3);
	status &= !raviX_bitmap_intersect_p(b1, b3);

	raviX_bitmap_clear(b2);
	status &= raviX_bitmap_empty_p(b2);
	status &= raviX_bitmap_bit_count(b2) == 0;

	raviX_bitmap_copy(b2, b1);
	status &= raviX_bitmap_equal_p(b1, b2);
	status &= raviX_bitmap_set_bit_p(b2, 1818);

	status &= raviX_bitmap_set_bit_p(b3, 555);
	status &= raviX_bitmap_set_bit_p(b3, 120);
	status &= raviX_bitmap_set_bit_p(b3, 42);
	status &= !raviX_bitmap_empty_p(b3);
	status &= raviX_bitmap_bit_count(b3) == 3;
	status &= raviX_bitmap_bit_p(b3, 555);
	status &= raviX_bitmap_bit_p(b3, 120);
	status &= raviX_bitmap_bit_p(b3, 42);

	status &= raviX_bitmap_and(b4, b1, b2);
	status &= raviX_bitmap_equal_p(b4, b1);

	status &= raviX_bitmap_ior(b4, b1, b2);
	status &= raviX_bitmap_equal_p(b4, b2);

	status &= raviX_bitmap_and_compl(b4, b2, b1);
	status &= raviX_bitmap_bit_p(b4, 1818);
	status &= raviX_bitmap_bit_count(b4) == 1;

	status &= raviX_bitmap_and_compl(b4, b1, b2);
	status &= raviX_bitmap_bit_count(b4) == 0;

	status &= raviX_bitmap_ior_and(b4, b1, b2, b3);
	status &= raviX_bitmap_bit_p(b4, 1);
	status &= raviX_bitmap_bit_p(b4, 120);
	status &= raviX_bitmap_bit_count(b4) == 2;

	status &= raviX_bitmap_ior_and(b4, b3, b1, b2);
	status &= raviX_bitmap_bit_p(b4, 1);
	status &= raviX_bitmap_bit_p(b4, 555);
	status &= raviX_bitmap_bit_p(b4, 42);
	status &= raviX_bitmap_bit_p(b4, 120);
	status &= raviX_bitmap_bit_count(b4) == 4;

	status &= raviX_bitmap_ior_and_compl(b4, b1, b2, b3);
	status &= raviX_bitmap_bit_p(b4, 1);
	status &= raviX_bitmap_bit_p(b4, 1818);
	status &= raviX_bitmap_bit_p(b4, 120);
	status &= raviX_bitmap_bit_count(b4) == 3;

	status &= raviX_bitmap_ior_and_compl(b3, b1, b2, b3);
	status &= raviX_bitmap_bit_p(b3, 1);
	status &= raviX_bitmap_bit_p(b3, 1818);
	status &= raviX_bitmap_bit_p(b3, 120);
	status &= raviX_bitmap_bit_count(b3) == 3;

	raviX_bitmap_clear(b1);
	status &= raviX_bitmap_set_bit_range_p(b1, 1, 62);
	for (int i = 1; i <= 62; i++) status &= raviX_bitmap_clear_bit_p(b1, i);
	status &= raviX_bitmap_empty_p(b1);

	status &= raviX_bitmap_set_bit_range_p(b1, 30, 362);
	for (int i = 30; i < 362 + 30; i++) status &= raviX_bitmap_clear_bit_p(b1, i);
	status &= raviX_bitmap_empty_p(b1);

	status &= raviX_bitmap_set_bit_range_p(b1, 1, 62);
	status &= raviX_bitmap_clear_bit_range_p(b1, 1, 62);
	status &= raviX_bitmap_empty_p(b1);

	status &= raviX_bitmap_set_bit_range_p(b1, 30, 362);
	status &= raviX_bitmap_clear_bit_range_p(b1, 30, 362);
	status &= raviX_bitmap_empty_p(b1);

	status &= raviX_bitmap_set_bit_range_p(b1, 30, 362);

	bitmap_iterator_t iter;
	size_t nb = 0, n = 0, nmax = 0, nmin = 10000;
	FOREACH_BITMAP_BIT(iter, b1, nb)
	{
		n++;
		if (nmax < nb) nmax = nb;
		if (nmin > nb) nmin = nb;
	}
	status &= n == 362;
	status &= nmin == 30 && nmax == 391;

	fprintf (stderr, status ? "BITMAP OK\n" : "BITMAP FAILURE!\n");
	raviX_bitmap_destroy(b1);
	raviX_bitmap_destroy(b2);
	raviX_bitmap_destroy(b3);
	raviX_bitmap_destroy(b4);
	return !status;
}

int main(int argc, const char *argv[])
{
	int rc = test_stringset();
	rc += test_memalloc();
	rc += test_bitmap();
	if (rc == 0)
		printf("Ok\n");
	else
		printf("FAILED\n");
	return rc != 0 ? 1 : 0;
}
