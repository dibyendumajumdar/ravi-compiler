#include "graph.h"

#include <stdlib.h>

struct node;
struct graph {
	unsigned allocated; /* tracks allocated size of nodes */
	struct node **nodes; /* array[allocated] indexed by nodeId_t, note user must check if nodes[i] != NULL */
	struct allocator node_allocator;
	nodeId_t entry, exit; /* entry and exit nodes */
	void *userdata;
};

struct edge_list {
	unsigned count : 16; /* in use */
	unsigned allocated : 16; /* tracks allocated size of nodes */
	struct edge *edges; /* array[allocated] of edges, populated 0..count */
};
struct node {
	nodeId_t index; /* the id of the basic_block */
	uint32_t pre; /* preorder */
	uint32_t rpost; /* reverse postorder? */
	struct edge_list in_edges; /* in_edges come from predecessor nodes */
	struct edge_list out_edges; /* out_edges lead to successor nodes */
};

static void raviX_edge_list_free(struct edge_list *edge_list);
static struct node *raviX_add_node(struct graph *g, nodeId_t index);

struct graph *raviX_init_graph(nodeId_t entry, nodeId_t exit, void *userdata)
{
	struct graph *g = (struct graph *)calloc(1, sizeof(struct graph));
	g->allocated = 0;
	g->nodes = NULL;
	raviX_allocator_init(&g->node_allocator, "node_allocator", sizeof(struct node), sizeof(double),
			     sizeof(struct node) * 32);
	raviX_add_node(g, entry);
	raviX_add_node(g, exit);
	g->entry = entry;
	g->exit = exit;
	g->userdata = userdata;
	return g;
}

static void raviX_destroy_node(struct node *n)
{
	if (n == NULL)
		return;
	raviX_edge_list_free(&n->in_edges);
	raviX_edge_list_free(&n->out_edges);
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
	edge_list->edges = NULL;
}

static void raviX_edge_list_free(struct edge_list *edge_list)
{
	edge_list->count = 0;
	edge_list->allocated = 0;
	free(edge_list->edges);
	edge_list->edges = NULL;
}

static struct edge *raviX_edge_list_find(const struct edge_list *edge_list, nodeId_t index)
{
	for (unsigned i = 0; i < edge_list->count; i++) {
		if (edge_list->edges[i].to_index == index) {
			return &edge_list->edges[i];
		}
	}
	return NULL;
}

static void raviX_edge_list_grow(struct edge_list *edge_list)
{
	unsigned new_size = edge_list->allocated + 8;
	struct edge *edges =
	    raviX_reallocate(edge_list->edges, edge_list->allocated * sizeof(struct edge), new_size * sizeof(struct edge));
	edge_list->allocated = new_size;
	edge_list->edges = edges;
}

/* add an edge to the edge list if not already added */
static void raviX_edge_list_add(struct edge_list *edge_list, nodeId_t index)
{
	if (raviX_edge_list_find(edge_list, index))
		return;
	if (edge_list->count >= edge_list->allocated) {
		raviX_edge_list_grow(edge_list);
	}
	assert(edge_list->count < edge_list->allocated);
	edge_list->edges[edge_list->count].to_index = index;
	assert(edge_list->edges[edge_list->count].edge_type == 0);

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
	assert(n->pre == 0);
	assert(n->rpost == 0);
	raviX_edge_list_init(&n->in_edges);
	raviX_edge_list_init(&n->out_edges);
	/* note that each node must have an index such that n = nodes[index] */
	n->index = index;
	g->nodes[index] = n;
	return n;
}

void raviX_add_edge(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_add_node(g, from);
	struct node *succnode = raviX_add_node(g, to);

	raviX_edge_list_add(&prednode->out_edges, to);
	raviX_edge_list_add(&succnode->in_edges, from);
}

bool raviX_has_edge(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_get_node(g, from);
	if (prednode == NULL)
		return false;
	return raviX_edge_list_find(&prednode->out_edges, to) != NULL;
}

