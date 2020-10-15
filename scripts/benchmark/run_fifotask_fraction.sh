#!/bin/bash

set -ex
SCRIPT_DIR=$(dirname $(readlink -f $0))

cd $SCRIPT_DIR
mkdir -p runlogs/

declare -a PROBS=("0.0" "0.0001" "0.001" "0.01" "0.1" "0.2" "0.5" "0.8" "1.0")
P=40
W=40
N=100
T=40

for prob in "${PROBS[@]}"; do
  OUTLOG="runlogs/syn-N${N}-W${W}-P${P}-T${T}-prob${prob}.csv"
  ${SCRIPT_DIR}/../../build/bin/SyntheticScheduler -i 5000 -t 15000 -o $OUTLOG \
    -N $N -W $W -P $P -T $T -A fifo-task -p ${prob}
done
