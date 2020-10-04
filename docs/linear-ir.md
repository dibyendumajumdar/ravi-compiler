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

Intruction operands and targets are represented via a `pseudo` type. This is a union type that allows various different types of objects to be
uniformly represented in an instruction. Following are the possible types:

<dl>
    <dt>PSEUDO_SYMBOL</dt><dd>a symbol which can be a variable or up-value</dd>
	<dt>PSEUDO_TEMP_FLT</dt><dd>a temporary of floating type</dd>
	<dt>PSEUDO_TEMP_INT</dt><dd>a temporary of integer type</dd>
	<dt>PSEUDO_TEMP_ANY</dt><dd>a temporary of any type, must be on Lua stack</dd>
	<dt>PSEUDO_CONSTANT</dt><dd>a literal constant</dd>
	<dt>PSEUDO_PROC</dt><dd>A Lua function</dd>
	<dt>PSEUDO_NIL</dt><dd>nil value</dd>
	<dt>PSEUDO_TRUE</dt><dd>true value</dd>
	<dt>PSEUDO_FALSE</dt><dd>false value</dd>
	<dt>PSEUDO_BLOCK</dt><dd>a basic block, used for targets of branching instructions</dd>
	<dt>PSEUDO_RANGE</dt><dd>a range of registers with a starting register, unbounded</dd>
	<dt>PSEUDO_RANGE_SELECT</dt><dd>specific register from a range</dd>
	<dt>PSEUDO_LUASTACK</dt><dd>Refers to Lua stack position, relative to ci->func, used by backend for copying results to calling function. Will never be emitted in the IR.</dd>
</dl>


## Instructions

The instruction set in the intermediate representation is covered here. As each opcode is implemented end to end, it will be added to the list below.

#### `op_ret` 

Returns values to calling function, and sets `L->ci` to parent.

<dl>
    <dt>operands</dt>
    <dd>0 or more pseudos representing values to be returned</dd>
    <dt>target</dt>
    <dd>The block to which we should jump to. Note that all returns jump to the exit block</dd>
</dl> 

The `op_ret` instruction must perform some housekeeping. 

* Firstly it must invoke `luaF_close()` if the proc has child procs so that up-values are closed and 
any deferred closures executed. This call may be omitted if no variables in the proc including child procs escaped.
* Next it must copy the return values to the stack, results must be placed at `ci->func` and above. The number of results to copy needs to take into account  `ci->nresults` field which says how many values the caller is expecting. If the caller is expecting more values that are available then the extra values should be set to `nil`. If `ci->nresults == -1` caller wants all available values.
* The `L->ci` must be set to the parent of the current function.
* TBC For compatibility with Lua/ravi, if the number of expected results was `-1` then we should set `L->top` to just past the last result copied, else restore the `L-top` to the previous callers `ci->top`.

### `op_mov`

Copies a value from one location to another

<dl>
    <dt>operands<dt>
    <dd>1 source pseudo</dd>
    <dt>targets</dt>
    <dd>1 target pseudo</dd>
</dl>

* The move operation deals with scalar quantities, table/array store/loads are handled by different operators.

### `op_br`

Branches unconditionally to the target block

<dl>
    <dt>operands</dt>
    <dd>none</dd>
    <dt>targets</dt>
    <dd>1 target block</dd>
</dl>

### `op_cbr`

Branches conditionally to one of two blocks.

<dl>
    <dt>operands</dt>
    <dd>The condition pseudo to be tested for truth</dd>
    <dt>targets</dt>
    <dd>Two block pseudos, the first is the target for true condition and second for false condition</dd>
</dl>

### `op_loadglobal`

The `op_loadglobal` opcode is used to retrieve a value from the `_ENV` table. By default Lua
provides an up-value in the main chunk that references the `_ENV` table. But users can defined
a local variable that overrides the default.

The `op_loadglobal` is akin to loading a value from a table, where the key is always a 
string constant and the table is usually an up-value.

<dl>
    <dt>operand</dt>
    <dd>The symbol representing `_ENV` - may be an up-value or a local</dd>
    <dt>operand</dt>
    <dd>A string constant representing the name of the global variable</dd>
    <dt>target</dt>
    <dd>Always a register pseudo - may be local or temporary register</dd>
</dl>

### `op_storeglobal`

The `op_storeglobal` opcode is used to save a value to the `_ENV` table. By default Lua
provides an up-value in the main chunk that references the `_ENV` table. But users can defined
a local variable that overrides the default.

The `op_storeglobal` is akin to saving a value to a table, where the key is always a 
string constant and the table is usually an up-value.

<dl>
    <dt>operand</dt>
    <dd>Value to store</dd>
    <dt>target</dt>
    <dd>The symbol representing `_ENV` - may be an up-value or a local</dd>
    <dt>target</dt>
    <dd>A string constant representing the name of the global variable</dd>
</dl>
