## Sources

* `trun.c` - a new driver for executing tests - handles multiple cases in a single input file. Runs lexer, parser, typechecker, linearizer, CFG generation, etc. and then dumps it all out on `stdout`.
* `tparse.c` - a simple driver that takes some source text as input and runs lexer, parser, typechecker, and linearizer on the input.
* `tstrset.c` - basic smoke test for strings in sets
* `tgraph.c` - basic smoke test for graph data structure.
* `tastwalk.c` - demonstrates how to write AST walking; it does not do anything but just walks the AST silently.

## Running tests

At the moment we run `tparse` or `trun` on inputs and compare the output as saved in `expected` folder. This also acts as a regression test because if anything changes then the test fails.
