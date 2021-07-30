# Linear IR

The compiler generates an intermediate representation (IR). This document describes the IR.

The code for each function is translated to linear IR. Each function is represented as a `proc`.
Each `proc` has a set of `basic blocks` - there are two distinguished basic blocks, the `entry` block and the
`exit` block.

Each basic block consists of a sequence of `instruction`s and ends with a branching instruction.

## Representation of Instructions

Instructions are uniformly represented as follows. Each instruction has an op code. Each instruction has a list of operands and a list of targets.
Conceptually the targets represent the output of the instruction, whereas operands are the input, although sometimes the interpretation may be different.

Instruction operands and targets are represented via a `pseudo` type. This is a union type that allows various different types of objects to be
uniformly represented in an instruction. Following are the possible types:

<dl>
    <dt>PSEUDO_SYMBOL</dt><dd>An object of type lua_symbol representing local variable or upvalue, always refers to Lua stack relative to 'base'</dd>
	<dt>PSEUDO_TEMP_FLT</dt><dd>A floating point temporary - may also be used for locals that do not escape - refers to C stack</dd>
	<dt>PSEUDO_TEMP_INT</dt><dd>An integer temporary - may also be used for locals that do not escape - references C stack</dd>
	<dt>PSEUDO_TEMP_BOOL</dt><dd>An integer temporary restricted to <code>1</code> and <code>0</code> - references C stack, shares the virtual C stack with <code>PSEUDO_TEMP_INT</code></dd>
	<dt>PSEUDO_TEMP_ANY</dt><dd>A temporray of any type - will always be on Lua stack relative to 'base'</dd>
	<dt>PSEUDO_CONSTANT</dt><dd>A literal constant</dd>
	<dt>PSEUDO_PROC</dt><dd>A Lua function</dd>
	<dt>PSEUDO_NIL</dt><dd><code>nil</code> value</dd>
	<dt>PSEUDO_TRUE</dt><dd><code>true</code> value</dd>
	<dt>PSEUDO_FALSE</dt><dd><code>false</code> value</dd>
	<dt>PSEUDO_BLOCK</dt><dd>A basic block, used for targets of branching instructions</dd>
	<dt>PSEUDO_RANGE</dt><dd>Represents a range of registers from a certain starting register on Lua stack relative to 'base'</dd>
	<dt>PSEUDO_RANGE_SELECT</dt><dd>Picks a certain register from a range, resolves to register on Lua stack, relative to 'base'</dd>
	<dt>PSEUDO_LUASTACK</dt><dd>Refers to Lua stack position, relative to <code>ci->func</code> rather than <code>base</code>, used by backend for copying results to calling function. Will never be emitted in the IR. This special pseudo type is needed because Lua puts variable args between <code>ci->func</code> and <code>base</code>.</dd>
</dl>


## Instructions

The instruction set in the intermediate representation is covered here. As each opcode is implemented end to end, it will be added to the list below.

When printed as text, the instructions are always output in following format.

```
 	OP_Code '{' operands '}' '{' targets '}'
```

Example:

```
	LOADGLOBAL {Upval(_ENV), 'assert' Ks(0)} {T(0)}
	MOV {T(0)} {local(assert, 0)}
```

### Operands and targets

In the textual output, each operand or target refers to a `Psuedo`. The following conventions are used:

* `Upval` - a `PSEUDO_SYMBOL` referencing an up-value
* `local` - a `PSEUDO_SYMBOL` referencing a local variable on Lua stack
* `Tint` - a `PSEUDO_TEMP_INT` referencing a C stack variable
* `Tflt` - a `PSEUDO_TEMP_FLT` referencing a C stack variable
* `Tbool` - a `PSEUDO_TEMP_BOOL` referencing a C stack variable
* `T` - a `PSEUDO_TEMP_ANY` referencing a Lua stack variable relative to 'base`
* `T(n..)` - a `PSEUDO_RANGE` referencing a Lua stack position starting at register `n` from 'base'
* `T(s[n..])` a `PSEUDO_RANGE_SELECT` referencing a particular register `s` from a range starting at register `n` from 'base'
* `Kint` - a `PSEUDO_CONSTANT` of integer type
* `Kflt` - a `PSEUDO_CONSTANT` of floating point type
* `Ks` - a `PSEUDO_CONSTANT` of string type

