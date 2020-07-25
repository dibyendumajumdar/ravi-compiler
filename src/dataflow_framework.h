#ifndef ravicomp_DATAFLOW_FRAMEWORK_H
#define ravicomp_DATAFLOW_FRAMEWORK_H

#include "graph.h"
#include <stdbool.h>

/*
 * Data Flow Analysis framework.
 * The Join/Transfer functions should return 1 if they made any changes else 0.
 */
extern void raviX_solve_dataflow(
    struct graph *g,
    bool forward_p, /* Set to true for forward data flow */
    int (*join_function)(void *userdata, nodeId_t, bool init), /* Join/Meet operator - init will be true when no successors/predecessors */
    int (*transfer_function)(void *userdata, nodeId_t), /* transfer function */
    void *userdata);  /* pointer to user data, will be passed to join/transfer functions */


#endif