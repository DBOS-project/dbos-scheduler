load classes build/DBOSProcedures.jar;
CREATE PROCEDURE FROM CLASS dbos.procedures.InsertWorker;
CREATE PROCEDURE PARTITION ON TABLE Worker COLUMN PKey FROM CLASS dbos.procedures.SelectWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateWorkerTable;
CREATE PROCEDURE FROM CLASS dbos.procedures.FinishWorkerTask;