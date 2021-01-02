#include "graph.h"

#include <assert.h>
#include <stdlib.h>

struct node;
struct graph {
	unsigned allocated;  /* tracks allocated size of nodes */
	struct node **nodes; /* array[allocated] indexed by nodeId_t, note user must check if nodes[i] != NULL */
	struct allocator node_allocator;
	nodeId_t entry, exit; /* entry and exit nodes */
	void *userdata;
};

struct GraphNodeLink {
	nodeId_t node_index;
	unsigned char edge_type;
};
/* A node_list is simply a dynamic array/vector of node ids.
 */
struct node_list {
	unsigned count : 16;	 /* in use */
	unsigned allocated : 16; /* tracks allocated size of links array */
	struct GraphNodeLink *links; /* array[allocated] of links, populated [0..count) */
};

/* A node in the graph. For each node we maintain a list of predecessor nodes and successor nodes.
 */
struct node {
	nodeId_t index;		/* the id of the basic_block */
	uint32_t pre;		/* preorder */
	uint32_t rpost;		/* reverse postorder */
	struct node_list preds; /* predecessor nodes */
	struct node_list succs; /* successor nodes */
};

static struct node *raviX_add_node(struct graph *g, nodeId_t index);

static void node_list_init(struct node_list *node_list)
{
	node_list->count = 0;
	node_list->allocated = 0;
	node_list->links = NULL;
}

static void node_list_destroy(struct node_list *node_list)
{
	node_list->count = 0;
	node_list->allocated = 0;
	free(node_list->links);
	node_list->links = NULL;
}

/* Gets the offset of the node or -1 if not found */
static int64_t node_list_search(const struct node_list *node_list, nodeId_t index)
{
	for (unsigned i = 0; i < node_list->count; i++) {
		if (node_list->links[i].node_index == index) {
			return i;
		}
	}
	return -1;
}

/* Gets the given node or NULL if node does not exist */
static inline struct GraphNodeLink *node_list_get(const struct node_list *node_list, nodeId_t index)
{
	int64_t i = node_list_search(node_list, index);
	if (i < 0)
		return NULL;
	return &node_list->links[i];
}

/* Grows the node list array */
static void node_list_grow(struct node_list *node_list)
{
	unsigned new_size = node_list->allocated + 8u;
	struct GraphNodeLink *edges = raviX_realloc_array(node_list->links, sizeof(struct GraphNodeLink), node_list->allocated,
						   new_size);
	node_list->allocated = new_size;
	node_list->links = edges;
}

/* add an node to the node_list if not already added */
static void node_list_add(struct node_list *node_list, nodeId_t index)
{
	if (node_list_search(node_list, index) != -1)
		return;
	if (node_list->count >= node_list->allocated) {
		node_list_grow(node_list);
	}
	assert(node_list->count < node_list->allocated);
	node_list->links[node_list->count].node_index = index;
	assert(node_list->links[node_list->count].edge_type == 0);

	node_list->count++;
}

/* delete an node from the node_list if it exists */
static void node_list_delete(struct node_list *node_list, nodeId_t index)
{
	int64_t i = node_list_search(node_list, index);
	if (i < 0)
		return;
	node_list->count = (unsigned) raviX_del_array_element(node_list->links, sizeof node_list->links[0], node_list->count, i, 1);
}

uint32_t raviX_node_list_size(struct node_list *list) { return list->count; }

nodeId_t raviX_node_list_at(struct node_list *list, uint32_t i)
{
	if (i < list->count)
		return list->links[i].node_index;
	assert(false);
	return (nodeId_t)-1;
}

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
	node_list_destroy(&n->preds);
	node_list_destroy(&n->succs);
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
	    raviX_realloc_array(g->nodes, sizeof(struct node*), g->allocated, new_size);
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
	node_list_init(&n->preds);
	node_list_init(&n->succs);
	/* note that each node must have an index such that n = nodes[index] */
	n->index = index;
	g->nodes[index] = n;
	return n;
}

void raviX_add_edge(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_add_node(g, from);
	struct node *succnode = raviX_add_node(g, to);

	node_list_add(&prednode->succs, to);
	node_list_add(&succnode->preds, from);
}

void raviX_delete_edge(struct graph *g, nodeId_t a, nodeId_t b)
{
	struct node_list *successors_of_a = raviX_successors(raviX_graph_node(g, a));
	struct node_list *predecessors_of_b = raviX_predecessors(raviX_graph_node(g, b));

	assert(successors_of_a);
	assert(predecessors_of_b);

	if (successors_of_a == NULL || predecessors_of_b == NULL)
		return;

	node_list_delete(successors_of_a, b);
	node_list_delete(predecessors_of_b, a);
}

bool raviX_has_edge(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_get_node(g, from);
	if (prednode == NULL)
		return false;
	return node_list_search(&prednode->succs, to) != -1;
}

