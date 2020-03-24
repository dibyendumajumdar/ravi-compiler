function matmul(a: table, b: table)
  	assert(@integer(#a[1]) == #b);
  	local m: integer, n: integer, p: integer, x: table = #a, #a[1], #b[1], {};
  	local c: table = matrix.T(b); -- transpose for efficiency
  	for i = 1, m do
  		local xi: number[] = table.numarray(p, 0.0)
  		x[i] = xi
  		for j = 1, p do
  			local sum: number, ai: number[], cj: number[] = 0.0, @number[](a[i]), @number[](c[j]);
  			-- for luajit, caching c[j] or not makes no difference; lua is not so clever
  			for k = 1, n do sum = sum + ai[k] * cj[k] end
  			xi[j] = sum;
  		end
  	end
  	return x
  end
return matmul