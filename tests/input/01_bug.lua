local assert = assert
return function(m: integer, n: integer)
  assert(m == n)
  print('testing')
end