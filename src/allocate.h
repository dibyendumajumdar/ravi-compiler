#ifndef ravicomp_ALLOCATOR_H
#define ravicomp_ALLOCATOR_H

/*
* allocate.c - simple space-efficient blob allocator.
*
* Copyright (C) 2003 Transmeta Corp.
*               2003-2004 Linus Torvalds
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
* Simple allocator for data that doesn't get partially free'd.
* The tokenizer and parser allocate a _lot_ of small data structures
* (often just two-three bytes for things like small integers),
* and since they all depend on each other you can't free them
* individually _anyway_. So do something that is very space-
* efficient: allocate larger "blobs", and give out individual
* small bits and pieces of it with no maintenance overhead.
*/
/*
* Portions Copyright (C) 2017-2020 Dibyendu Majumdar
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


struct allocation_blob {
	struct allocation_blob *next;
	size_t left, offset;
	unsigned char data[];
};

/*
* Our "blob" allocator works on chunks that are multiples
* of this size (the underlying allocator may be a mmap that
* cannot handle smaller chunks, for example, so trying to
* allocate blobs that aren't aligned is not going to work).
*/
#define CHUNK 32768

struct allocator {
	const char *name_;
	struct allocation_blob *blobs_;
	size_t size_;
	unsigned int alignment_;
	unsigned int chunking_;
	void *freelist_;
	size_t allocations, total_bytes, useful_bytes;
};

extern void raviX_allocator_init(struct allocator *A, const char *name, size_t size,
			   unsigned int alignment, unsigned int chunking);

extern void *raviX_allocator_allocate(struct allocator *A, size_t extra);

extern void raviX_allocator_free(struct allocator *A, void *entry);

extern void raviX_allocator_show_allocations(struct allocator *A);

extern void raviX_allocator_drop_all_allocations(struct allocator *A);

extern void raviX_allocator_destroy(struct allocator *A);

extern void raviX_allocator_transfer(struct allocator *A,
			       struct allocator *transfer_to);

/* 
Reallocate array from old_n to new_n. If new_n is 0 then array memory is freed.
If new_n is greater than old_n then old data is copied across and the 
additional allocated space is zeroed out so caller can rely on the extra space being
initialized to zeros.
*/
extern void* raviX_realloc_array(void* oldp, size_t element_size, size_t old_n, size_t new_n);
/*
Delete n elements starting at i from array a of size array_size, where sizeof(each element) is element_size.
The freed up space will be zero initialized. Returns the new array_size.
*/
extern size_t raviX_del_array_element(void* p, size_t element_size, size_t array_size, size_t i, size_t n);

#ifdef __cplusplus
}
#endif


#endif