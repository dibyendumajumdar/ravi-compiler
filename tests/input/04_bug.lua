-- issue #66
local function test_upvaluejoin()
    local debug = require'debug'
    local foo1, foo2, foo3, foo4
    do
      local a:integer, b:integer = 3, 5
      local c:number = 7.1
      foo1 = function() return a+b end
      foo2 = function() return b+a end
      foo4 = function() return c end
      do
        local a: integer = 10
        foo3 = function() return a+b end
      end
    end
end