#include "graph.h"
#include "implementation.h"
#include "ravi_compiler.h"

struct dominator_tree {
	struct graph *g;
	struct node **IDoms;
	uint32_t N;
};

struct dominator_tree *raviX_new_dominator_tree(struct graph *g)
{
	struct dominator_tree *state =
	    (struct dominator_tree *)calloc(1, sizeof(struct dominator_tree));
	state->N = raviX_graph_size(g);
	state->IDoms = (struct node **)calloc(state->N, sizeof(struct node *));
	state->g = g;
	return state;
}

void raviX_destroy_dominator_tree(struct dominator_tree *state)
{
	free(state->IDoms);
	free(state);
}

static struct node *intersect(struct dominator_tree *state, struct node *i, struct node *j)
{
	struct graph *g = state->g;
	struct node *finger1 = i;
	struct node *finger2 = j;
	while (finger1 != finger2) {
		while (raviX_node_RPO(finger1) > raviX_node_RPO(finger2)) {
			finger1 = state->IDoms[raviX_node_index(finger1)];
			assert(finger1);
		}
		while (raviX_node_RPO(finger2) > raviX_node_RPO(finger1)) {
			finger2 = state->IDoms[raviX_node_index(finger2)];
			assert(finger2);
		}
	}
	return finger1;
}

static struct node *find_first_processed(struct dominator_tree *state, struct edge_list *predlist)
{
	for (uint32_t i = 0; i < raviX_edge_count(predlist); i++) {
		nodeId_t id = raviX_get_edge(predlist, i);
		if (state->IDoms[id])
			return raviX_graph_node(state->g, id);
	}
	return NULL;
}

void raviX_calculate_dominator_tree(struct dominator_tree *state)
{
	uint32_t N = raviX_graph_size(state->g);
	struct node **sorted_by_rpo = raviX_graph_nodes_sorted_by_RPO(state->g, false);
	for (uint32_t i = 0; i < state->N; i++) {
		state->IDoms[i] = NULL; /* undefined - set to a invalid value */
	}
	// Set IDom entry for root to itself
	state->IDoms[ENTRY_BLOCK] = raviX_graph_node(state->g, ENTRY_BLOCK);
	bool changed = true;
	while (changed) {
		changed = false;
		// for all nodes, b, in reverse postorder (except root)
		for (uint32_t i = 0; i < N; i++) {
			struct node *b = sorted_by_rpo[i];
			if (raviX_node_index(b) == ENTRY_BLOCK) // except root
				continue;
			struct edge_list *predecessors = raviX_predecessors(b); // Predecessors
			// NewIDom = first (processed) predecessor of b, pick one
			struct node *firstpred = find_first_processed(state, predecessors);
			assert (firstpred != NULL);
			struct node *NewIDom = firstpred;
			// for all other predecessors, p, of b
			for (uint32_t k = 0; k < raviX_edge_count(predecessors); k++) {
				nodeId_t pid = raviX_get_edge(predecessors, k);
				struct node *p = raviX_graph_node(state->g, pid);
				if (p == firstpred)
					continue;				 // all other predecessors
				if (state->IDoms[raviX_node_index(p)] != NULL) { // i.e. IDoms[p] calculated
					NewIDom = intersect(state, p, NewIDom);
				}
			}
			if (state->IDoms[raviX_node_index(b)] != NewIDom) {
				fprintf(stdout, "Setting IDom for %d to %d\n", raviX_node_index(b), raviX_node_index(NewIDom));
				state->IDoms[raviX_node_index(b)] = NewIDom;
				changed = true;
			}
		}
	}
	free(sorted_by_rpo);
	for (uint32_t i = 0; i < N; i++) {
		fprintf(stdout, "IDOM[%d] = %d\n", i, raviX_node_index(state->IDoms[i]));
	}
}