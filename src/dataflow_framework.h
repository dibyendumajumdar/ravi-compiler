#ifndef ravicomp_DATAFLOW_FRAMEWORK_H
#define ravicomp_DATAFLOW_FRAMEWORK_H

#include "allocate.h"
#include "graph.h"
#include "bitset.h"

#include <stdbool.h>

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

extern void raviX_init_data_flow(struct dataflow_context *dataflow_context, struct graph *g);
extern void raviX_finish_data_flow(struct dataflow_context *dataflow_context);
extern void raviX_solve_dataflow(struct dataflow_context *dataflow_context, bool forward_p,
				 int (*join_function)(void *, nodeId_t, bool init),
				 int (*transfer_function)(void *, nodeId_t));


#endif