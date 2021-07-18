-- issue #65
do
  local x = function ()
    local function createarray(values: number[])
        local arr: number[] = table.numarray(#values, 0)
        for i=1,#values do
            arr[i] = values[i]
        end
        return arr
    end
  end
end