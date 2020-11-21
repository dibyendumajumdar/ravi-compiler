command=$1
set -e

echo "testing t00_exprs"
$command -f input/t00_exprs.in > results.out
#cp results.out expected/t00_exprs.expected
diff expected/t00_exprs.expected results.out
rm results.out

echo "testing t01_globals"
$command -f input/t01_globals.in > results.out
#cp results.out expected/t01_globals.expected
diff expected/t01_globals.expected results.out
rm results.out

echo "testing t02_locals"
$command -f input/t02_locals.in > results.out
#cp results.out expected/t02_locals.expected
diff expected/t02_locals.expected results.out
rm results.out

echo "testing t03_types"
$command -f input/t03_types.in > results.out
#cp results.out expected/t03_types.expected
diff expected/t03_types.expected results.out
rm results.out

echo "testing t04_stmts_1"
$command -f input/t04_stmts_1.in > results.out
#cp results.out expected/t04_stmts_1.expected
diff expected/t04_stmts_1.expected results.out
rm results.out

echo "testing t05_upvals"
$command -f input/t05_upvals.in > results.out
#cp results.out expected/t05_upvals.expected
diff expected/t05_upvals.expected results.out
rm results.out

echo "testing t06_close"
$command -f input/t06_close.in > results.out
#cp results.out expected/t06_close.expected
diff expected/t06_close.expected results.out
rm results.out
