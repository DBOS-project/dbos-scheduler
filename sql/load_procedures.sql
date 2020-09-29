load classes build/DBOSProcedures.jar;
CREATE PROCEDURE FROM CLASS dbos.procedures.InsertWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.SelectWorker;
CREATE PROCEDURE FROM CLASS dbos.procedures.TruncateWorkerTable;
CREATE PROCEDURE FROM CLASS dbos.procedures.FinishWorkerTask;