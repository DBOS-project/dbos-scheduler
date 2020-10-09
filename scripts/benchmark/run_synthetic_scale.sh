#!/bin/bash

set -ex
SCRIPT_DIR=$(dirname $(readlink -f $0))

cd $SCRIPT_DIR
mkdir -p runlogs/

declare -a SCHEDULERS=(1 10 20 40 80 120 160 200 240)
P=40
W=8000
N=100

for N in "${SCHEDULERS[@]}"; do
  OUTLOG="runlogs/syn-N${N}-W${W}-P${P}.csv"
  ${SCRIPT_DIR}/../../build/bin/SyntheticScheduler -i 5000 -t 15000 -o $OUTLOG \
    -N $N -W $W -P $P
done
