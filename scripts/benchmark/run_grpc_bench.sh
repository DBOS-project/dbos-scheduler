#!/bin/bash

set -ex
SCRIPT_DIR=$(dirname $(readlink -f $0))

cd $SCRIPT_DIR

# First, start the gRPC server on the remote machine.
# ./bin/GrpcBenchServer -N <numReceivers>

# Then, run this script and provide server name.
SERVER="localhost"

if [[ $# -eq 1 ]]; then
  SERVER="$1"
fi


# For ping-ping-pong-pong
LOGFILE="grpc_pingpong.log"
declare -a msgsizes=(1024 8192)

# Outstanding messages.
declare -a outmsgs=(1 5 10 20 50 100)

# Number of receivers
declare -a numrecvs=(1 10)

for numrecv in "${numrecvs[@]}"; do
  echo "================= Running $numrecv receiver(s) =================" | tee -a ${LOGFILE}
  for msgsize in "${msgsizes[@]}"; do
    echo "================= Running msgsize ${msgsize} =================" | tee -a ${LOGFILE}
    for outmsg in "${outmsgs[@]}"; do
      ${SCRIPT_DIR}/../../build/bin/GrpcBenchClient -N ${numrecv} -s ${SERVER} -m ${msgsize} -M ${outmsg} 2>&1 | tee -a ${LOGFILE}

      # Cool down a bit.
      sleep 5
    done
  done
done


# For broadcast.
LOGFILE="grpc_broadcast.log"
declare -a msgsizes=(1024 8192)

# Number of receivers
declare -a numrecvs=(1 5 10 20 40)

for msgsize in "${msgsizes[@]}"; do
  echo "================= Running msgsize ${msgsize} =================" | tee -a ${LOGFILE}
  for numrecv in "${numrecvs[@]}"; do
    ${SCRIPT_DIR}/../../build/bin/GrpcBenchClient -b -R ${numrecv} -s ${SERVER} -m ${msgsize} -M 1  2>&1 | tee -a ${LOGFILE}

    # Cool down a bit.
    sleep 5
  done
done

