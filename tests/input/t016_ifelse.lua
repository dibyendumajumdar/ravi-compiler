function doit(a: number, what)
    if what == 'add' then
        return a+5
    elseif what == 'sub' then
        return a-5
    elseif what == 'mul' then
        return a*5
    elseif what == 'div' then
        return a/5
    else
        return 0
    end
end