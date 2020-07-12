#ifndef ravicomp_CFG_H
#define ravicomp_CFG_H

#include "linearizer.h"

#include <stdio.h>

int raviX_construct_cfg(struct proc *proc);
void raviX_output_cfg(struct proc *proc, FILE *fp);

#endif
