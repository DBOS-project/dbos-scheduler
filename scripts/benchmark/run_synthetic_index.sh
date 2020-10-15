#!/bin/bash

set -ex
SCRIPT_DIR=$(dirname $(readlink -f $0))

cd $SCRIPT_DIR
mkdir -p runlogs/

P=1
N=8

declare -a WORKERS=(1 10 100 1000 10000 100000)
declare -a CAPACITIES=(2000000 200000 20000 2000 200 20)

for i in {0..5}; do
  W=${WORKERS[$i]}
  C=${CAPACITIES[$i]}
  OUTLOG="runlogs/syn-N${N}-W${W}-C${C}-P${P}-index.csv"
  ${SCRIPT_DIR}/../../build/bin/SyntheticScheduler -i 5000 -t 15000 -o ${OUTLOG} \
    -N $N -W ${W} -C ${C} -P ${P}
done
