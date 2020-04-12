command=$1

$command "return"
$command "return 1"
$command "return 42, 4.2, true, 'hello'"
$command "return a"
$command "return 1+2"
$command "return 2^3-5*4"
$command "return 1+1"
$command "return 1+1+1"
$command "return 2-3/5*4"
$command "return 4.2//5"	

$command "return 0.0"
$command "return 0"
$command "return -0//1"
$command "return 3^-1"
$command "return (1 + 1)^(50 + 50)"
$command "return (-2)^(31 - 2)"
$command "return (-3^0 + 5) // 3.0"
$command "return 0xF0.0 | 0xCC.0 ~ 0xAA & 0xFD"
$command "return ~(~0xFF0 | 0xFF0)"
$command "return ~~-100024.0"
$command "return ((100 << 6) << -4) >> 2"

$command "return 2^3^2 == 2^(3^2)"
$command "return 2^3*4 == (2^3)*4"
$command "return 2.0^-2 == 1/4 and -2^- -2 == - - -4"
$command "return not nil and 2 and not(2>3 or 3<2)"
$command "return -3-1-5 == 0+0-9"
$command "return -2^2 == -4 and (-2)^2 == 4 and 2*2-3-1 == 0"
$command ""
$command "return 2*1+3/3 == 3 and 1+2 .. 3*1 == '33'"
$command "return not(2+1 > 3*1) and 'a'..'b' > 'a'"

$command "return '7' .. 3 << 1 == 146"
$command "return 10 >> 1 .. '9' == 0"
$command "return 10 | 1 .. '9' == 27"

$command "return 0xF0 | 0xCC ~ 0xAA & 0xFD == 0xF4"
$command "return 0xFD & 0xAA ~ 0xCC | 0xF0 == 0xF4"
$command "return 0xF0 & 0x0F + 1 == 0x10"

$command "return 3^4//2^3//5 == 2"

$command "return not ((true or false) and nil)"
$command "return true or false and nil"

$command "return (((1 or false) and true) or false) == true"
$command "return (((nil and true) or false) and true) == false"

$command "return -(1 or 2) == -1 and (1 and 2)+(-1.25 or -4) == 0.75"
$command "return (b or a)+1 == 2 and (10 or a)+1 == 11"
$command "return ((2<3) or 1) == true and (2<3 and 4) == 4"

$command "return (x>y) and x or y == 2"

$command "function x() for i = 1, 10 do; print(i); end end"

$command "function x() local a=1; function y() return function() return a end end; end"


$command "return @integer 1"
$command "return @string 'hello'"
$command "return @table {}"
$command "return @integer[] {}"
$command "return @number[] {}"
$command "return @closure function() end"
$command "return @number 54.4"
$command "return @User.Type a"

$command "return {1,2,3}"
$command "return {[1] = a}"
$command "return {a = b}"
$command "return @integer[]{[1] = 5.5, [2] = 4}"
$command "return @number[] {[1] = 4, [2] = 5.4}"

$command "if 1 == 1 then return true else return false end"
$command "if 1 ~= 1 then return 0 elseif 1 < 2 then return 1 elseif 1 < 2 then return 2 else return 5 end"
$command "if 1 == 1 then return 'hi' end"
$command "if 5 + 5 == 10 then return 'got it' else if 6 < 7 then return 4 end"
$command "if 5 + 5 == 10 then return 'got it' elseif 6 < 7 then return 4 end"

$command "return 1 and 2"
$command "return 3 and 4 and 5"

$command "return 1 or 2"
$command "return 3 or 4 or 5"

$command "return x[1]"
$command "return x()"
$command "return x[1]()"
$command "return x[1]:name()"
$command "return x[1]:name(1,2)"
$command "return x(), y()"
$command "return y(x())"

$command "x = 1"
$command "x = 1, 2"
$command "x[1] = 1"
$command "x[1] = b"
$command "x[1][1] = b"
$command "x()"
$command "x()[1]"
$command "x()[1](a,b)"
$command "x,y = 1,2"
$command "x,y = f()"
$command "x[1],y[1],c,d = 1,z()"
$command "x[1][2],y[1],c,d = 1,z()"
$command "x,y = y,x"
$command "x,y,z = z,y,x"
$command "i = 3; i, a[i] = i+1, 20"
$command "x(y(a[10],5,z()))[1] = 9"
$command "local i = 0"
$command "local i, j = 1, 2"
$command "local a = a"
$command "local a = b"
$command "local a, b = x, y"

$command "local a: integer return a+3"
$command "local i: integer; return t[i/5]"
$command "local t: integer[]; return t[0]"
$command "return f()[1]"
$command "return x.y[1]"

$command "local t: integer[] if (t[1] == 5) then return true end return false"
$command "local t: table local len: integer = #t return len"
$command "return function(t: table, i: integer) i = #t end"

$command "::L1:: a = 1; goto L1; return"
$command "::l1:: do goto l1; x = 1; ::l1:: z = 2 end y = 1; goto l1"
$command "goto l1; do ::l1:: end"

$command -f input/t001.lua
$command -f input/t002.lua
$command -f input/t003.lua
$command -f input/t004.lua
$command -f input/t005.lua
$command -f input/t006.lua
$command -f input/t007.lua
$command -f input/t008.lua
$command -f input/t009.lua
$command -f input/t010.lua
$command -f input/t011.lua
$command -f input/t012.lua

$command "for i=1,10 do print(i) end"
$command "for i=10,1,-1 do print(i) end"

$command "while true do print('forever') end"
$command "repeat print('forever') brek until true"