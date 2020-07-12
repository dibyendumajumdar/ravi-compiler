/* This file is a part of MIR project.
   Copyright (C) 2018-2020 Vladimir Makarov <vmakarov.gcc@gmail.com>.
*/
/*
 * Adapted for Ravi Compiler project
 */

#ifndef ravicomp_BITSET_H
#define ravicomp_BITSET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t bitset_el_t;

struct bitset_t {
	size_t els_num;
	size_t size;
	bitset_el_t *varr;
};

extern void raviX_bitset_create2(struct bitset_t *, size_t init_bits_num);
static inline void raviX_bitset_create(struct bitset_t *bm)
{
	raviX_bitset_create2(bm, 0);
}
extern void raviX_bitset_destroy(struct bitset_t * bm);
static inline void raviX_bitset_clear(struct bitset_t * bm)
{
	bm->els_num = 0;
}
extern int raviX_bitset_bit_p(const struct bitset_t * bm, size_t nb);
/* Sets a bit ON and returns previous value */
extern int raviX_bitset_set_bit_p(struct bitset_t * bm, size_t nb);
extern int raviX_bitset_clear_bit_p(struct bitset_t * bm, size_t nb);
extern int raviX_bitset_set_or_clear_bit_range_p(struct bitset_t * bm, size_t nb, size_t len, int set_p);
static inline int raviX_bitset_set_bit_range_p(struct bitset_t * bm, size_t nb, size_t len) {
	return raviX_bitset_set_or_clear_bit_range_p(bm, nb, len, true);
}
static inline int raviX_bitset_clear_bit_range_p(struct bitset_t * bm, size_t nb, size_t len) {
	return raviX_bitset_set_or_clear_bit_range_p(bm, nb, len, false);
}
extern void raviX_bitset_copy(struct bitset_t * dst, const struct bitset_t * src);
extern int raviX_bitset_equal_p(const struct bitset_t * bm1, const struct bitset_t * bm2);
extern int raviX_bitset_intersect_p(const struct bitset_t * bm1, const struct bitset_t * bm2);
extern int raviX_bitset_empty_p(const struct bitset_t * bm);
/* Return the number of bits set in BM.  */
extern size_t raviX_bitset_bit_count(const struct bitset_t * bm);
extern int raviX_bitset_op2(struct bitset_t * dst, const struct bitset_t * src1, const struct bitset_t * src2,
			    bitset_el_t (*op) (bitset_el_t, bitset_el_t));
static inline bitset_el_t raviX_bitset_el_and(bitset_el_t el1, bitset_el_t el2) { return el1 & el2; }
static inline int raviX_bitset_and(struct bitset_t * dst, struct bitset_t * src1, struct bitset_t * src2) {
	return raviX_bitset_op2(dst, src1, src2, raviX_bitset_el_and);
}
static inline bitset_el_t raviX_bitset_el_and_compl(bitset_el_t el1, bitset_el_t el2) {
	return el1 & ~el2;
}
static inline int raviX_bitset_and_compl(struct bitset_t * dst, struct bitset_t * src1, struct bitset_t * src2) {
	return raviX_bitset_op2(dst, src1, src2, raviX_bitset_el_and_compl);
}
static inline bitset_el_t raviX_bitset_el_ior(bitset_el_t el1, bitset_el_t el2) { return el1 | el2; }
static inline int raviX_bitset_ior(struct bitset_t * dst, struct bitset_t * src1, struct bitset_t * src2) {
	return raviX_bitset_op2(dst, src1, src2, raviX_bitset_el_ior);
}
int raviX_bitset_op3(struct bitset_t * dst, const struct bitset_t * src1, const struct bitset_t * src2,
		const struct bitset_t * src3, bitset_el_t (*op) (bitset_el_t, bitset_el_t, bitset_el_t));
static inline bitset_el_t raviX_bitset_el_ior_and(bitset_el_t el1, bitset_el_t el2, bitset_el_t el3) {
	return el1 | (el2 & el3);
}
/* DST = SRC1 | (SRC2 & SRC3).  Return true if DST changed.  */
static inline int raviX_bitset_ior_and(struct bitset_t * dst, struct bitset_t * src1, struct bitset_t * src2, struct bitset_t * src3) {
	return raviX_bitset_op3(dst, src1, src2, src3, raviX_bitset_el_ior_and);
}
static inline bitset_el_t raviX_bitset_el_ior_and_compl(bitset_el_t el1, bitset_el_t el2, bitset_el_t el3) {
	return el1 | (el2 & ~el3);
}
/* DST = SRC1 | (SRC2 & ~SRC3).  Return true if DST changed.  */
static inline int raviX_bitset_ior_and_compl(struct bitset_t * dst, struct bitset_t * src1, struct bitset_t * src2, struct bitset_t * src3) {
	return raviX_bitset_op3(dst, src1, src2, src3, raviX_bitset_el_ior_and_compl);
}

typedef struct {
	struct bitset_t * bitset;
	size_t nbit;
} bitset_iterator_t;
static inline void raviX_bitset_iterator_init(bitset_iterator_t *iter, struct bitset_t * bitset) {
	iter->bitset = bitset;
	iter->nbit = 0;
}
extern int raviX_bitset_iterator_next(bitset_iterator_t *iter, size_t *nbit);
#define FOREACH_BITSET_BIT(iter, bitset, nbit) \
  for (raviX_bitset_iterator_init (&iter, bitset); raviX_bitset_iterator_next (&iter, &nbit);)




#ifdef __cplusplus
} /* extern C */
#endif

#endif

