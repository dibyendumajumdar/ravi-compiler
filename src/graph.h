#ifndef ravicomp_GRAPH_H
#define ravicomp_GRAPH_H

#include "implementation.h"
#include "allocate.h"

#include <stdint.h>

/*
 * Current plan is to implement a graph structure that is separate from
 * the basic blocks. Each basic_block is identified by a unique id; and we assume
 * given an id we can easily locate the basic_block. Thus the graph itself 
 * only concerns itself with the ids.
 */

/* nodeId_t is declared elsewhere */
struct graph;
struct node;
struct edge_list;
enum {
	EDGE_TYPE_UNCLASSIFIED = 0,
	EDGE_TYPE_TREE = 1,
	EDGE_TYPE_BACKWARD = 2,
	EDGE_TYPE_FORWARD = 4,
	EDGE_TYPE_CROSS = 8
};
struct edge {
	nodeId_t to_index; /* destination edge */
	unsigned char edge_type;
};

struct graph *raviX_init_graph(nodeId_t entry, nodeId_t exit, void *userdata);
void raviX_destroy_graph(struct graph *g);

void raviX_add_edge(struct graph *g, nodeId_t from, nodeId_t to);
bool raviX_has_edge(struct graph *g, nodeId_t from, nodeId_t to);

struct edge_list* raviX_node_successors(struct graph* g, nodeId_t n);
struct edge_list* raviX_node_predecessors(struct graph* g, nodeId_t n);

uint32_t raviX_edge_count(struct edge_list* list);
/* Get the nodeId at given edge position */
nodeId_t raviX_get_nodeid_at_edge(struct edge_list* list, uint32_t i);
void raviX_for_each_node(struct graph *g, void (*callback)(void *arg, struct graph *g, nodeId_t nodeid), void *arg);

const struct edge *raviX_get_edge(struct graph *g, nodeId_t from, nodeId_t to);
uint32_t raviX_node_RPO(struct node *n);
nodeId_t raviX_node_index(struct node *n);
struct node *raviX_graph_node(struct graph *g, nodeId_t index);
struct edge_list* raviX_predecessors(struct node *n);
/*
 * Classifies edges in the graph and also computes the
 * reverse post order value.
 */
void raviX_classify_edges(struct graph* g);
/*
 * Returns a sorted array (allocated).
 * Sorted by reverse postorder value.
 * If forward=true then
 * it will be the opposite direction, so to get reverse postorder,
 * set forward=false.
 * You must deallocate the array when done.
 * The array size will be equal to raviX_graph_size(g).
 * Before attempting to sort, you must have called
 * raviX_classify_edges(g).
 */
struct node **raviX_graph_nodes_sorted_by_RPO(struct graph *g, bool forward);

/* says how many nodes are in the graph */
uint32_t raviX_graph_size(struct graph* g);
/* Generates GraphViz (dot) output */
void raviX_draw_graph(struct graph *g, FILE *fp);

struct dominator_tree;

struct dominator_tree *raviX_new_dominator_tree(struct graph *g);
void raviX_calculate_dominator_tree(struct dominator_tree *state);
void raviX_destroy_dominator_tree(struct dominator_tree *state);
void raviX_dominator_tree_output(struct dominator_tree *tree, FILE *fp);

#endif