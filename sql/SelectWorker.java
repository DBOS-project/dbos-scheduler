package dbos.procedures;

import org.voltdb.*;

public class SelectWorker extends VoltProcedure {
    
    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity FROM Worker WHERE PKey=? AND Capacity > 0 LIMIT 1;"
    );

    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    public long run(int pkey) throws VoltAbortException {
        voltQueueSQL(selectWorker, pkey);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
            return -1;
        }
        long workerID = r.fetchRow(0).getLong(0);
        long capacity = r.fetchRow(0).getLong(1);
        voltQueueSQL(updateCapacity, capacity - 1, pkey, workerID);
        voltExecuteSQL();
        return workerID;
    }
}