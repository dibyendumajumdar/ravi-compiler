#ifndef ravicomp_GRAPH_H
#define ravicomp_GRAPH_H

#include "allocate.h"
#include "common.h"

#include <stdint.h>
#include <stdbool.h>

/*
 * Various graph manipulation routines.
 * The graph is designed to manage nodes that are just integer ids.
 * Node ids range from [0..n) - hence one can simply represent nodes as arrays.
 *
 * The graph structure does not care what the node represents and
 * knows nothing about it. The benefit of this approach is that we can make
 * the graph algorithms reusable. There may be some performance cost as we
 * need to map node ids to nodes.
 *
 * The assumption here is that each node corresponds to a basic block in
 * the program intermediate code. And each basic block is identified by a node
 * id which can be used to construct the control flow graph.
 */

/* nodeId_t is declared elsewhere */
struct graph;
struct node;
struct node_list;
enum edge_type {
	EDGE_TYPE_UNCLASSIFIED = 0,
	EDGE_TYPE_TREE = 1,
	EDGE_TYPE_BACKWARD = 2,
	EDGE_TYPE_FORWARD = 4,
	EDGE_TYPE_CROSS = 8
};


/* Initialize the graph data structure and associate some userdata with it. */
struct graph *raviX_init_graph(nodeId_t entry, nodeId_t exit, void *userdata);
/* Destroy the graph data structure */
void raviX_destroy_graph(struct graph *g);

/* Add an edge from one node a to b. Both nodes a and b will be implicitly added
 * to the graph if they do not already exist.
 */
void raviX_add_edge(struct graph *g, nodeId_t a, nodeId_t b);
/* Check if an edge exists from one node a to b */
bool raviX_has_edge(struct graph *g, nodeId_t a, nodeId_t b);
/* Delete an edge from a to b */
void raviX_delete_edge(struct graph *g, nodeId_t a, nodeId_t b);
/* Get the edge classification for edge from a to b; this is only available if graph has been
 * analyzed for edges. */
enum edge_type raviX_get_edge_type(struct graph *g, nodeId_t a, nodeId_t b);

/* Get node identified by index */
struct node *raviX_graph_node(struct graph *g, nodeId_t index);
/* Get the RPO - reverse post order index of the node */
uint32_t raviX_node_RPO(struct node *n);
/* Get the node's id */
nodeId_t raviX_node_index(struct node *n);
/* Get list of predecessors */
struct node_list *raviX_predecessors(struct node *n);
/* Get list of successors */
struct node_list *raviX_successors(struct node *n);

/* Number of entries in the node_list */
uint32_t raviX_node_list_size(struct node_list *list);
/* Get the nodeId at given node_link position */
nodeId_t raviX_node_list_at(struct node_list *list, uint32_t i);

void raviX_for_each_node(struct graph *g, void (*callback)(void *arg, struct graph *g, nodeId_t nodeid), void *arg);

/*
 * Classifies links in the graph and also computes the
 * reverse post order value.
 */
void raviX_classify_edges(struct graph *g);
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

void raviX_sort_nodes_by_RPO(struct node **nodes, size_t count, bool forward);

/* says how many nodes are in the graph */
uint32_t raviX_graph_size(struct graph *g);
/* Generates GraphViz (dot) output */
void raviX_draw_graph(struct graph *g, FILE *fp);


#endif