local f = package.load_ravi_lib('matrixmul_lib.so', 'mymain')
assert(f and type(f) == 'function')
matrix = f()
assert(matrix and type(matrix) == 'table')
local n = 1000;
n = math.floor(n/2) * 2;
local t1 = os.clock()
local a = matrix.mul(matrix.gen(n), matrix.gen(n))
local t2 = os.clock()
print("time taken ", t2-t1)
print(a[n/2+1][n//2+1])
print 'Ok'
