# Sources

* `lexer.c` - derived from Lua 5.3 lexer but modified to work as a standalone lexer
* `parser.c` - responsible for generating abstract syntax tree (AST) - consumes lexer output.
* `print.c` - responsible for printing out the AST
* `ast_walker.c` - will provide support for walking the AST
* `typechecker.c` - responsible for performing typechecking and assigning types to various things. Runs on the AST.
* `linearizer.c` - responsible for generating linear intermediate code (IR) - builds basic blocks and also responsible for contructing control flow graph (CFG).

## Utilities

* `allocate.c` - memory allocator
* `fnv_hash.c` - string hashing function
* `hash_table.c` - hash table
* `set.c` - set data structure
* `ptrlist.c` - a hybrid array/linked list data structure
* `membuf.c` - dynamic memory buffer hat supports formatted input - used to build strings incrementally
