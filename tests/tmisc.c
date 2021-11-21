/******************************************************************************
 * Copyright (C) 2020-2021 Dibyendu Majumdar
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include <allocate.h>
#include <ravi_compiler.h>

#include <string.h>
#include <bitset.h>

#include "ravi_alloc.h"

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

static int test_stringset(void)
{
	C_MemoryAllocator allocator;
	create_allocator(&allocator);
	CompilerState *compiler_state = raviX_init_compiler(&allocator);
	int rc = 0;

	const char *s1 = "local";
	const StringObject *s2 = raviX_create_string(compiler_state, s1, (uint32_t)strlen(s1));
	if (strcmp(s1, s2->str) != 0)
		rc++;
	if (s1 == s2->str)
		rc++;
	const StringObject *s3 = raviX_create_string(compiler_state, s1, (uint32_t)strlen(s1));
	if (s3 != s2)
		rc++;
	if (strcmp(s1, s3->str) != 0)
		rc++;
	if (s2->hash != s3->hash)
		rc++;
	raviX_destroy_compiler(compiler_state);
	destroy_allocator(&allocator);
	return rc;
}

static int test_memalloc(void)
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

static int test_bitset(void)
{
	int status;
	BitSet b1, b2, b3, b4;

	raviX_bitset_create(&b1);
	raviX_bitset_create(&b2);
	raviX_bitset_create(&b3);
	raviX_bitset_create(&b4);
	status = raviX_bitset_empty_p(&b1);
	status &= raviX_bitset_bit_count(&b1) == 0;

	status &= raviX_bitset_set_bit_p(&b1, 1);
	status &= raviX_bitset_set_bit_p(&b1, 120);
	status &= !raviX_bitset_set_bit_p(&b1, 120);
	status &= !raviX_bitset_empty_p(&b1);
	status &= raviX_bitset_bit_p(&b1, 1);
	status &= raviX_bitset_bit_p(&b1, 120);
	status &= !raviX_bitset_bit_p(&b1, 42);

	status &= raviX_bitset_clear_bit_p(&b1, 120);
	status &= !raviX_bitset_bit_p(&b1, 120);
	status &= raviX_bitset_set_bit_p(&b1, 120);

	raviX_bitset_copy(&b2, &b1);
	status &= raviX_bitset_equal_p(&b1, &b2);
	status &= raviX_bitset_intersect_p(&b1, &b2);
	status &= !raviX_bitset_equal_p(&b1, &b3);
	status &= !raviX_bitset_intersect_p(&b1, &b3);

	raviX_bitset_clear(&b2);
	status &= raviX_bitset_empty_p(&b2);
	status &= raviX_bitset_bit_count(&b2) == 0;

	raviX_bitset_copy(&b2, &b1);
	status &= raviX_bitset_equal_p(&b1, &b2);
	status &= raviX_bitset_set_bit_p(&b2, 1818);

	status &= raviX_bitset_set_bit_p(&b3, 555);
	status &= raviX_bitset_set_bit_p(&b3, 120);
	status &= raviX_bitset_set_bit_p(&b3, 42);
	status &= !raviX_bitset_empty_p(&b3);
	status &= raviX_bitset_bit_count(&b3) == 3;
	status &= raviX_bitset_bit_p(&b3, 555);
	status &= raviX_bitset_bit_p(&b3, 120);
	status &= raviX_bitset_bit_p(&b3, 42);

	status &= raviX_bitset_and(&b4, &b1, &b2);
	status &= raviX_bitset_equal_p(&b4, &b1);

	status &= raviX_bitset_ior(&b4, &b1, &b2);
	status &= raviX_bitset_equal_p(&b4, &b2);

	status &= raviX_bitset_and_compl(&b4, &b2, &b1);
	status &= raviX_bitset_bit_p(&b4, 1818);
	status &= raviX_bitset_bit_count(&b4) == 1;

	status &= raviX_bitset_and_compl(&b4, &b1, &b2);
	status &= raviX_bitset_bit_count(&b4) == 0;

	status &= raviX_bitset_ior_and(&b4, &b1, &b2, &b3);
	status &= raviX_bitset_bit_p(&b4, 1);
	status &= raviX_bitset_bit_p(&b4, 120);
	status &= raviX_bitset_bit_count(&b4) == 2;

	status &= raviX_bitset_ior_and(&b4, &b3, &b1, &b2);
	status &= raviX_bitset_bit_p(&b4, 1);
	status &= raviX_bitset_bit_p(&b4, 555);
	status &= raviX_bitset_bit_p(&b4, 42);
	status &= raviX_bitset_bit_p(&b4, 120);
	status &= raviX_bitset_bit_count(&b4) == 4;

	status &= raviX_bitset_ior_and_compl(&b4, &b1, &b2, &b3);
	status &= raviX_bitset_bit_p(&b4, 1);
	status &= raviX_bitset_bit_p(&b4, 1818);
	status &= raviX_bitset_bit_p(&b4, 120);
	status &= raviX_bitset_bit_count(&b4) == 3;

	status &= raviX_bitset_ior_and_compl(&b3, &b1, &b2, &b3);
	status &= raviX_bitset_bit_p(&b3, 1);
	status &= raviX_bitset_bit_p(&b3, 1818);
	status &= raviX_bitset_bit_p(&b3, 120);
	status &= raviX_bitset_bit_count(&b3) == 3;

	raviX_bitset_clear(&b1);
	status &= raviX_bitset_set_bit_range_p(&b1, 1, 62);
	for (int i = 1; i <= 62; i++) status &= raviX_bitset_clear_bit_p(&b1, i);
	status &= raviX_bitset_empty_p(&b1);

	status &= raviX_bitset_set_bit_range_p(&b1, 30, 362);
	for (int i = 30; i < 362 + 30; i++) status &= raviX_bitset_clear_bit_p(&b1, i);
	status &= raviX_bitset_empty_p(&b1);

	status &= raviX_bitset_set_bit_range_p(&b1, 1, 62);
	status &= raviX_bitset_clear_bit_range_p(&b1, 1, 62);
	status &= raviX_bitset_empty_p(&b1);

	status &= raviX_bitset_set_bit_range_p(&b1, 30, 362);
	status &= raviX_bitset_clear_bit_range_p(&b1, 30, 362);
	status &= raviX_bitset_empty_p(&b1);

	status &= raviX_bitset_set_bit_range_p(&b1, 30, 362);

	BitSetIterator iter;
	size_t nb = 0, n = 0, nmax = 0, nmin = 10000;
	FOREACH_BITSET_BIT(iter, &b1, nb)
	{
		n++;
		if (nmax < nb) nmax = nb;
		if (nmin > nb) nmin = nb;
	}
	status &= n == 362;
	status &= nmin == 30 && nmax == 391;

	fprintf (stderr, status ? "BITSET OK\n" : "BITSET FAILURE!\n");
	raviX_bitset_destroy(&b1);
	raviX_bitset_destroy(&b2);
	raviX_bitset_destroy(&b3);
	raviX_bitset_destroy(&b4);
	return !status;
}

int main(int argc, const char *argv[])
{
	int rc = test_stringset();
	rc += test_memalloc();
	rc += test_bitset();
	if (rc == 0)
		printf("Ok\n");
	else
		printf("FAILED\n");
	return rc != 0 ? 1 : 0;
}
