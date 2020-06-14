#include "graph.h"
#include "implementation.h"
#include "ravi_compiler.h"

/* The dominator tree construction algorithm is based on figure 9.24,
 * chapter 9, p 532, of Engineering a Compiler
 */

struct dominator_tree {
	struct graph *g;
	struct node **immediate_dominators; /* array of immediate dominators, one per node in the graph */
	uint32_t N;			    /* sizeof immediate_dominators */
};

struct dominator_tree *raviX_new_dominator_tree(struct graph *g)
{
	struct dominator_tree *state = (struct dominator_tree *)calloc(1, sizeof(struct dominator_tree));
	state->N = raviX_graph_size(g);
	state->immediate_dominators = (struct node **)calloc(state->N, sizeof(struct node *));
	state->g = g;
	return state;
}

void raviX_destroy_dominator_tree(struct dominator_tree *state)
{
	free(state->immediate_dominators);
	free(state);
}

/* Finds nearest common ancestor? */
/* The algorithm starts at the two nodes whose sets are being intersected, and walks
 * upward from each toward the root. By comparing the nodes with their RPO numbers
 * the algorithm finds the common ancestor - the immediate dominator of i and i.
 */
static struct node *intersect(struct dominator_tree *state, struct node *i, struct node *j)
{
	struct graph *g = state->g;
	struct node *finger1 = i;
	struct node *finger2 = j;
	while (finger1 != finger2) {
		while (raviX_node_RPO(finger1) > raviX_node_RPO(finger2)) {
			finger1 = state->immediate_dominators[raviX_node_index(finger1)];
			assert(finger1);
		}
		while (raviX_node_RPO(finger2) > raviX_node_RPO(finger1)) {
			finger2 = state->immediate_dominators[raviX_node_index(finger2)];
			assert(finger2);
		}
	}
	return finger1;
}

/* Look for the first predecessor whose immediate dominator has been calculated.
 * Because of the order in which this search occurs, we will always find at least 1
 * predecessor whose immediate dominator has been calculated.
 */
static struct node *find_first_predecessor_with_idom(struct dominator_tree *state, struct edge_list *predlist)
{
	for (uint32_t i = 0; i < raviX_edge_count(predlist); i++) {
		nodeId_t id = raviX_get_nodeid_at_edge(predlist, i);
		if (state->immediate_dominators[id])
			return raviX_graph_node(state->g, id);
	}
	return NULL;
}

void raviX_calculate_dominator_tree(struct dominator_tree *state)
{
	uint32_t N = raviX_graph_size(state->g);
	struct node **nodes_in_reverse_postorder = raviX_graph_nodes_sorted_by_RPO(state->g, false);
	for (uint32_t i = 0; i < state->N; i++) {
		state->immediate_dominators[i] = NULL; /* undefined - set to a invalid value */
	}
	// Set IDom entry for root to itself
	state->immediate_dominators[ENTRY_BLOCK] = raviX_graph_node(state->g, ENTRY_BLOCK);
	bool changed = true;
	while (changed) {
		changed = false;
		// for all nodes, b, in reverse postorder (except root)
		for (uint32_t i = 0; i < N; i++) {
			struct node *b = nodes_in_reverse_postorder[i];
			nodeId_t bid = raviX_node_index(b);
			if (bid == ENTRY_BLOCK) // skip root
				continue;
			struct edge_list *predecessors = raviX_predecessors(b); // Predecessors
			// NewIDom = first (processed) predecessor of b, pick one
			struct node *firstpred = find_first_predecessor_with_idom(state, predecessors);
			assert(firstpred != NULL);
			struct node *NewIDom = firstpred;
			// for all other predecessors, p, of b
			for (uint32_t k = 0; k < raviX_edge_count(predecessors); k++) {
				nodeId_t pid = raviX_get_nodeid_at_edge(predecessors, k);
				struct node *p = raviX_graph_node(state->g, pid);
				if (p == firstpred)
					continue; // all other predecessors
				if (state->immediate_dominators[raviX_node_index(p)] != NULL) {
					// i.e. IDoms[p] calculated
					NewIDom = intersect(state, p, NewIDom);
				}
			}
			if (state->immediate_dominators[bid] != NewIDom) {
				state->immediate_dominators[bid] = NewIDom;
				changed = true;
			}
		}
	}
	free(nodes_in_reverse_postorder);
}

void raviX_dominator_tree_output(struct dominator_tree *tree, FILE *fp)
{
	for (uint32_t i = 0; i < tree->N; i++) {
		fprintf(stdout, "IDOM[%d] = %d\n", i, raviX_node_index(tree->immediate_dominators[i]));
	}
}