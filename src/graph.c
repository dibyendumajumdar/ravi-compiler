#include "graph.h"

#include <stdlib.h>

struct node;
struct graph {
	unsigned allocated; /* tracks allocated size of nodes */
	struct node **nodes; /* indexed by nodeId_t */
	struct allocator node_allocator;
};
struct edge_list {
	unsigned count : 16; /* in use */
	unsigned allocated : 16; /* tracks allocated size of nodes */
	nodeId_t *nodes;
};
struct node {
	nodeId_t index; /* the id of the basic_block */
	struct edge_list pred;
	struct edge_list succ;
};

static void raviX_edge_list_free(struct edge_list *edge_list);

struct graph *raviX_init_graph(void)
{
	struct graph *g = (struct graph *)calloc(1, sizeof(struct graph));
	g->allocated = 0;
	g->nodes = NULL;
	raviX_allocator_init(&g->node_allocator, "node_allocator", sizeof(struct node), sizeof(double),
			     sizeof(struct node) * 32);
	return g;
}

static void raviX_destroy_node(struct node *n)
{
	if (n == NULL)
		return;
	raviX_edge_list_free(&n->pred);
	raviX_edge_list_free(&n->succ);
}

void raviX_destroy_graph(struct graph *g)
{
	for (unsigned i = 0; i < g->allocated; i++) {
		raviX_destroy_node(g->nodes[i]);
	}
	raviX_allocator_destroy(&g->node_allocator);
	free(g->nodes);
	free(g);
}

static void raviX_edge_list_init(struct edge_list *edge_list)
{
	edge_list->count = 0;
	edge_list->allocated = 0;
	edge_list->nodes = NULL;
}

static void raviX_edge_list_free(struct edge_list *edge_list)
{
	edge_list->count = 0;
	edge_list->allocated = 0;
	free(edge_list->nodes);
	edge_list->nodes = NULL;
}

static bool raviX_edge_list_contains(const struct edge_list *edge_list, nodeId_t index)
{
	for (unsigned i = 0; i < edge_list->count; i++) {
		if (edge_list->nodes[i] == index) {
			return true;
		}
	}
	return false;
}

static void raviX_edge_list_grow(struct edge_list *edge_list)
{
	unsigned new_size = edge_list->allocated + 8;
	nodeId_t *nodes =
	    raviX_reallocate(edge_list->nodes, edge_list->allocated * sizeof(nodeId_t), new_size * sizeof(nodeId_t));
	edge_list->allocated = new_size;
	edge_list->nodes = nodes;
}

/* add an edge to the edge list if not already added */
static void raviX_edge_list_add(struct edge_list *edge_list, nodeId_t index)
{
	if (raviX_edge_list_contains(edge_list, index))
		return;
	if (edge_list->count >= edge_list->allocated) {
		raviX_edge_list_grow(edge_list);
	}
	assert(edge_list->count < edge_list->allocated);
	edge_list->nodes[edge_list->count] = index;
	edge_list->count++;
}

static struct node *raviX_get_node(const struct graph *g, nodeId_t index)
{
	if (index < g->allocated && g->nodes[index] != NULL) {
		// already allocated
		return g->nodes[index];
	}
	return NULL;
}

static void raviX_graph_grow(struct graph *g, nodeId_t needed)
{
	unsigned new_size = needed + 8;
	struct node **new_data =
	    raviX_reallocate(g->nodes, g->allocated * sizeof(struct node *), new_size * sizeof(struct node *));
	g->allocated = new_size;
	g->nodes = new_data;
}

static struct node *raviX_add_node(struct graph *g, nodeId_t index)
{
	if (index < g->allocated && g->nodes[index] != NULL) {
		// already allocated
		return g->nodes[index];
	}
	if (index >= g->allocated) {
		raviX_graph_grow(g, index);
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

void raviX_add_edge(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_add_node(g, from);
	struct node *succnode = raviX_add_node(g, to);

	raviX_edge_list_add(&prednode->succ, to);
	raviX_edge_list_add(&succnode->pred, from);
}

bool raviX_has_edge(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_get_node(g, from);
	if (prednode == NULL)
		return false;
	return raviX_edge_list_contains(&prednode->succ, to);
}

struct edge_list* raviX_node_successors(struct graph* g, nodeId_t n)
{
	struct node* prednode = raviX_get_node(g, n);
	if (prednode == NULL)
		return false;
	return &prednode->succ;
}

struct edge_list* raviX_node_predecessors(struct graph* g, nodeId_t n)
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

nodeId_t raviX_get_edge(struct edge_list* list, uint32_t i)
{
	if (i < list->count)
		return list->nodes[i];
	assert(false);
	return (nodeId_t)-1;
}

void raviX_for_each_node(struct graph *g, void (*callback)(void *arg, struct graph *g, nodeId_t nodeid), void *arg)
{
	for (unsigned i = 0; i < g->allocated; i++) {
		if (g->nodes[i] != NULL) {
			callback(arg, g, g->nodes[i]->index);
		}
	}
}
