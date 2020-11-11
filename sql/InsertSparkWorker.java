package dbos.procedures;

import org.voltdb.*;

public class InsertSparkWorker extends VoltProcedure {
    
    public final SQLStmt insert = new SQLStmt (
        "INSERT INTO Worker VALUES (?, ?, ?, ?, ?);"
    );

    public final SQLStmt insertData = new SQLStmt (
        "INSERT INTO DataLocation VALUES (?, ?);"
    );

    public long run(int workerID, int capacity, int workerData, int pkey, String url) throws VoltAbortException {
        voltQueueSQL(insert, workerID, capacity, workerData, pkey, url);
        voltExecuteSQL();
        voltQueueSQL(insertData, workerData, pkey);
        voltExecuteSQL();
        return 0;
    }
}
