# Sources

* `lexer.c` - derived from Lua 5.3 lexer but modified to work as a standalone lexer
* `parser.c` - responsible for generating abstract syntax tree (AST) - consumes lexer output.
* `typechecker.c` - responsible for performing typechecking and assigning types to various things. Runs on the AST.
* `linearizer.c` - responsible for generating linear intermediate code (IR) - builds basic blocks and also responsible for contructing control flow graph (CFG).
