# Linear IR

The compiler generates an intermediate representation. Thid document describes the
IR.

The code for each function is translated to linear IR. Each function is represented as a `proc`.
Each `proc` has a set of `basic blocks` - there are two distinguished basic blocks, the `entry` block and the
`exit` block.

Each basic block consists of a sequence of `instruction`s and ends with a branching instruction.

## Representation of Instructions

Instructions are uniformly represented as follows. Each instruction has an op code. Each instruction has a list of operands and a list of targets.
Conceptually the targets represent the output of the instruction, wheras operands are the input, although sometimes the interpretation may be different.

## Branching Instructions

#### op_ret 

Returns values to calling function

<dl>
    <dt>operands</dt>
    <dd>0 or more pseudos representing values to be returned</dd>
    <dt>target</dt>
    <dd>The block to which we should jump to</dd>
</dl> 

The `op_ret` instruction must perform some housekeeping. 

* Firstly it must invoke `luaF_close()` if the proc has child procs so that up-values are closed and 
any deferred closures executed. 
* Next it must copy the return values to the stack position starting from `ci->func`, but respecting the `ci->nresults` fields which says
how many values the caller is expecting. If the caller is expecting more values that we have then the extra values should be
set to `nil`. 
* The `L->ci` must be set to the parent of the current function.


