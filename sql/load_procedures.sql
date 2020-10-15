load classes build/DBOSProcedures.jar;
CREATE PROCEDURE FROM CLASS dbos.procedures.InsertTask;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.InsertWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.SelectTaskWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectPartitionedTaskWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateTaskTable;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateWorkerTable;
CREATE PROCEDURE FROM CLASS dbos.procedures.FinishWorkerTask;