const struct edge *raviX_get_edge_info(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_get_node(g, from);
	if (prednode == NULL)
		return NULL;
	return raviX_edge_list_find(&prednode->out_edges, to);
}

struct edge_list* raviX_node_successors(struct graph* g, nodeId_t n)
{
	struct node* prednode = raviX_get_node(g, n);
	if (prednode == NULL)
		return false;
	return &prednode->out_edges;
}

struct edge_list* raviX_node_predecessors(struct graph* g, nodeId_t n)
{
	struct node* prednode = raviX_get_node(g, n);
	if (prednode == NULL)
		return false;
	return &prednode->in_edges;
}

uint32_t raviX_edge_count(struct edge_list* list)
{
	return list->count;
}

nodeId_t raviX_get_edge(struct edge_list* list, uint32_t i)
{
	if (i < list->count)
		return list->edges[i].to_index;
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

/* says how many nodes are in the graph */
uint32_t raviX_graph_size(struct graph* g) 
{
	uint32_t count = 0;
	for (unsigned i = 0; i < g->allocated; i++) {
		if (g->nodes[i] != NULL) {
			count++;
		}
	}
	return count;
}

struct classifier_state {
	uint32_t preorder;
	uint32_t rpostorder;
};

/*
 * Do a recursive depth first search and mark nodes with pre/reverse post order sequence, as well
 * as classify edges. Algorithm from figure 3.2 in Building and Optimizing Compiler
 */
static void DFS_classify(struct graph* g, struct node *n, struct classifier_state *state)
{
	assert(n);

	n->pre = state->preorder;
	state->preorder++;

	/* For each successor node */
	for (unsigned i = 0; i < n->out_edges.count; i++) {
		struct edge* E = &n->out_edges.edges[i];
		struct node* S = g->nodes[E->to_index];
		if (S->pre == 0) {
			E->edge_type = EDGE_TYPE_TREE;
			DFS_classify(g, S, state);
		}
		else if (S->rpost == 0) {
			E->edge_type = EDGE_TYPE_BACKWARD;
		}
		else if (n->pre < S->pre) {
			E->edge_type = EDGE_TYPE_FORWARD;
		}
		else {
			E->edge_type = EDGE_TYPE_CROSS;
		}
	}

	n->rpost = state->rpostorder;
	state->rpostorder--;
}

/* 
Classify edges in the graph. Implements algorithm described in 
figure 3.2 - Building an Optimizing Compiler. This algorithm is also implemented
in MIR.
*/
void raviX_classify_edges(struct graph* g)
{
	uint32_t N = raviX_graph_size(g);
	if (N == 0)
		return;

	struct classifier_state state = { .preorder = 1, .rpostorder = N };

	/* reset all data we will be computing */
	for (unsigned i = 0; i < g->allocated; i++) {
		if (g->nodes[i] != NULL) {
			g->nodes[i]->pre = 0;
			g->nodes[i]->rpost = 0;
			for (unsigned i = 0; i < g->nodes[i]->out_edges.count; i++) {
				struct edge* E = &g->nodes[i]->out_edges.edges[i];
				E->edge_type = 0;
			}
		}
	}

	DFS_classify(g, g->nodes[g->entry], &state);
}

static void draw_node(void *arg, struct graph *g, uint32_t nodeid)
{
	FILE *fp = (FILE *) arg;
	struct edge_list *edge_list = raviX_node_successors(g, nodeid);
	if (!edge_list)
		return;
	for (unsigned i = 0; i < raviX_edge_count(edge_list); i++) {
		fprintf(fp, "L%d -> L%d\n", nodeid, raviX_get_edge(edge_list, i));
	}
}

void raviX_draw_graph(struct graph *g, FILE *fp)
{
	fprintf(fp, "digraph {\n");
	raviX_for_each_node(g, draw_node, fp);
	fprintf(fp, "}\n");
}

