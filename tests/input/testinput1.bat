tparse "return"
tparse "return 1"
tparse "return 42, 4.2, true, 'hello'"
tparse "return a"
tparse "return 1+2"
tparse "return 2^3-5*4"
tparse "return 1+1"
tparse "return 1+1+1"
tparse "return 2-3/5*4"
tparse "return 4.2//5"	

tparse "return 0.0"
tparse "return 0"
tparse "return -0//1"
tparse "return 3^-1"
tparse "return (1 + 1)^(50 + 50)"
tparse "return (-2)^(31 - 2)"
tparse "return (-3^0 + 5) // 3.0"
tparse "return 0xF0.0 | 0xCC.0 ~ 0xAA & 0xFD"
tparse "return ~(~0xFF0 | 0xFF0)"
tparse "return ~~-100024.0"
tparse "return ((100 << 6) << -4) >> 2"

tparse "return 2^3^2 == 2^(3^2)"
tparse "return 2^3*4 == (2^3)*4"
tparse "return 2.0^-2 == 1/4 and -2^- -2 == - - -4"
tparse "return not nil and 2 and not(2>3 or 3<2)"
tparse "return -3-1-5 == 0+0-9"
tparse "return -2^2 == -4 and (-2)^2 == 4 and 2*2-3-1 == 0"
tparse ""
tparse "return 2*1+3/3 == 3 and 1+2 .. 3*1 == '33'"
tparse "return not(2+1 > 3*1) and 'a'..'b' > 'a'"

tparse "return '7' .. 3 << 1 == 146"
tparse "return 10 >> 1 .. '9' == 0"
tparse "return 10 | 1 .. '9' == 27"

tparse "return 0xF0 | 0xCC ~ 0xAA & 0xFD == 0xF4"
tparse "return 0xFD & 0xAA ~ 0xCC | 0xF0 == 0xF4"
tparse "return 0xF0 & 0x0F + 1 == 0x10"

tparse "return 3^4//2^3//5 == 2"

tparse "return not ((true or false) and nil)"
tparse "return true or false and nil"

tparse "return (((1 or false) and true) or false) == true"
tparse "return (((nil and true) or false) and true) == false"

tparse "return -(1 or 2) == -1 and (1 and 2)+(-1.25 or -4) == 0.75"
tparse "return (b or a)+1 == 2 and (10 or a)+1 == 11"
tparse "return ((2<3) or 1) == true and (2<3 and 4) == 4"

tparse "return (x>y) and x or y == 2"

tparse "function x() for i = 1, 10 do; print(i); end end"

tparse "function x() local a=1; function y() return function() return a end end; end"


tparse "return @integer 1"
tparse "return @string 'hello'"
tparse "return @table {}"
tparse "return @integer[] {}"
tparse "return @number[] {}"
tparse "return @closure function() end"
tparse "return @number 54.4"
tparse "return @User.Type a"

tparse "return {1,2,3}"
tparse "return {[1] = a}"
tparse "return {a = b}"
tparse "return @integer[]{[1] = 5.5, [2] = 4}"
tparse "return @number[] {[1] = 4, [2] = 5.4}"

tparse "if 1 == 1 then return true else return false end"
tparse "if 1 ~= 1 then return 0 elseif 1 < 2 then return 1 elseif 1 < 2 then return 2 else return 5 end"
tparse "if 1 == 1 then return 'hi' end"
tparse "if 5 + 5 == 10 then return 'got it' else if 6 < 7 then return 4 end"
tparse "if 5 + 5 == 10 then return 'got it' elseif 6 < 7 then return 4 end"

tparse "return 1 and 2"
tparse "return 3 and 4 and 5"

tparse "return 1 or 2"
tparse "return 3 or 4 or 5"

tparse "return x[1]"
tparse "return x()"
tparse "return x[1]()"
tparse "return x[1]:name()"
tparse "return x[1]:name(1,2)"
tparse "return x(), y()"
tparse "return y(x())"

tparse "x = 1"
tparse "x = 1, 2"
tparse "x[1] = 1"
tparse "x[1] = b"
tparse "x[1][1] = b"
tparse "x()"
tparse "x()[1]"
tparse "x()[1](a,b)"
tparse "x,y = 1,2"
tparse "x,y = f()"
tparse "x[1],y[1],c,d = 1,z()"
tparse "x[1][2],y[1],c,d = 1,z()"
tparse "x,y = y,x"
tparse "x,y,z = z,y,x"
tparse "i = 3; i, a[i] = i+1, 20"
tparse "x(y(a[10],5,z()))[1] = 9"
tparse "local i = 0"
tparse "local i, j = 1, 2"
tparse "local a = a"
tparse "local a = b"
tparse "local a, b = x, y"

tparse "local a: integer return a+3"
tparse "local i: integer; return t[i/5]"
tparse "local t: integer[]; return t[0]"
tparse "return f()[1]"
tparse "return x.y[1]"

tparse "local t: integer[] if (t[1] == 5) then return true end return false"
tparse "local t: table local len: integer = #t return len"
tparse "return function(t: table, i: integer) i = #t end"

tparse "::L1:: a = 1; goto L1; return"
tparse "::l1:: do goto l1; x = 1; ::l1:: z = 2 end y = 1; goto l1"
tparse "goto l1; do ::l1:: end"

tparse -f t001.lua
tparse -f t002.lua
tparse -f t003.lua
tparse -f t004.lua
tparse -f t005.lua
tparse -f t006.lua
tparse -f t007.lua
tparse -f t008.lua
tparse -f t009.lua
tparse -f t010.lua
tparse -f t011.lua
tparse -f t012.lua
