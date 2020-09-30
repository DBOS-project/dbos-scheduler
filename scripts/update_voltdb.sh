#!/bin/bash
set -ex

SCRIPT_DIR=$(dirname $(readlink -f $0))
# Enter the root dir of the repo.
cd ${SCRIPT_DIR}/../

# Create obj directory
mkdir -p obj/

# Reload the procedures.
javac -classpath "$VOLT_HOME/voltdb/*" -d obj sql/*.java 
jar cvf build/DBOSProcedures.jar -C obj .   
echo "load classes build/DBOSProcedures.jar;" | sqlcmd
