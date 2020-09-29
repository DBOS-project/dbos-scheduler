#!/bin/bash
set -ex

SCRIPT_DIR=$(dirname $(readlink -f $0))
# Enter the root dir of the repo.
cd ${SCRIPT_DIR}/../

voltdb init --dir=/var/tmp
voltdb start -B --dir=/var/tmp
sleep 5
sqlcmd < sql/create_worker_table.sql

# Create obj directory
mkdir -p obj/

javac -classpath "$VOLT_HOME/voltdb/*" -d obj sql/*.java 
jar cvf build/InsertWorker.jar -C obj .   
sqlcmd < sql/load_procedures.sql
