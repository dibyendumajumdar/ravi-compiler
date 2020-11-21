command=$1
set -e

# This is a legacy test runner
# will be replaced by trun based runner truntests.sh

sh ./runtparse.sh $command > results.out
#cp results.out expected/results.expected
diff expected/results.expected results.out
rm results.out
