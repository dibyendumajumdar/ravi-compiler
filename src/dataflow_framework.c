/**
 * A framework fpor performing data flow analysis.
 * The framework is based upon similar framework in MIR project (https://github.com/vnmakarov/mir)
 */

#include "dataflow_framework.h"
#include "graph.h"

#include <string.h>

void raviX_init_data_flow(struct dataflow_context *dataflow_context, struct graph *g)
{
	memset(dataflow_context, 0, sizeof *dataflow_context);
	raviX_bitset_create2(&dataflow_context->bb_to_consider, 512);
	dataflow_context->g = g;
}

void raviX_finish_data_flow(struct dataflow_context *dataflow_context)
{
	array_clearmem(&dataflow_context->worklist);
	array_clearmem(&dataflow_context->pending);
	raviX_bitset_destroy(&dataflow_context->bb_to_consider);
}

void raviX_solve_dataflow(struct dataflow_context *dataflow_context, bool forward_p,
			  int (*join_function)(void *, nodeId_t, bool),
			  int (*transfer_function)(void *, nodeId_t))
{
	unsigned iter;
	struct node_array *worklist = &dataflow_context->worklist;
	struct node_array *pending = &dataflow_context->pending;

	worklist->count = 0;
	for (uint32_t i = 0; i < raviX_graph_size(dataflow_context->g); i++) {
		array_push(worklist, raviX_graph_node(dataflow_context->g, i));
	}
	iter = 0;
	while (worklist->count != 0) {
		struct node_array *t = NULL;
		struct node **addr = worklist->data;
		raviX_sort_nodes_by_RPO(addr, worklist->count, forward_p);
		raviX_bitset_clear(&dataflow_context->bb_to_consider);
		pending->count = 0;
		for (unsigned i = 0; i < worklist->count; i++) {
			int changed_p = iter == 0;
			struct node *bb = addr[i];
			struct node_list *nodes = forward_p ? raviX_predecessors(bb) : raviX_successors(bb);
			if (raviX_node_list_size(nodes) == 0)
				join_function(dataflow_context->userdata, raviX_node_index(bb), true);
			else
				changed_p |= join_function(dataflow_context->userdata, raviX_node_index(bb), false);
			if (changed_p && transfer_function(dataflow_context->userdata, raviX_node_index(bb))) {
				struct node_list *list = forward_p ? raviX_successors(bb) : raviX_predecessors(bb);
				for (unsigned i = 0; i < raviX_node_list_size(list); i++) {
					nodeId_t index = raviX_node_list_at(list, i);
					if (raviX_bitset_set_bit_p(&dataflow_context->bb_to_consider, index)) {
						array_push(pending, raviX_graph_node(dataflow_context->g, index));
					}
				}
			}
		}
		iter++;
		t = worklist;
		worklist = pending;
		pending = t;
	}
}
