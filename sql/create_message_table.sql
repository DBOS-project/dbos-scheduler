CREATE TABLE Message (
    ReceiverID INTEGER NOT NULL,
    SenderID INTEGER NOT NULL,
    MessageID BIGINT NOT NULL,
    Data VARCHAR(1048576) NOT NULL,
    Received INTEGER NOT NULL
);
PARTITION TABLE Message ON COLUMN ReceiverID;
CREATE INDEX receivedIndex ON Message (Received);
