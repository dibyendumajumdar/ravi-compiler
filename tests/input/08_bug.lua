do
  local y
  local z = function()
    local x=function()
      local j:number[] = {}
      return j
    end
    y=x()
    y[1] = 99.67
    assert(y[1], 99.67)
    assert(@integer (#y) == 1)
  end
  z()
end