#ifndef ravicomp_PTRLIST_H
#define ravicomp_PTRLIST_H

/*
* Generic pointer list manipulation code.
*
* (C) Copyright Linus Torvalds 2003-2005
*/
/*
* This version is part of the dmr_c project.
* Copyright (C) 2017 Dibyendu Majumdar
*/

#include <allocate.h>

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
* The ptr list data structure is like a train - with cars linked to each other.
* Just as in a train each car has many seats, so in ptr list each "node" has
* several entries. Unlike a train however, the ptr list is arranged as a ring,
* i.e. the the front and back nodes are linked to each other. Hence there is no
* such thing as a 'head' of the list - i.e. any node can be the head!
*/

#ifndef LIST_NODE_NR
#define LIST_NODE_NR (7)
#endif

#define DECLARE_PTR_LIST(listname, type)                                       \
	struct listname {                                                      \
		int nr_ : 8;                                                   \
		int rm_ : 8;                                                   \
		struct listname *prev_;                                        \
		struct listname *next_;                                        \
		struct allocator *allocator_;                                  \
		type *list_[LIST_NODE_NR];                                     \
	}

/* Each node in the list */
DECLARE_PTR_LIST(ptr_list, void);

struct ptr_list_iter {
	struct ptr_list *__head;
	struct ptr_list *__list;
	int __nr;
};

/* The ptr list */
extern int ptrlist_size(const struct ptr_list *self);
extern void **ptrlist_add(struct ptr_list **self, void *ptr, struct allocator *ptr_list_allocator);
extern void *ptrlist_nth_entry(struct ptr_list *list, unsigned int idx);
extern void *ptrlist_first(struct ptr_list *list);
extern void *ptrlist_last(struct ptr_list *list);
extern int ptrlist_linearize(struct ptr_list *head, void **arr, int max);
extern void ptrlist_split_node(struct ptr_list *head);
extern void ptrlist_pack(struct ptr_list **self);
extern void ptrlist_remove_all(struct ptr_list **self);
extern int ptrlist_remove(struct ptr_list **self, void *entry, int count);
extern int ptrlist_replace(struct ptr_list **self, void *old_ptr, void *new_ptr,
	int count);
extern void *ptrlist_undo_last(struct ptr_list **self);
extern void *ptrlist_delete_last(struct ptr_list **self);
extern void ptrlist_concat(struct ptr_list *a, struct ptr_list **self);
extern void ptrlist_sort(struct ptr_list **self, void *,
			 int (*cmp)(void *, const void *, const void *));

/* iterator functions */
extern struct ptr_list_iter ptrlist_forward_iterator(struct ptr_list *self);
extern struct ptr_list_iter ptrlist_reverse_iterator(struct ptr_list *self);
extern void *ptrlist_iter_next(struct ptr_list_iter *self);
extern void *ptrlist_iter_prev(struct ptr_list_iter *self);
extern void ptrlist_iter_split_current(struct ptr_list_iter *self);
extern void ptrlist_iter_insert(struct ptr_list_iter *self, void *newitem);
extern void ptrlist_iter_remove(struct ptr_list_iter *self);
extern void ptrlist_iter_set(struct ptr_list_iter *self, void *ptr);
extern void ptrlist_iter_mark_deleted(struct ptr_list_iter *self);

static inline void **ptrlist_iter_this_address(struct ptr_list_iter *self) {
	return &self->__list->list_[self->__nr];
}
#define ptr_list_empty(x) ((x) == NULL)
#define PTR_ENTRY_NOTAG(h,i)	((h)->list_[i])
#define PTR_ENTRY(h,i)	(void *)(PTR_ENTRY_NOTAG(h,i))

#define FOR_EACH_PTR(list, var) \
	{ struct ptr_list_iter var##iter__ = ptrlist_forward_iterator((struct ptr_list *)list); \
	for (var = ptrlist_iter_next(&var##iter__); var != NULL; var = ptrlist_iter_next(&var##iter__))
#define END_FOR_EACH_PTR(var) }

#define FOR_EACH_PTR_REVERSE(list, var) \
	{ struct ptr_list_iter var##iter__ = ptrlist_reverse_iterator((struct ptr_list *)list); \
	for (var = ptrlist_iter_prev(&var##iter__); var != NULL; var = ptrlist_iter_prev(&var##iter__))
#define END_FOR_EACH_PTR_REVERSE(var) }

#define RECURSE_PTR_REVERSE(list, var) \
	{ struct ptr_list_iter var##iter__ = list##iter__; \
	for (var = ptrlist_iter_prev(&var##iter__); var != NULL; var = ptrlist_iter_prev(&var##iter__))

#define PREPARE_PTR_LIST(list, var)	\
	struct ptr_list_iter var##iter__ = ptrlist_forward_iterator((struct ptr_list *)list); \
	var = ptrlist_iter_next(&var##iter__)

#define NEXT_PTR_LIST(var) \
	var = ptrlist_iter_next(&var##iter__)
#define FINISH_PTR_LIST(var) 

#define THIS_ADDRESS(type, var) \
	(type *)ptrlist_iter_this_address(&var##iter__)

#define DELETE_CURRENT_PTR(var) \
	ptrlist_iter_remove(&var##iter__)	

#define REPLACE_CURRENT_PTR(type, var, replacement) \
	ptrlist_iter_set(&var##iter__, replacement)

#define INSERT_CURRENT(newval, var) \
	ptrlist_iter_insert(&var##iter__, newval)	

#define MARK_CURRENT_DELETED(PTR_TYPE, var) \
	ptrlist_iter_mark_deleted(&var##iter__)


extern int test_ptrlist();

#ifdef __cplusplus
}
#endif


#endif
