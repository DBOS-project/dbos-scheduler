ECHO "Task state: 0=unknwon, 1=pending, 2=running, 3=completed"
CREATE TABLE Task (
    TaskID INTEGER NOT NULL,
    WorkerID INTEGER NOT NULL,
    State INTEGER NOT NULL,
    PKey INTEGER NOT NULL
);
PARTITION TABLE Task ON COLUMN PKey;
CREATE ASSUMEUNIQUE INDEX taskIDIndex ON Task (taskID);
CREATE INDEX stateIndex ON Task (State);
