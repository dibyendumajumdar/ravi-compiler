local sieve = package.load_ravi_lib('sieve_lib.so', 'mymain')
assert(sieve and type(sieve) == 'function')

local t1 = os.clock()
local count = sieve()
local t2 = os.clock()
print("time taken ", t2-t1)
print(count)
assert(count == 1899)
print 'Ok'