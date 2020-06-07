#ifndef ravicomp_GRAPH_H
#define ravicomp_GRAPH_H

#include "implementation.h"
#include "allocate.h"

#include <stdint.h>

/*
 * Current plan is to implement a graph structure that is separate from
 * the basic blocks.
 */

struct graph;
struct edge_list;

struct graph *raviX_init_graph(void);
void raviX_destroy_graph(struct graph *g);

void raviX_add_edge(struct graph *g, uint32_t from, uint32_t to);
bool raviX_has_edge(struct graph *g, uint32_t from, uint32_t to);

struct edge_list* raviX_node_successors(struct graph* g, uint32_t n);
struct edge_list* raviX_node_predecessors(struct graph* g, uint32_t n);

uint32_t raviX_edge_count(struct edge_list* list);
uint32_t raviX_get_edge(struct edge_list* list, uint32_t i);
void raviX_for_each_node(struct graph *g, void (*callback)(void *arg, struct graph *g, uint32_t nodeid), void *arg);

#endif