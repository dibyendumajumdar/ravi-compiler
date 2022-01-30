/******************************************************************************
 * Copyright (C) 2020-2022 Dibyendu Majumdar
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

#include <assert.h>
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

/* TODO the test below duplicates code from linearizer */

typedef struct PseudoGenerator {
	uint64_t bits[4]; /* bitset of registers */
	unsigned max_reg;
} PseudoGenerator;
static inline unsigned raviX_max_reg(PseudoGenerator *generator) { return generator->max_reg; }

#define FIELD_SIZEOF(t, f) (sizeof(((t *)0)->f))
#define FIELD_E_SIZEOF(t, f) (sizeof(((t *)0)->f[0]))

enum {
	BITSET_SIZE = FIELD_SIZEOF(PseudoGenerator, bits),    /* total size of bits in #bytes */
	BITSET_ESIZE = FIELD_E_SIZEOF(PseudoGenerator, bits), /* sizeof bits[0] in #bytes */
	N_WORDS = BITSET_SIZE / BITSET_ESIZE,		      /* bits array size in #bytes */
	ESIZE = BITSET_ESIZE * 8,			      /* bits in bits[0] */
	MAXBIT = N_WORDS * ESIZE			      /* total bits */
};

static_assert(N_WORDS == 4, "Invalid computation of bitset size"); /* must be kept in sync with PseudoGeneraotor.bits */

/* Identify the top most register allocated; this is useful when we need
 * to ensure that the next register goes to the top of the stack
 */
static int top_reg(PseudoGenerator *generator)
{
	/* start from the last element of bits */
	for (int i = N_WORDS - 1; i >= 0; i--) {
		if (!generator->bits[i]) // no bit set
			continue;
		uint64_t bit = generator->bits[i];
		int x = i * ESIZE - 1; /* x is rightmost bit in bits[i] */
		/* keep getting rid of bits until we have none left */
		while (bit != 0) {
			bit >>= 1;
			x++;
		}
		return x;
	}
	return -1;
}

/**
 * Is given register top of the stack of registers?
 */
static int pseudo_gen_is_top(PseudoGenerator *generator, unsigned reg)
{
	int top = top_reg(generator);
	if (top < 0)
		return 0;
	return top == reg;
}

/**
 * Free the given register
 */
static void pseudo_gen_free(PseudoGenerator *generator, unsigned reg)
{
	assert(reg < MAXBIT);
	unsigned n = reg / ESIZE;
	reg = reg % ESIZE;
	generator->bits[n] &= ~(1ull << reg);
}

/**
 * Allocate a register, if top is specified then ensure it is top of the
 * stack else look for a free register. Not the most efficient as we
 * don't yet use hardware intrinsics for bit scans.
 */
static unsigned pseudo_gen_alloc(PseudoGenerator *generator, bool top)
{
	unsigned reg;
	if (top) {
		int current_top = top_reg(generator);
		reg = current_top + 1;
	} else {
		/* look for the first free reg */
		reg = 0;
		int is_set = 1;
		for (int i = 0; is_set && i < N_WORDS; i++) {
			uint64_t bit = generator->bits[i];
			if (bit == ~0ull) {
				/* all bits set? skip */ 
				reg += ESIZE;
				continue;
			} 
			for (int j = 0; is_set && j < ESIZE; j++) {
				is_set = (bit & (1ull << j)) != 0;
				if (is_set)
					reg++;
			}
		}
	}
	unsigned i = reg / ESIZE;
	unsigned j = reg % ESIZE;
	generator->bits[i] |= (1ull << j);
	assert(reg <= generator->max_reg);
	if (reg == generator->max_reg)
		generator->max_reg += 1;
	return reg;
}

static int test_pseudo_reg() 
{
	PseudoGenerator generator = {0};
	/* as we keep allocating top reg is updated */
	for (int i = 0; i < 255; i++) {
		unsigned max_reg = raviX_max_reg(&generator);
		if (i != max_reg)
			assert(max_reg == i);
		unsigned reg = pseudo_gen_alloc(&generator, false);
		if (top_reg(&generator) != reg)
			assert(pseudo_gen_is_top(&generator, reg));
	}

	/* freeing some inbetween reg doesn't affect top reg */
	pseudo_gen_free(&generator, 240);
	assert(raviX_max_reg(&generator) == 255);
	/* next alloc picks free reg */
	unsigned reg = pseudo_gen_alloc(&generator, false);
	assert(reg == 240);

	/* repeat test for free and alloc, no change to top reg */
	for (int i = 0; i < 3; i++) {
		pseudo_gen_free(&generator, 63);
		pseudo_gen_free(&generator, 64);
		assert(raviX_max_reg(&generator) == 255);
		reg = pseudo_gen_alloc(&generator, false);
		assert(reg == 63);
		reg = pseudo_gen_alloc(&generator, false);
		assert(reg == 64);
	}

	/* reset and alloc regs upto 99 */
	generator = (PseudoGenerator){0};
	for (int i = 0; i < 100; i++) {
		unsigned max_reg = raviX_max_reg(&generator);
		if (i != max_reg)
			assert(max_reg == i);
		unsigned reg = pseudo_gen_alloc(&generator, false);
		assert(pseudo_gen_is_top(&generator, reg));
	}

	assert(pseudo_gen_is_top(&generator, 99));
	/* no free reg so top will be bumped */
	reg = pseudo_gen_alloc(&generator, true);
	assert(reg == 100);
	pseudo_gen_free(&generator, reg);
	/* 100 free now */
	reg = pseudo_gen_alloc(&generator, true);
	assert(reg == 100);
	/* none free so next is 101 */
	reg = pseudo_gen_alloc(&generator, true);
	assert(reg == 101);
	fprintf(stderr, "PseudoGenerator OK\n");
	return 0;
}

int main(int argc, const char *argv[])
{	
	int rc = test_stringset();
	rc += test_memalloc();
	rc += test_bitset();
	rc += test_pseudo_reg();
	if (rc == 0)
		printf("Ok\n");
	else
		printf("FAILED\n");
	return rc != 0 ? 1 : 0;
}
