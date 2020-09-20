![build](https://github.com/dibyendumajumdar/ravi-compiler/workflows/build/badge.svg)

# ravi-compiler
A compiler for Ravi and Lua.

## Goals

* Create a re-usable Lua/Ravi lexer/parser
* Define conventional linear intermediate representation (IR)
* Generate C code from Lua/Ravi source
* Support Ahead of time (AOT) compilation

## Modules

The compiler library will consist of distinct modules:

* lexer (alpha) - responsible for tokenizing an input buffer
* parser (alpha) - responsible for generating abstract syntax tree (AST).
* typechecker (alpha) - responsible for assigning types to variables when possible.
* AST simplifier (alpha) - responsible for performing some initial simplifications such as constant folding.
* linearizer (alpha) - responsible for constructing a linear IR representation of the AST.
* optimizer (Work in progress) - responsible for improving the code
* codegenerator (Work in progress) - responsible for generating C code

## Status

* Work in progress - currently woking on C code generation from the IR

## Documentation

Documentation is coming soon.

For now you can look at following:
* [WIP public api](https://github.com/dibyendumajumdar/ravi-compiler/blob/master/include/ravi_compiler.h) - only the lexer and parser API are public for now
* [Test inputs and outputs](https://github.com/dibyendumajumdar/ravi-compiler/blob/master/tests)
* [CFG Examples](https://github.com/dibyendumajumdar/ravi-compiler/tree/master/docs/cfg)
* See the [Wiki](https://github.com/dibyendumajumdar/ravi-compiler/wiki) for various notes

## Why

Lua's inbuilt parser and code generator is a work of art, very compact and low overhead but extremely fast. It uses minimal memory and produces bytecodes as it parses the source code (single pass compiler). This is great for Lua and Ravi given the use cases of these languages, but makes the parser and code generator hard to understand, play with, or reuse in tools such as IDEs. It also makes it harder to perform any advanced type checking or performance optimizations. 

This project will create a new parser and code generator that is not a replacement for the default one in Lua/Ravi but can be used for more specialised code generation, as well as as a means of understanding how the parser and code generator works.

## Technology

This project is written in C for maximum portability like Lua. I considered using Rust or D and may eventually port it to one of these languages but for now it is good old C.

## Building 

You will need CMake 3.12 or greater. The build steps are fairly simple on Linux:

```
mkdir build
cd build
cmake ..
make 
```

## Testing

At the moment we have a couple of simple test driver programs: `tparse` and `trun`. These drivers take a string or file input which must be a valid Lua/Ravi chunk of code, and output the AST, the result of type checking, linear IR output if supported, and the CFG as a `dot` file. Example of the output can be found in the `tests/expected` folder.

Suppose `tparse` was built in `build` folder then you can run the tests as follows:

```
cd tests && sh runtests.sh ../build/tparse
```

The test script compares the output to the expected output. Any difference will cause the test script to fail.

The `trun` utility will eventually replace `tparse` as it can handle multiple chunks of code in a single input file. 
