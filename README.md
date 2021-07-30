![build](https://github.com/dibyendumajumdar/ravi-compiler/workflows/build/badge.svg)

# ravi-compiler
A compiler for Ravi and Lua that processes Lua/Ravi source code and generates C code.

## Goals

* Create a re-usable Lua/Ravi lexer/parser.
* Define conventional linear intermediate representation (IR).
* Generate C code from Lua/Ravi source.
* Support Ahead of time (AOT) compilation.
* The generated code can be executed by Ravi. Since the generated code depends on various VM structures it is not binary compatible with Lua, but in theory one can modify the code relatively easily to work with Lua 5.3. Lua 5.4 has modifications to the call stack used by Lua which makes it harder to port to.

## Modules

The compiler library consists of distinct modules:

* lexer (alpha) - responsible for tokenizing an input buffer.
* parser (alpha) - responsible for generating abstract syntax tree (AST).
* AST lowerer (alpha) - currently transforms generic for loops to while loops.
* typechecker (alpha) - responsible for assigning types to variables when possible.
* AST simplifier (alpha) - responsible for performing some initial simplifications such as constant folding, and string concatenation re-writing.
* linearizer (alpha) - responsible for constructing a linear IR representation of the AST.
* optimizer (WIP) - responsible for improving the code, this doesn't do much right now.
* codegenerator (alpha) - responsible for generating C code. Each input is translated to a standalone C file that can be compiled using any C compiler. Ravi can compile this at runtime using MIR C JIT compiler. For AOT compilation, dynamic library needs to be created and a special loader needs to be used that treats the shared library as a compiled version of Lua chunk. The generated C code doesn't use the Lua C call api, as it is designed to look like Lua code.

## Status

* 22-Jun-2021 Increased coverage of Lua syntax to cover string concatenations and generic for loops.
* 28-Nov-2020 We can generate code for a large subset of Ravi language and run the compiled code from Ravi.
* 01-Dec-2020 The generated code is now also suitable for AOT compilation but requires special loading facility in Ravi.
* 17-Jan-2021 The code is now C++ compliant so we can compile everything in C++ or C.

## LICENSE

The project is available under MIT license.

## Documentation

Documentation is coming soon.

For now you can look at following:
* [Linear IR](https://github.com/dibyendumajumdar/ravi-compiler/blob/master/docs/linear-ir.md)
* [WIP public api](https://github.com/dibyendumajumdar/ravi-compiler/blob/master/include/ravi_compiler.h) - only the lexer and parser API are public for now
* [Test inputs and outputs](https://github.com/dibyendumajumdar/ravi-compiler/blob/master/tests)
* [Example Ravi Tests](https://github.com/dibyendumajumdar/ravi/tree/master/tests/comptests)
* [AOT Examples](https://github.com/dibyendumajumdar/ravi-compiler/tree/master/examples)
* [CFG Examples](https://github.com/dibyendumajumdar/ravi-compiler/tree/master/docs/cfg)
* See the [Wiki](https://github.com/dibyendumajumdar/ravi-compiler/wiki) for various notes

## Why

Lua's inbuilt parser and code generator is a work of art, very compact and low overhead but extremely fast. It uses minimal memory and produces bytecodes as it parses the source code (single pass compiler). This is great for Lua and Ravi given the use cases of these languages, but makes the parser and code generator hard to understand, play with, or reuse in tools such as IDEs. It also makes it harder to perform any advanced type checking or performance optimizations. 

This project will create a new parser and code generator that is not a replacement for the default one in Lua/Ravi but can be used for more specialised code generation, as well as as a means of understanding how the parser and code generator works.

## Technology

This project is written in C for maximum portability like Lua. 

## Building 

You will need CMake 3.12 or greater. The build steps are fairly simple on Linux:

```
mkdir build
cd build
cmake ..
make 
```

## Try it out!

The compiler can be run using the `trun` command line utility.
Example:

```
trun "return 'hello'"
```

To see the C output, try:

```
trun --gen-C "print 'hello world'"
```

## Testing

At the moment we have a couple of simple test driver programs: `tparse` and `trun`. These drivers take a string or file input which must be a valid Lua/Ravi chunk of code, and output the AST, the result of type checking, linear IR output if supported, and the CFG as a `dot` file. Example of the output can be found in the `tests/expected` folder.

Suppose `trun` was built in `build` folder then you can run the tests as follows:

```
cd tests && sh truntests.sh ../build/trun
```

The test script compares the output to the expected output. Any difference will cause the test script to fail.
