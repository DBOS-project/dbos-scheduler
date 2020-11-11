load classes build/DBOSProcedures.jar;

DROP PROCEDURE InsertTask IF EXISTS;
CREATE PROCEDURE FROM CLASS dbos.procedures.InsertTask;

DROP PROCEDURE InsertWorker IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey PARAMETER 2 FROM CLASS dbos.procedures.InsertWorker;

DROP PROCEDURE InsertSparkWorker IF EXISTS;
CREATE PROCEDURE FROM CLASS dbos.procedures.InsertSparkWorker;

DROP PROCEDURE SelectTaskWorker IF EXISTS;
CREATE PROCEDURE FROM CLASS dbos.procedures.SelectTaskWorker;

DROP PROCEDURE SelectWorker IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectWorker;

DROP PROCEDURE SelectPartitionedTaskWorker IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectPartitionedTaskWorker;

DROP PROCEDURE SelectSinglePartitionedTaskWorker IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectSinglePartitionedTaskWorker;

DROP PROCEDURE SelectSparkWorker IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectSparkWorker;

DROP PROCEDURE TruncateTaskTable IF EXISTS;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateTaskTable;

DROP PROCEDURE TruncateWorkerTable IF EXISTS;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateWorkerTable;

DROP PROCEDURE FinishWorkerTask IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.FinishWorkerTask;

DROP PROCEDURE ScanPartitionedTaskWorker IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Task COLUMN PKey FROM CLASS dbos.procedures.ScanPartitionedTaskWorker;

DROP PROCEDURE ScanTaskWorker IF EXISTS;
CREATE PROCEDURE FROM CLASS dbos.procedures.ScanTaskWorker;

DROP PROCEDURE WorkerSelectTask IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Task COLUMN PKey FROM CLASS dbos.procedures.WorkerSelectTask;

DROP PROCEDURE WorkerUpdateTask IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Task COLUMN PKey FROM CLASS dbos.procedures.WorkerUpdateTask;

DROP PROCEDURE PushTask IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.PushTask;

DROP PROCEDURE SelectDataShardPartition IF EXISTS;
CREATE PROCEDURE FROM CLASS dbos.procedures.SelectDataShardPartition;
