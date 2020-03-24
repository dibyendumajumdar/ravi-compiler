return function ()
    local sum 
    for j = 1,500 do
        sum = 0.0
        for k = 1,10000 do
            sum = sum + 1.0/(k*k)
        end
    end
    return sum
end