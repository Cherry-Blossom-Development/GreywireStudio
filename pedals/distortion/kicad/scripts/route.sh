# Autoroute one board with Freerouting and bring the result back into KiCad.
# usage: bash route.sh <project> [max passes, default 100]  (stopped after 90 min regardless)
# needs build/freerouting.jar (Freerouting 2.1.x, Java 21): https://github.com/freerouting/freerouting/releases
cd "$(dirname "$0")" && . ./env.sh
"$KP" route.py $1 export && \
(cd build && timeout 5400 java -jar freerouting.jar -de $1.dsn -do $1.ses --router.max_passes=${2:-100} -mt 4 --gui.enabled=false > fr_$1.log 2>&1)
grep -o '"incomplete_count": [0-9]*' build/fr_$1.log | tail -1
[ -s build/$1.ses ] && "$KP" route.py $1 import
