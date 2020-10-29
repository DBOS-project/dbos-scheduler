load classes build/DBOSProcedures.jar;
CREATE PROCEDURE FROM CLASS dbos.procedures.InsertTask;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey PARAMETER 2 FROM CLASS dbos.procedures.InsertWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey PARAMETER 3 FROM CLASS dbos.procedures.InsertSparkWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.SelectTaskWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectPartitionedTaskWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectSinglePartitionedTaskWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectSparkWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateTaskTable;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateWorkerTable;
CREATE PROCEDURE FROM CLASS dbos.procedures.FinishWorkerTask;
CREATE PROCEDURE PARTITION ON TABLE Task COLUMN PKey FROM CLASS dbos.procedures.ScanPartitionedTaskWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.ScanTaskWorker;
