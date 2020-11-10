#!/bin/bash
set -ex

SCRIPT_DIR=$(dirname $(readlink -f $0))
# Enter the root dir of the repo.
cd ${SCRIPT_DIR}/../

# Create obj directory
mkdir -p obj/

# On supercloud, use the enterprise version. On local cluster, change the path.
VOLT_HOME=${VOLT_HOME:-"/home/gridsan/groups/DBOS/shared/VoltDB/voltdb-ent-9.3.2"}
VOLTDB_BIN="${VOLT_HOME}/bin"

javac -classpath "$VOLT_HOME/voltdb/*" -d obj sql/*.java 
jar cvf build/DBOSProcedures.jar -C obj .   
${VOLTDB_BIN}/sqlcmd < sql/load_procedures.sql
