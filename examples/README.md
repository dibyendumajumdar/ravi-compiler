# Examples

This folder contains examples of code that can be compiled AOT and run via Ravi.
Note that these examples can also be JIT compiled - for an example of how that works, please see [21_matrixmul.lua](https://github.com/dibyendumajumdar/ravi/blob/master/tests/comptests/inputs/21_matrixmul.lua).

The examples assume that Ravi and Ravi Compiler have been built in parallel folders, and the
executables are available in `build` folders.

You can run these examples as follows on a Linux host:

```
sh matrixmul_aot.sh
sh sieve_aot.sh
```

The following explains the steps, using the matrix multiplication example.

```
../build/trun --gen-C -main mymain -f matrixmul_lib.lua > matrixmul_lib.c
```

Above generates C code that represents the Ravi code in `matrixmul_lib.lua`.
The C code will contain a function named `mymain` that will be the equivalent of the Lua chunk.

```
gcc -O2 -shared -fpic matrixmul_lib.c -o matrixmul_lib.so
```

Above creates a shared library.

```
../../ravi/build/ravi matrixmul_aot.lua
```

Above step loads the shared library and then executes it similar to the way a Lua chunk is executed when it is loaded.

```
local f = package.load_ravi_lib('matrixmul_lib.so', 'mymain')
assert(f and type(f) == 'function')
local matrix = f()
```

The `package.load_ravi_lib()` function is a special Ravi extension that loads the `mymain` function from the shared
library. Note that this function behaves like a Lua load, rather than C functions that are normally loaded by Lua.

`matrix` will contain the table that was returned by the `matrixmul_lib.lua` script.