Example:

```
	LOADGLOBAL {Upval(_ENV), 'io' Ks(5)} {T(5)}
```
Above we see three pseudos, `Upval(_ENV)`, `'io' Ks(5)` and `T(5)`, an upvalue, a string constant and a temporary, respectively.

```
	CALL {T(0), Tint(0), 0E0 Kflt(0)} {T(0..), 1 Kint(0)}
```
In this example, we have 3 operand pseuods, and 2 target pseudos. 

* `T(0)` refers to a temporary at register `0`
* `Tint(0)` refers to an integer temporary at C stack variable `0`
* `0E0 Kflt(0)` refers to a floating point constant
* `T(0..)` refers to a range of registers starting at register `0`.
* `1 Kint(0)` refers to an integer constant


#### `RET` 

Returns values to calling function, and sets `L->ci` to parent.

<dl>
    <dt>operands[]</dt>
    <dd>0 or more pseudos representing values to be returned</dd>
    <dt>target</dt>
    <dd>The block to which we should jump to. Note that all returns jump to the exit block</dd>
</dl> 

The `RET` instruction must perform some housekeeping. 

* Firstly it must invoke `luaF_close()` if the proc has child procs so that up-values are closed and 
any deferred closures executed. This call may be omitted if no variables in the proc including child procs escaped.
* Next it must copy the return values to the stack, results must be placed at `ci->func` and above. The number of results to copy needs to take into account  `ci->nresults` field which says how many values the caller is expecting. If the caller is expecting more values that are available then the extra values should be set to `nil`. If `ci->nresults == -1` caller wants all available values.
* The last operand might be a `PSEUDO_RANGE`, in which case `RET` must inspect `L->top` to determine the number of values to copy. 
* The `L->ci` must be set to the parent of the current function.
* `RET` sets `L-top` to just past the return values on the stack.

### `MOV`

Copies a value from one location to another

<dl>
    <dt>operands<dt>
    <dd>1 source pseudo</dd>
    <dt>targets</dt>
    <dd>1 target pseudo</dd>
</dl>

* The move operation deals with scalar quantities, table/array store/loads are handled by different operators.

### `BR`

Branches unconditionally to the target block

<dl>
    <dt>operands</dt>
    <dd>none</dd>
    <dt>targets</dt>
    <dd>1 target block</dd>
</dl>

### `CBR`

Branches conditionally to one of two blocks.

<dl>
    <dt>operands</dt>
    <dd>The condition pseudo to be tested for truth</dd>
    <dt>targets</dt>
    <dd>Two block pseudos, the first is the target for true condition and second for false condition</dd>
</dl>

### `CALL`

The `CALL` opcode is used to invoke a function.

<dl>
    <dt>operands[]</dt>
    <dd>The first operand is the function to be called.
    This is followed by function arguments. The last argument may be a <code>PSEUDO_RANGE</code>.
    </dd>
    <dt>target[0]</dt>
    <dd>The register where results will be placed. This
    ia also where the function value / arguments will be placed prior to
    invoking the function.</dd>
    <dt>target[1]</dt>
    <dd>The second value is the number of results the caller is expecting.
    If this is <code>-1</code> then caller wants all results.</dd>
</dl>

* If the caller supplied `-1` as number of results then `CALL` needs to determine
the number of values to return by inspecting `L->top` - the number of values to return will
be the difference between `L->top` and register at `target[0]`.
* Before calling a function, `CALL` needs to ensure that `L->top` is set to just past
the last argument as `luaD_precall()` uses this to determine the number of arguments.
* `CALL` copies the function and arguments to the right place and then 
invokes `luaD_precall()` to handle the actual function call.

