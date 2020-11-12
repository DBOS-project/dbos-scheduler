CREATE TABLE TaskQueue (
    TaskID INTEGER NOT NULL,
    /*ExecutionTime INTEGER NOT NULL,*/
    PKey INTEGER NOT NULL
);
PARTITION TABLE TaskQueue ON COLUMN PKey;
CREATE ASSUMEUNIQUE INDEX unassignedTaskIDIndex ON Task (taskID);
/*CREATE INDEX executionTimeIndex ON TaskQueue (ExecutionTime);*/