CREATE TABLE Worker (
    WorkerID INTEGER NOT NULL,
    Capacity INTEGER NOT NULL,
    DataShards Integer NOT NULL,
    PKey INTEGER NOT NULL
);
PARTITION TABLE Worker ON COLUMN PKey;
CREATE ASSUMEUNIQUE INDEX workerIDIndex ON Worker (workerID);
CREATE INDEX capacityIndex ON Worker (Capacity);
CREATE INDEX dataShardsIndex ON Worker (DataShards, Capacity);