/**
 * A framework for performing data flow analysis.
 * The framework is based upon similar framework in MIR project (https://github.com/vnmakarov/mir)
 */

#include "dataflow_framework.h"
#include "allocate.h"
#include "graph.h"
#include "bitset.h"

#include <string.h>

DECLARE_ARRAY(node_array, struct node *);

/**
 * The data flow framework is based on the implementation in MIR project.
 * https://github.com/vnmakarov/mir
 */
struct dataflow_context {
	struct graph *g;
	struct node_array worklist;
	struct node_array pending;
	struct bitset_t bb_to_consider;
	void *userdata;
};

static void raviX_init_data_flow(struct dataflow_context *dataflow_context, struct graph *g)
{
	memset(dataflow_context, 0, sizeof *dataflow_context);
	raviX_bitset_create2(&dataflow_context->bb_to_consider, 512);
	dataflow_context->g = g;
}

static void raviX_finish_data_flow(struct dataflow_context *dataflow_context)
{
	array_clearmem(&dataflow_context->worklist);
	array_clearmem(&dataflow_context->pending);
	raviX_bitset_destroy(&dataflow_context->bb_to_consider);
}

void raviX_solve_dataflow(struct graph *g, bool forward_p,
			  int (*join_function)(void *, nodeId_t, bool),
			  int (*transfer_function)(void *, nodeId_t), void *userdata)
{
	unsigned iter;
	struct dataflow_context ctx, *dataflow_context;
	struct node_array *worklist;
	struct node_array *pending;

	raviX_init_data_flow(&ctx, g);
	dataflow_context = &ctx;
	worklist = &dataflow_context->worklist;
	pending = &dataflow_context->pending;

	/* ensure that the graph has RPO calculated */
	raviX_classify_edges(dataflow_context->g);

	worklist->count = 0;
	/* Initially the basic blocks are added to the worklist */
	for (uint32_t i = 0; i < raviX_graph_size(dataflow_context->g); i++) {
		array_push(worklist, raviX_graph_node(dataflow_context->g, i));
	}
	iter = 0;
	while (worklist->count != 0) {
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
					// TODO - raviX_bitset_set_bit_p(0 return previous value of the bit,
					// as well as setting it ?
					if (raviX_bitset_set_bit_p(&dataflow_context->bb_to_consider, index)) {
						array_push(pending, raviX_graph_node(dataflow_context->g, index));
					}
				}
			}
		}
		iter++;
		{
			/* Swap worklist and pending */
			struct node_array *t = NULL;
			t = worklist;
			worklist = pending;
			pending = t;
		}
	}

	raviX_finish_data_flow(&ctx);
}
