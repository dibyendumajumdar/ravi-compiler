#ifndef ravicomp_DATAFLOW_FRAMEWORK_H
#define ravicomp_DATAFLOW_FRAMEWORK_H

#include "graph.h"
#include <stdbool.h>

extern void raviX_solve_dataflow(struct graph *g, bool forward_p,
				 int (*join_function)(void *, nodeId_t, bool init),
				 int (*transfer_function)(void *, nodeId_t), void *userdata);


#endif