../build/trun --gen-C -main mymain -f gaussian2_lib.lua > gaussian2_lib.c
gcc -O2 -shared -fpic gaussian2_lib.c -o gaussian2_lib.so
../../ravi/build/ravi gaussian2_aot.lua
