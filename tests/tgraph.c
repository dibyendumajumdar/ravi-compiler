#include "graph.h"

int main(int argc, const char *argv[])
{
	int errcount = 0;
	struct graph *g = raviX_init_graph();
	raviX_add_edge(g, 0, 1);
	raviX_add_edge(g, 1, 2);
	if (!raviX_has_edge(g, 0, 1))
		errcount += 1;
	if (!raviX_has_edge(g, 1, 2))
		errcount += 1;
	if (raviX_has_edge(g, 0, 2))
		errcount += 1;
	raviX_destroy_graph(g);
	if (errcount == 0)
		printf("Ok\n");
	else
		printf("Failed\n");
	return errcount == 0 ? 0 : 1;
}