/******************************************************************************
 * Copyright (C) 2020-2021 Dibyendu Majumdar
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include "graph.h"
#include "dominator.h"

static int test1(void)
{
	int errcount = 0;
	Graph *g = raviX_init_graph(0, 2, NULL);
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

static Graph *make_graph(void)
{
	Graph *g = raviX_init_graph(0, 5, NULL);
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
	Graph *g = make_graph();
	raviX_classify_edges(g);
	if (raviX_get_edge_type(g, 0, 1) != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge_type(g, 1, 2) != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge_type(g, 2, 3) != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge_type(g, 2, 6) != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge_type(g, 3, 4) != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge_type(g, 4, 5) != EDGE_TYPE_TREE)
		errcount++;
	if (raviX_get_edge_type(g, 0, 5) != EDGE_TYPE_FORWARD)
		errcount++;
	if (raviX_get_edge_type(g, 1, 4) != EDGE_TYPE_FORWARD)
		errcount++;
	if (raviX_get_edge_type(g, 6, 3) != EDGE_TYPE_CROSS)
		errcount++;
	if (raviX_get_edge_type(g, 3, 2) != EDGE_TYPE_BACKWARD)
		errcount++;
	if (raviX_get_edge_type(g, 4, 1) != EDGE_TYPE_BACKWARD)
		errcount++;
	raviX_draw_graph(g, stdout);
	raviX_destroy_graph(g);
	return errcount;
}

static Graph *make_graph2(void)
{
	Graph *g = raviX_init_graph(0, 4, NULL);
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
	Graph *g = make_graph2();
	raviX_classify_edges(g);
	DominatorTree *tree = raviX_new_dominator_tree(g);
	raviX_calculate_dominator_tree(tree);
	raviX_dominator_tree_output(tree, stdout);
	raviX_destroy_dominator_tree(tree);
	raviX_destroy_graph(g);
	return errcount;
}

static int test4(void)
{
	int errcount = 0;
	Graph *g = make_graph2();
	if (raviX_node_list_size(raviX_successors(raviX_graph_node(g, 1))) != 2)
		errcount++;
	if (raviX_node_list_size(raviX_successors(raviX_graph_node(g, 2))) != 1)
		errcount++;
	raviX_delete_edge(g, 1, 2);
	GraphNodeList *succ = raviX_successors(raviX_graph_node(g, 1));
	if (raviX_node_list_size(succ) != 1)
		errcount++;
	if (raviX_node_list_at(succ, 0) != 5)
		errcount++;
	GraphNodeList *preds = raviX_predecessors(raviX_graph_node(g, 2));
	if (raviX_node_list_size(preds) != 0)
		errcount++;
	raviX_destroy_graph(g);
	return errcount;
}

int main(int argc, const char *argv[])
{
	int errcount = test1();
	errcount += test2();
	errcount += test3();
	errcount += test4();
	if (errcount == 0)
		printf("Ok\n");
	else
		printf("Failed\n");
	return errcount == 0 ? 0 : 1;
}