### `LOADGLOBAL`

The `LOADGLOBAL` opcode is used to retrieve a value from the `_ENV` table. By default Lua
provides an up-value in the main chunk that references the `_ENV` table. But users can define
a local `_ENV` variable that overrides the default.

The `LOADGLOBAL` is akin to loading a value from a table, where the key is always a 
string constant and the table is usually an up-value.

<dl>
    <dt>operand[0]</dt>
    <dd>The symbol representing <code>_ENV</code> - may be an up-value or a local</dd>
    <dt>operand[1]</dt>
    <dd>A string constant representing the name of the global variable</dd>
    <dt>target</dt>
    <dd>Always a register pseudo - may be local or temporary register</dd>
</dl>

### Various `GET` opcodes

The IR has several flavours of the `GET` opcode. The general structure is as follows:

<dl>
    <dt>operand[0]</dt>
    <dd>The symbol representing table, or table like interface, or array</dd>
    <dt>operand[1]</dt>
    <dd>A key</dd>
    <dt>target</dt>
    <dd>The destination pseudo where the value extracted will be loaded</dd>
</dl>

The various flavours of `GET` opcodes are listed below.

OpCode  | Operand[0] = table | Operand[1] = key | Target[0] = destination
--- | --- | --- | ---
`GET` | a table like interface | Any key type | A register pseudo 
`GETsk` | a table like interface | A string key | A register pseudo
`GETik` | a table like interface | An integer key | A register pseudo
`TGET` | Symbol representing table | A key of any type | A register pseudo
`TGETik` | Symbol representing table | A key of integer type | A register pseudo
`TGETsk` | Symbol representing table | A key of string type | A register pseudo
`IAGET` | Symbol representing an `integer[]` | A key of any type | Integer temporary
`IAGETik` | Symbol representing an `integer[]` | A key of integer type | Integer temporary
`FAGET` | Symbol representing an `number[]` | A key of any type | Floating point temporary
`FAGETik` | Symbol representing an `number[]` | A key of integer type | Floating point temporary


### `STOREGLOBAL`

The `STOREGLOBAL` opcode is used to save a value to the `_ENV` table. By default Lua
provides an up-value in the main chunk that references the `_ENV` table. But users can define
a local `_ENV` variable that overrides the default.

The `STOREGLOBAL` is akin to saving a value to a table, where the key is always a 
string constant and the table is usually an up-value.

<dl>
    <dt>operand</dt>
    <dd>Value to store</dd>
    <dt>target[0]</dt>
    <dd>The symbol representing <code>_ENV</code> - may be an up-value or a local</dd>
    <dt>target[1]</dt>
    <dd>A string constant representing the name of the global variable</dd>
</dl>

### Various `PUT` opcodes

The IR has several flavours of the `PUT` opcode. The general structure is as follows:

<dl>
    <dt>operand[0]</dt>
    <dd>The source pseudo</dd>
    <dt>target[0]</dt>
    <dd>Destination table like object</dd>
    <dt>target[1]</dt>
    <dd>The key to sue to index into the table like object</dd>
</dl>

The various flavours of `PUT` opcodes are listed below.

OpCode  | Operand[0] = source pseudo | Target[0] = table like object | Target[1] = key
--- | --- | --- | ---
`PUT` | Register pseudo | a table like interface | Any key type 
`PUTsk` | Register pseudo | a table like interface | A string key
`PUTik` | Register pseudo | a table like interface | An integer key
`TPU` | Register pseudo | Symbol representing table | A key of any type
`TPUTik` | Register pseudo | Symbol representing table | A key of integer type
`TPUTsk` | Register pseudo | Symbol representing table | A key of string type
`IAPUT` | Register pseudo | Symbol representing an `integer[]` | An integer key
`IAPUTiv` | Integer value | Symbol representing an `integer[]` | An integer key
`FAPUT` | Register pseudo | Symbol representing an `number[]` | An integer key
`FAPUTfv` | Floating point value | Symbol representing an `number[]` | An integer key


