# Sources

* `lexer.c` - derived from Lua 5.3 lexer but modified to work as a standalone lexer
* `parser.c` - responsible for generating abstract syntax tree (AST) - consumes lexer output.
* `ast_printer.c` - responsible for printing out the AST
* `ast_walker.c` (WIP) - will provide support for walking the AST
* `typechecker.c` - responsible for performing typechecking and assigning types to various things. Runs on the AST.
* `linearizer.c` (WIP) - responsible for generating linear intermediate code (linear IR). 
* `cfg.c` - responsible for constructing a control flow graph from the output of the linearizer.
* `dominator.c` - implementation of dominator tree calculation 

## Utilities

* `allocate.c` - memory allocator
* `fnv_hash.c` - string hashing function
* `hash_table.c` - hash table
* `set.c` - set data structure
* `ptrlist.c` - a hybrid array/linked list data structure
* `membuf.c` - dynamic memory buffer that supports formatted input - used to build strings incrementally
* `graph.c` - simple graph data structure used to generate control flow graph.
* `bitset.c` - Bitset data structure
