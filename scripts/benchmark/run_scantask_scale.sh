#!/bin/bash

set -ex
SCRIPT_DIR=$(dirname $(readlink -f $0))

cd $SCRIPT_DIR
mkdir -p runlogs/

declare -a TASKS=(1 10 100 1000 10000 100000)
P=1
W=16
N=8

for T in "${TASKS[@]}"; do
  OUTLOG="runlogs/scan-N${N}-W${W}-P${P}-T${T}.csv"
  ${SCRIPT_DIR}/../../build/bin/SyntheticScheduler -i 5000 -t 15000 -o $OUTLOG \
    -N $N -W $W -P $P -T $T -p 0.0 -A scan-task
done

N=100
P=40
T=10000
p="1.0" # change to 0.0 for single-partition test

declare -a WORKERS=(1 20 40 60 80 100)
for W in "${WORKERS[@]}"; do
  OUTLOG="runlogs/scan-N${N}-W${W}-P${P}-T${T}-prob${p}.csv"
  ${SCRIPT_DIR}/../../build/bin/SyntheticScheduler -i 5000 -t 15000 -o $OUTLOG \
    -N $N -W $W -P $P -T $T -p $p -A scan-task
done


