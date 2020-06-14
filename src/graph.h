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
nodeId_t raviX_get_edge(struct edge_list* list, uint32_t i);
void raviX_for_each_node(struct graph *g, void (*callback)(void *arg, struct graph *g, nodeId_t nodeid), void *arg);

const struct edge *raviX_get_edge_info(struct graph *g, nodeId_t from, nodeId_t to);
void raviX_classify_edges(struct graph* g);

/* says how many nodes are in the graph */
uint32_t raviX_graph_size(struct graph* g);
/* Generates GraphViz (dot) output */
void raviX_draw_graph(struct graph *g, FILE *fp);

#endif