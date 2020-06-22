#ifndef ravicomp_BITSET_H
#define ravicomp_BITSET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t bitmap_el_t;

struct bitset_t {
	size_t els_num;
	size_t size;
	bitmap_el_t *varr;
};

typedef struct bitset_t * bitmap_t;
typedef const struct bitset_t * const_bitmap_t;

extern bitmap_t raviX_bitmap_create2(size_t init_bits_num);
static inline bitmap_t raviX_bitmap_create(void)
{
	return raviX_bitmap_create2(0);
}
extern void raviX_bitmap_destroy(bitmap_t bm);
static inline void raviX_bitmap_clear(bitmap_t bm)
{
	bm->els_num = 0;
}
extern int raviX_bitmap_bit_p(const_bitmap_t bm, size_t nb);
extern int raviX_bitmap_set_bit_p(bitmap_t bm, size_t nb);
extern int raviX_bitmap_clear_bit_p(bitmap_t bm, size_t nb);
extern int raviX_bitmap_set_or_clear_bit_range_p(bitmap_t bm, size_t nb, size_t len, int set_p);
static inline int raviX_bitmap_set_bit_range_p(bitmap_t bm, size_t nb, size_t len) {
	return raviX_bitmap_set_or_clear_bit_range_p(bm, nb, len, true);
}
static inline int raviX_bitmap_clear_bit_range_p(bitmap_t bm, size_t nb, size_t len) {
	return raviX_bitmap_set_or_clear_bit_range_p(bm, nb, len, false);
}
extern void raviX_bitmap_copy(bitmap_t dst, const_bitmap_t src);
extern int raviX_bitmap_equal_p(const_bitmap_t bm1, const_bitmap_t bm2);
extern int raviX_bitmap_intersect_p(const_bitmap_t bm1, const_bitmap_t bm2);
extern int raviX_bitmap_empty_p(const_bitmap_t bm);
/* Return the number of bits set in BM.  */
extern size_t raviX_bitmap_bit_count(const_bitmap_t bm);
extern int raviX_bitmap_op2(bitmap_t dst, const_bitmap_t src1, const_bitmap_t src2,
		       bitmap_el_t (*op) (bitmap_el_t, bitmap_el_t));
static inline bitmap_el_t raviX_bitmap_el_and(bitmap_el_t el1, bitmap_el_t el2) { return el1 & el2; }
static inline int raviX_bitmap_and(bitmap_t dst, bitmap_t src1, bitmap_t src2) {
	return raviX_bitmap_op2(dst, src1, src2, raviX_bitmap_el_and);
}
static inline bitmap_el_t raviX_bitmap_el_and_compl(bitmap_el_t el1, bitmap_el_t el2) {
	return el1 & ~el2;
}
static inline int raviX_bitmap_and_compl(bitmap_t dst, bitmap_t src1, bitmap_t src2) {
	return raviX_bitmap_op2(dst, src1, src2, raviX_bitmap_el_and_compl);
}
static inline bitmap_el_t raviX_bitmap_el_ior(bitmap_el_t el1, bitmap_el_t el2) { return el1 | el2; }
static inline int raviX_bitmap_ior(bitmap_t dst, bitmap_t src1, bitmap_t src2) {
	return raviX_bitmap_op2(dst, src1, src2, raviX_bitmap_el_ior);
}
int raviX_bitmap_op3(bitmap_t dst, const_bitmap_t src1, const_bitmap_t src2,
		const_bitmap_t src3,
		bitmap_el_t (*op) (bitmap_el_t, bitmap_el_t, bitmap_el_t));
static inline bitmap_el_t raviX_bitmap_el_ior_and(bitmap_el_t el1, bitmap_el_t el2, bitmap_el_t el3) {
	return el1 | (el2 & el3);
}
/* DST = SRC1 | (SRC2 & SRC3).  Return true if DST changed.  */
static inline int raviX_bitmap_ior_and(bitmap_t dst, bitmap_t src1, bitmap_t src2, bitmap_t src3) {
	return raviX_bitmap_op3(dst, src1, src2, src3, raviX_bitmap_el_ior_and);
}
static inline bitmap_el_t raviX_bitmap_el_ior_and_compl(bitmap_el_t el1, bitmap_el_t el2,
						   bitmap_el_t el3) {
	return el1 | (el2 & ~el3);
}
/* DST = SRC1 | (SRC2 & ~SRC3).  Return true if DST changed.  */
static inline int raviX_bitmap_ior_and_compl(bitmap_t dst, bitmap_t src1, bitmap_t src2, bitmap_t src3) {
	return raviX_bitmap_op3(dst, src1, src2, src3, raviX_bitmap_el_ior_and_compl);
}

typedef struct {
	bitmap_t bitmap;
	size_t nbit;
} bitmap_iterator_t;
static inline void raviX_bitmap_iterator_init(bitmap_iterator_t *iter, bitmap_t bitmap) {
	iter->bitmap = bitmap;
	iter->nbit = 0;
}
extern int raviX_bitmap_iterator_next(bitmap_iterator_t *iter, size_t *nbit);
#define FOREACH_BITMAP_BIT(iter, bitmap, nbit) \
  for (raviX_bitmap_iterator_init (&iter, bitmap); raviX_bitmap_iterator_next (&iter, &nbit);)




#ifdef __cplusplus
} /* extern C */
#endif

#endif

