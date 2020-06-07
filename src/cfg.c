/* Build CFG */

#include "graph.h"
#include "implementation.h"
#include "ravi_compiler.h"

#include <assert.h>

/* Recursively create control flow graph for each proc
 * Return 0 on success
 */
int raviX_construct_cfg(struct proc *proc)
{
	struct graph *g = raviX_init_graph();
	for (unsigned i = 0; i < proc->node_count; i++) {
		struct basic_block *block = proc->nodes[i];
		struct instruction *insn = raviX_last_instruction(block);
		if (insn == NULL)
			continue;
		if (insn->opcode == op_br || insn->opcode == op_cbr || insn->opcode == op_ret) {
			struct pseudo *pseudo;
			FOR_EACH_PTR(insn->targets, pseudo)
			{
				assert(pseudo->type == PSEUDO_BLOCK);
				raviX_add_edge(g, block->index, pseudo->block->index);
			}
			END_FOR_EACH_PTR(pseudo)
		} else {
			return 1;
		}
	}
	proc->cfg = g;
	struct proc *childproc;
	FOR_EACH_PTR(proc->procs, childproc)
	{
		if (raviX_construct_cfg(childproc) != 0)
			return 1;
	}
	END_FOR_EACH_PTR(childproc)
	return 0;
}

struct CfgArg {
	FILE *fp;
	struct proc *proc;
};
void raviX_output_cfg(struct proc *proc, FILE *fp);

static void output_node(void *arg, struct graph *g, uint32_t nodeid)
{
	struct CfgArg *myargs = (struct CfgArg *)arg;
	FILE *fp = myargs->fp;
	struct proc *proc = myargs->proc;
	struct edge_list *edge_list = raviX_node_successors(g, nodeid);
	if (!edge_list)
		return;
	membuff_t buf;
	raviX_buffer_init(&buf, 1024);
	raviX_output_basic_block_as_table(proc, proc->nodes[nodeid], &buf);
	fprintf(fp, "L%d [shape=none, margin=0, label=<%s>];\n", nodeid, raviX_buffer_data(&buf));
	raviX_buffer_reset(&buf);
	for (unsigned i = 0; i < raviX_edge_count(edge_list); i++) {
		fprintf(fp, "L%d -> L%d\n", nodeid, raviX_get_edge(edge_list, i));
	}
	struct proc *childproc;
	FOR_EACH_PTR(proc->procs, childproc) { raviX_output_cfg(childproc, fp); }
	END_FOR_EACH_PTR(childproc)
}

void raviX_output_cfg(struct proc *proc, FILE *fp)
{
	struct graph *g = proc->cfg;
	if (!g)
		return;
	fprintf(fp, "digraph Proc%d {\n", proc->id);
	struct CfgArg args = {.proc = proc, .fp = fp};
	raviX_for_each_node(g, output_node, &args);
	fprintf(fp, "}\n");
}