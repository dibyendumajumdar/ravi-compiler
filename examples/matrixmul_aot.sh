../build/trun --gen-C -main mymain -f matrixmul_lib.lua > matrixmul_lib.c
gcc -O2 -shared -fpic matrixmul_lib.c -o matrixmul_lib.so
../../ravi/build/ravi matrixmul_aot.lua