# ravi-compiler
Experimental compiler for Ravi and Lua 5.3

The goal of this project is to create a standalone parser and compiler for Lua / Ravi 5.3. The output of the compiler will be bytecodes.
The compiler library will consist of distinct modules:

* lexer - responsible for tokenizing an input buffer
* parser - responsible for generating abstract syntax tree (AST).
* typechecker - responsible for assigning types to variables when possible.
* linearizer - responsible for constructing a linear IR representation of the AST.
* optimizer - responsible for improving the code
* codegenerator - responsible for generate bytecodes

## Why

Lua's inbuilt parser and code generator is a work of art, very compact and low overhead but extremely fast. It uses minimal memory and produces bytecodes as it parses the source code (single pass compiler). This is great for Lua and Ravi given the use cases of these languages, but makes the parser and code generator hard to understand, play with, or reuse in tools such as IDEs. It also makes it harder to perform any advanced type checking or performance optimizations. 

This project will create a new parser and code generator that is not a replacement for the default one in Lua/Ravi but can be used for more specialised code generation, as well as as a means of understanding how the parser and code generator works.

## Technology

This project is written in C for maximum portability like Lua. I considered using Rust or D and may eventually port it to one of these languages but for now it is good old C.

## Building 

You will need CMake installation. The build steps are fairly simple on Linux:

```
mkdir build
cd build
cmake ..
make 
```

## Testing

At the moment we have a simple test driver program named `tparse`. It takes a string input which must be a valid Lua/Ravi chunk of code, and outputs the AST, the result of type checking and also any linear IR output if supported. Example of the output can be fund in the `tests` folder.