### Comparison operators

A number of comparison operators are used in the linear IR. 

The general form is as follows:

<dl>
    <dt>operand[0]</dt>
    <dd>First input operand</dd>
    <dt>operand[1]</dt>
    <dd>Second input operand</dd>
    <dt>target[0]</dt>
    <dd>Destination pseudo</dd>
</dl>

The available op codes are listed below:

OPCode | Description | Result 
--- | --- | ---
`EQ` | compares two Lua stack values for `==` | Temp register
`EQii` | compares two C stack integers | Temp boolean
`EQff` | compares two C stack floats | Temp boolean
`LT` | compares two Lua stack values for `<` | Temp register
`LTii` | compares two C stack integers | Temp boolean 
`LTff` | compares two C stack floats | Temp boolean 
`LE` | compares two Lua stack values for `<=` | Temp register
`LEii` | compares two C stack integers | Temp boolean 
`LEff` | compares two C stack floats | Temp boolean 

### Binary operators

A number of binary arithmetic/bitwise operators are used in the linear IR. 

The general form is as follows:

<dl>
    <dt>operand[0]</dt>
    <dd>First input operand</dd>
    <dt>operand[1]</dt>
    <dd>Second input operand</dd>
    <dt>target[0]</dt>
    <dd>Destination pseudo</dd>
</dl>

The available op codes are listed below:

OPCode | Operand 1 | Operand 2 | Result 
--- | --- | --- | ---
`ADD` | Any | Any | Temp register
`ADDii` | Integer | Integer | Temp integer
`ADDff` | Floating point | Floating point | Temp floating point
`ADDfi` | Floating point | Integer | Temp floating point
`SUB` | Any | Any | Temp register 
`SUBii` | Integer | Integer | Temp integer 
`SUBff` | Floating point | Floating point | Temp floating point
`SUBfi` | Floating point | Integer | Temp floating point 
`SUBif` | Integer | Floating point | Temp floating point
`MUL` | Any | Any | Temp register
`MULii` | Integer | Integer | Temp integer
`MULff` | Floating point | Floating point | Temp floating point
`MULfi` | Floating point | Integer | Temp floating point
`DIV` | Any | Any | Temp register 
`DIVii` | Integer | Integer | Temp integer 
`DIVff` | Floating point | Floating point | Temp floating point
`DIVfi` | Floating point | Integer | Temp floating point 
`DIVif` | Integer | Floating point | Temp floating point
`IDIV` | Any | Any | Temp register 
`MOD` | Any | Any | Temp register 
`POW` | Any | Any | Temp register 
`BAND` | Any | Any | Temp register 
`BANDii` | Integer | Integer | Temp integer 
`BOR` | Any | Any | Temp register 
`BORii` | Integer | Integer | Temp integer 
`BXOR` | Any | Any | Temp register 
`BXORii` | Integer | Integer | Temp integer 
`SHR` | Any | Any | Temp register 
`SHRii` | Integer | Integer | Temp integer 
`SHL` | Any | Any | Temp register 
`SHLii` | Integer | Integer | Temp integer 

### Unary operators

A number of unary arithmetic/bitwise operators are used in the linear IR. 

The general form is as follows:

<dl>
    <dt>operand[0]</dt>
    <dd>Input operand</dd>
    <dt>target[0]</dt>
    <dd>Destination pseudo</dd>
</dl>

The available op codes are listed below:

OPCode | Operand | Result 
--- | --- | ---
`UNM` | Any | Temp register
`UNMi` | Integer | Temp integer
`UNMf` | Floating point | Temp floating point
`LEN` | table like object | Temp register
`LENi` | table like object | Temp integer



