#!/bin/bash

voltdb init --dir=/var/tmp
voltdb start -B --dir=/var/tmp
sleep 5
sqlcmd < sql/create_worker_table.sql
javac -classpath "$VOLT_HOME/voltdb/*" -d obj sql/*.java 
jar cvf build/InsertWorker.jar -C obj .   
sqlcmd < sql/load_procedures.sql