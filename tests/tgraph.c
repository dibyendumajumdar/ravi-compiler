#include "graph.h"

static int test1(void)
{
	int errcount = 0;
	struct graph *g = raviX_init_graph(0, 2, NULL);
	raviX_add_edge(g, 0, 1);
	raviX_add_edge(g, 1, 2);
	if (!raviX_has_edge(g, 0, 1))
		errcount += 1;
	if (!raviX_has_edge(g, 1, 2))
		errcount += 1;
	if (raviX_has_edge(g, 0, 2))
		errcount += 1;
	raviX_draw_graph(g, stdout);
	raviX_destroy_graph(g);
	return errcount;
}

static struct graph *make_graph(void)
{
	struct graph *g = raviX_init_graph(0, 5, NULL);
	raviX_add_edge(g, 0, 1);
	raviX_add_edge(g, 1, 2);
	raviX_add_edge(g, 2, 3);
	raviX_add_edge(g, 3, 4);
	raviX_add_edge(g, 4, 5);
	raviX_add_edge(g, 0, 5);
	raviX_add_edge(g, 1, 4);
	raviX_add_edge(g, 3, 2);
	raviX_add_edge(g, 2, 6);
	raviX_add_edge(g, 6, 3);
	raviX_add_edge(g, 4, 1);
	return g;
}

static int test2(void)
{
	int errcount = 0;
	struct graph *g = make_graph();
	raviX_classify_edges(g);
	if (raviX_get_edge(g, 0, 1)->edge_type != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge(g, 1, 2)->edge_type != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge(g, 2, 3)->edge_type != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge(g, 2, 6)->edge_type != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge(g, 3, 4)->edge_type != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge(g, 4, 5)->edge_type != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge(g, 0, 5)->edge_type != EDGE_TYPE_FORWARD)
		errcount++;
	if (raviX_get_edge(g, 1, 4)->edge_type != EDGE_TYPE_FORWARD)
		errcount++;
	if (raviX_get_edge(g, 6, 3)->edge_type != EDGE_TYPE_CROSS)
		errcount++;
	if (raviX_get_edge(g, 3, 2)->edge_type != EDGE_TYPE_BACKWARD)
		errcount++;
	if (raviX_get_edge(g, 4, 1)->edge_type != EDGE_TYPE_BACKWARD)
		errcount++;
	raviX_draw_graph(g, stdout);
	raviX_destroy_graph(g);
	return errcount;
}

static struct graph *make_graph2(void)
{
	struct graph *g = raviX_init_graph(0, 4, NULL);
	raviX_add_edge(g, 0, 1);
	raviX_add_edge(g, 1, 2);
	raviX_add_edge(g, 1, 5);
	raviX_add_edge(g, 2, 3);
	raviX_add_edge(g, 5, 6);
	raviX_add_edge(g, 5, 8);
	raviX_add_edge(g, 6, 7);
	raviX_add_edge(g, 8, 7);
	raviX_add_edge(g, 7, 3);
	raviX_add_edge(g, 3, 1);
	raviX_add_edge(g, 3, 4);
	return g;
}

static int test3(void)
{
	int errcount = 0;
	struct graph *g = make_graph2();
	raviX_classify_edges(g);
	struct dominator_tree *tree = raviX_new_dominator_tree(g);
	raviX_calculate_dominator_tree(tree);
	raviX_dominator_tree_output(tree, stdout);
	raviX_destroy_dominator_tree(tree);
	raviX_destroy_graph(g);
	return errcount;
}

int main(int argc, const char *argv[])
{
	int errcount = test1();
	errcount += test2();
	errcount += test3();
	if (errcount == 0)
		printf("Ok\n");
	else
		printf("Failed\n");
	return errcount == 0 ? 0 : 1;
}