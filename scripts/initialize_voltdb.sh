#!/bin/bash
set -ex

# Increase open files limit.
ulimit -n 32768

SCRIPT_DIR=$(dirname $(readlink -f $0))
# Enter the root dir of the repo.
cd ${SCRIPT_DIR}/../

# On supercloud, use the enterprise version. On local cluster, change the path.
VOLT_HOME=${VOLT_HOME:-"/home/gridsan/groups/DBOS/shared/VoltDB/voltdb-ent-9.3.2"}
VOLTDB_BIN="${VOLT_HOME}/bin"

# Create obj directory
mkdir -p obj/sql
mkdir -p obj/exporter

# Add custom exporter BEFORE starting the db.
javac -classpath "$VOLT_HOME/voltdb/*" -d obj/exporter exporter/HttpTaskExporter.java
jar cvf build/HttpTaskExporter.jar -C obj/exporter .
cp build/HttpTaskExporter.jar ${VOLT_HOME}/lib/extension/

# On supercloud, use the shared scripts.
SHARED_DIR="/home/gridsan/groups/DBOS/shared_scripts/voltdb"
bash ${SHARED_DIR}/init_start.sh

# On local cluster, uncomment the following lines.
# voltdb init -f --dir=/var/tmp
# voltdb start -B --ignore=thp --dir=/var/tmp
# sleep 5

# Create tables
bash ${SCRIPT_DIR}/create_tables.sh

# Load stored procedures
bash ${SCRIPT_DIR}/update_voltdb_procedures.sh
