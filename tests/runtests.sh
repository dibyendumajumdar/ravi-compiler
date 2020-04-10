command=$1
set -e

sh ./runtparse.sh $command > results.out
diff expected/results.expected results.out
rm results.out
