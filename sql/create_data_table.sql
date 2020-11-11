CREATE TABLE DataLocation (
    DataShard INTEGER NOT NULL,
    PKey INTEGER NOT NULL
);
CREATE INDEX DataShardIndex ON DataLocation (DataShard);
