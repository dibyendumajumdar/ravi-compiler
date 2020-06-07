#include "graph.h"

#include <stdlib.h>

struct node;
struct graph {
	unsigned allocated;
	struct node **nodes;
	struct allocator node_allocator;
};
struct edge_list {
	unsigned count : 16;
	unsigned allocated : 16;
	uint32_t *nodes;
};
struct node {
	uint32_t index;
	struct edge_list pred;
	struct edge_list succ;
};

void raviX_edge_list_free(struct edge_list *edge_list);

struct graph *raviX_init_graph(void)
{
	struct graph *g = (struct graph *)calloc(1, sizeof(struct graph));
	g->allocated = 0;
	g->nodes = NULL;
	raviX_allocator_init(&g->node_allocator, "node_allocator", sizeof(struct node), sizeof(double),
			     sizeof(struct node) * 32);
	return g;
}

void raviX_destroy_node(struct node *n)
{
	raviX_edge_list_free(&n->pred);
	raviX_edge_list_free(&n->succ);
}

void raviX_destroy_graph(struct graph *g)
{
	for (unsigned i = 0; i < g->allocated; i++) {
		if (g->nodes[i])
			raviX_destroy_node(&g->nodes[i]);
	}
	raviX_allocator_destroy(&g->node_allocator);
	free(g->nodes);
	free(g);
}

void raviX_edge_list_init(struct edge_list *edge_list)
{
	edge_list->count = 0;
	edge_list->allocated = 0;
	edge_list->nodes = NULL;
}

void raviX_edge_list_free(struct edge_list *edge_list)
{
	edge_list->count = 0;
	edge_list->allocated = 0;
	free(edge_list->nodes);
	edge_list->nodes = NULL;
}

bool raviX_edge_list_contains(const struct edge_list *edge_list, uint32_t index)
{
	for (unsigned i = 0; i < edge_list->count; i++) {
		if (edge_list->nodes[i] == index) {
			return true;
		}
	}
	return false;
}

/* add an edge to the edge list if not already added */
void raviX_edge_list_add(struct edge_list *edge_list, uint32_t index)
{
	if (raviX_edge_list_contains(edge_list, index))
		return;
	if (edge_list->count >= edge_list->allocated) {
		unsigned new_size = edge_list->allocated + 1;
		uint32_t *nodes = realloc(edge_list->nodes, new_size * sizeof(uint32_t));
		assert(nodes != NULL);
		edge_list->allocated = new_size;
		edge_list->nodes = nodes;
	}
	edge_list->nodes[edge_list->count] = index;
	edge_list->count++;
}

struct node *raviX_get_node(const struct graph *g, uint32_t index)
{
	if (index < g->allocated && g->nodes[index] != NULL) {
		// already allocated
		return g->nodes[index];
	}
	return NULL;
}

struct node *raviX_add_node(struct graph *g, uint32_t index)
{
	if (index < g->allocated && g->nodes[index] != NULL) {
		// already allocated
		return g->nodes[index];
	}
	if (index >= g->allocated) {
		unsigned new_size = index + 1;
		struct node **new_data = realloc(g->nodes, new_size * sizeof(struct node *));
		assert(new_data != NULL);
		for (unsigned i = g->allocated; i < new_size; i++)
			new_data[i] = NULL;
		g->allocated = new_size;
		g->nodes = new_data;
	}
	assert(index < g->allocated);
	struct node *n = raviX_allocator_allocate(&g->node_allocator, 0);
	raviX_edge_list_init(&n->pred);
	raviX_edge_list_init(&n->succ);
	/* note that each node must have an index such that n = nodes[index] */
	n->index = index;
	g->nodes[index] = n;
	return n;
}

void raviX_add_edge(struct graph *g, uint32_t from, uint32_t to)
{
	struct node *prednode = raviX_add_node(g, from);
	struct node *succnode = raviX_add_node(g, to);

	raviX_edge_list_add(&prednode->succ, to);
	raviX_edge_list_add(&succnode->pred, from);
}

bool raviX_has_edge(struct graph *g, uint32_t from, uint32_t to)
{
	struct node *prednode = raviX_get_node(g, from);
	if (prednode == NULL)
		return false;
	return raviX_edge_list_contains(&prednode->succ, to);
}

struct edge_list* raviX_node_successors(struct graph* g, uint32_t n)
{
	struct node* prednode = raviX_get_node(g, n);
	if (prednode == NULL)
		return false;
	return &prednode->succ;
}

struct edge_list* raviX_node_predecessors(struct graph* g, uint32_t n)
{
	struct node* prednode = raviX_get_node(g, n);
	if (prednode == NULL)
		return false;
	return &prednode->pred;
}

uint32_t raviX_edge_count(struct edge_list* list)
{
	return list->count;
}