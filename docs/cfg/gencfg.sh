doit()
{
    code=$1
    name=$2
    echo "$code" > $name.input
    ../../build/trun --noastdump --noirdump --nocodump --remove-unreachable-blocks "$code" > $name.gv
    dot -Tsvg -o$name.svg $name.gv
}

doit "if 1 < 2 then print 'hi' end" "if1"
doit "if 1 < 2 then print 'hi' else print 'no' end" "if2"
doit "if a < b then print 'case 1' elseif a==b then print 'case2' elseif a >= b then print 'case 3' else print 'last case' end" "if3"
# doit "function x(a, b, c, d, e) if a == b then goto l1 elseif a == c then goto l2 elseif a == d then goto l2 else if a == e then goto l3 else goto l3 end end ::l1:: ::l2:: ::l3:: ::l4:: end" "if4-goto"
doit "local a, b, c, d, e if a == b then goto l1 elseif a == c then goto l2 elseif a == d then goto l2 else if a == e then goto l3 else goto l3 end end ::l1:: print 'hello' ::l2:: ::l3:: ::l4::" "if4-goto"

doit "for i=1,10 do print(i) end" "for1"
doit "for i=1,10 do for j = 10,1,-1 do print(i,j) end end" "for2"
doit "for i=1,10 do for j = 10,1,-1 do for k=1,3,2 do print(i,j,k) end end end" "for3"
doit "for i =1,10 do if i == 2 then break end end" "for4"

doit "return a and b" "log1"
doit "return a or b" "log2"
doit "return a or b and c" "log3"
doit "return a and b or c" "log4"
doit "return not b" "log5"

doit "while true do print('forever') end" "while1"
# doit "function x(a) while a < 10 do a = a + 1 end end" "while2"
doit "local a while a < 10 do a = a + 1 end" "while2"
doit "repeat print('forever') break until true return" "repeat1"