enum edge_type raviX_get_edge_type(struct graph *g, nodeId_t from, nodeId_t to)
{
	struct node *prednode = raviX_get_node(g, from);
	if (prednode == NULL)
		return EDGE_TYPE_UNCLASSIFIED;
	struct GraphNodeLink *node_link = node_list_get(&prednode->succs, to);
	if (node_link == NULL)
		return EDGE_TYPE_UNCLASSIFIED;
	return node_link->edge_type;
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
uint32_t raviX_graph_size(struct graph *g)
{
	uint32_t count = 0;
	for (unsigned i = 0; i < g->allocated; i++) {
		if (g->nodes[i] != NULL) {
			count++;
		}
	}
	return count;
}

uint32_t raviX_node_RPO(struct node *n)
{
	assert(n);
	return n->rpost;
}
nodeId_t raviX_node_index(struct node *n)
{
	assert(n);
	return n->index;
}
struct node *raviX_graph_node(struct graph *g, nodeId_t index)
{
	assert(index < g->allocated);
	return g->nodes[index];
}
struct node_list *raviX_predecessors(struct node *n)
{
	assert(n);
	return &n->preds;
}
struct node_list *raviX_successors(struct node *n)
{
	assert(n);
	return &n->succs;
}

struct classifier_state {
	uint32_t preorder;
	uint32_t rpostorder;
};

/*
 * Do a recursive depth first search and mark nodes with pre/reverse post order sequence, as well
 * as classify links. Algorithm from figure 3.2 in Building and Optimizing Compiler
 */
static void DFS_classify(struct graph *g, struct node *n, struct classifier_state *state)
{
	assert(n);

	n->pre = state->preorder;
	state->preorder++;

	/* For each successor node */
	for (unsigned i = 0; i < n->succs.count; i++) {
		struct GraphNodeLink *E = &n->succs.links[i];
		struct node *S = g->nodes[E->node_index];
		if (S->pre == 0) {
			E->edge_type = EDGE_TYPE_TREE;
			DFS_classify(g, S, state);
		} else if (S->rpost == 0) {
			E->edge_type = EDGE_TYPE_BACKWARD;
		} else if (n->pre < S->pre) {
			E->edge_type = EDGE_TYPE_FORWARD;
		} else {
			E->edge_type = EDGE_TYPE_CROSS;
		}
	}

	n->rpost = state->rpostorder;
	state->rpostorder--;
}

/*
Classify links in the graph. Implements algorithm described in
figure 3.2 - Building an Optimizing Compiler. This algorithm is also implemented
in MIR.
*/
void raviX_classify_edges(struct graph *g)
{
	uint32_t N = raviX_graph_size(g);
	if (N == 0)
		return;

	struct classifier_state state = {.preorder = 1, .rpostorder = N};

	/* reset all data we will be computing */
	for (unsigned i = 0; i < g->allocated; i++) {
		if (g->nodes[i] != NULL) {
			g->nodes[i]->pre = 0;
			g->nodes[i]->rpost = 0;
			for (unsigned i = 0; i < g->nodes[i]->succs.count; i++) {
				struct GraphNodeLink *E = &g->nodes[i]->succs.links[i];
				E->edge_type = 0;
			}
		}
	}

	DFS_classify(g, g->nodes[g->entry], &state);
}

static int rpost_cmp(const void *a1, const void *a2)
{
	const struct node *n1 = *((const struct node **)a1);
	const struct node *n2 = *((const struct node **)a2);
	int result = n1->rpost - n2->rpost;
	return result;
}

static int post_cmp(const void *a1, const void *a2) { return -rpost_cmp(a1, a2); }

void raviX_sort_nodes_by_RPO(struct node **nodes, size_t count, bool forward)
{
	qsort(nodes, count, sizeof(struct node *), forward ? post_cmp : rpost_cmp);
}

struct node **raviX_graph_nodes_sorted_by_RPO(struct graph *g, bool forward)
{
	uint32_t N = raviX_graph_size(g);
	struct node **nodes = calloc(N, sizeof(struct node *));
	unsigned j = 0;
	for (unsigned i = 0; i < g->allocated; i++) {
		if (g->nodes[i] == NULL)
			continue;
		nodes[j++] = g->nodes[i];
	}
	assert(j == N);
	raviX_sort_nodes_by_RPO(nodes, N, forward);
	return nodes;
}

static void draw_node(void *arg, struct graph *g, uint32_t nodeid)
{
	FILE *fp = (FILE *)arg;
	struct node_list *successors = raviX_successors(raviX_graph_node(g, nodeid));
	if (!successors)
		return;
	for (unsigned i = 0; i < raviX_node_list_size(successors); i++) {
		fprintf(fp, "L%d -> L%d\n", nodeid, raviX_node_list_at(successors, i));
	}
}

void raviX_draw_graph(struct graph *g, FILE *fp)
{
	fprintf(fp, "digraph {\n");
	raviX_for_each_node(g, draw_node, fp);
	fprintf(fp, "}\n");
}
