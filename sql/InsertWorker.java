package dbos.procedures;

import org.voltdb.*;

public class InsertWorker extends VoltProcedure {
    
    public final SQLStmt insert = new SQLStmt (
        "INSERT INTO Worker VALUES (?, ?);"
    );

    public long run(int workerID, int capacity) throws VoltAbortException {
        voltQueueSQL(insert, workerID, capacity);
        voltExecuteSQL();
        return 0;
    }
}