#!/bin/bash
set -ex

SCRIPT_DIR=$(dirname $(readlink -f $0))
# Enter the root dir of the repo.
cd ${SCRIPT_DIR}/../

# On supercloud, use the enterprise version. On local cluster, change the path.
VOLT_HOME=${VOLT_HOME:-"/home/gridsan/groups/DBOS/shared/VoltDB/voltdb-ent-9.3.2"}
VOLTDB_BIN="${VOLT_HOME}/bin"

# Create tables.
${VOLTDB_BIN}/sqlcmd < sql/create_task_table.sql
${VOLTDB_BIN}/sqlcmd < sql/create_worker_table.sql
${VOLTDB_BIN}/sqlcmd < sql/create_task_stream.sql
${VOLTDB_BIN}/sqlcmd < sql/create_data_table.sql
${VOLTDB_BIN}/sqlcmd < sql/create_message_table.sql
${VOLTDB_BIN}/sqlcmd < sql/create_broadcast_table.sql
${VOLTDB_BIN}/sqlcmd < sql/create_dummy_table.sql
