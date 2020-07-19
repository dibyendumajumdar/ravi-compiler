## Sources

* `trun.c` - a new driver for executing tests - handles multiple cases in a single input file. Runs lexer, parser, typechecker, linearizer, CFG generation, etc. and then dumps it all out on `stdout`.
* `tparse.c` - a simple driver that takes some source text as input and runs lexer, parser, typechecker, and linearizer on the input. This is an older version that is now superceded by `trun`.
* `tgraph.c` - basic smoke test for graph data structure.
* `tastwalk.c` - demonstrates how to write AST walking; it does not do anything but just walks the AST silently.
* `tmisc.c` - miscellaneous internal tests.

## Running tests

At the moment we run `tparse` or `trun` on inputs and compare the output as saved in `expected` folder. This also acts as a regression test because if anything changes then the test fails.

## `trun`

The `trun` utility has the following interface.

```
trun [string | -f filename] [--notypecheck] [--nolinearize] [--noastdump] [--noirdump] [--nocodump] [--nocfgdump] [--simplify-ast]
```

The options have following meanings:

* `-f filename` - input file. The input should consist of chunks of code separated by a line containing just `#`. See `t00_exprs.in` in the input folder.
* `--notypecheck` - omits the type checking step
* `--nolinearize` - omits creating the linear IR
* `--noastdump` - stops output of AST
* `--noirdump` - stops output of the linear IR
* `--nocodump` - stops output of the input code chunk
* `--nocfgdump` - stops output of the CFG
* `--simplify-ast` - performs simplifications on the AST such as constant folding

The CFG output is generated in the format supported by the `dot` command in `graphviz`. 

Currently all output will be produced to `stdout`.

Example. 

```
trun "print 'hello world'"
```

Output generated:

```
print 'hello world'
function()
  --[expression statement start]
   --[expression list start]
     --[suffixed expr start] any
      --[primary start] any
        print --global symbol any
      --[primary end]
      --[suffix list start]
        --[function call start] any
         (
           'hello world'
         )
        --[function call end]
      --[suffix list end]
     --[suffixed expr end]
   --[expression list end]
  --[expression statement end]
end
function()
  --[expression statement start]
   --[expression list start]
     --[suffixed expr start] any
      --[primary start] any
        print --global symbol any
      --[primary end]
      --[suffix list start]
        --[function call start] any
         (
           'hello world'
         )
        --[function call end]
      --[suffix list end]
     --[suffixed expr end]
   --[expression list end]
  --[expression statement end]
end
define Proc%1
L0 (entry)
        LOADGLOBAL {print} {T(0)}
        CALL {T(0), 'hello world' Ks(0)} {T(0..)}
        BR {L1}
L1 (exit)
digraph Proc1 {
L0 [shape=none, margin=0, label=<<TABLE BORDER="1" CELLBORDER="0">
<TR><TD><B>L0</B></TD></TR>
<TR><TD>LOADGLOBAL {print} {T(0)}</TD></TR>
<TR><TD>CALL {T(0), 'hello world' Ks(0)} {T(0..)}</TD></TR>
<TR><TD>BR {L1}</TD></TR>
</TABLE>>];
L0 -> L1
}
```

