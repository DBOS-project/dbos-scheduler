package dbos.procedures;

import org.voltdb.*;

public class InsertWorker extends VoltProcedure {
    
    public final SQLStmt insert = new SQLStmt (
        "INSERT INTO Worker VALUES (?, ?, 0, ?);"
    );

    public long run(int workerID, int capacity, int pkey) throws VoltAbortException {
        voltQueueSQL(insert, workerID, capacity, pkey);
        voltExecuteSQL();
        return 0;
    }
}