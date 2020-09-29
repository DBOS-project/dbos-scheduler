package dbos.procedures;

import org.voltdb.*;

public class FinishWorkerTask extends VoltProcedure {
    
    public final SQLStmt getCapacity = new SQLStmt (
        "SELECT Capacity FROM Worker WHERE WorkerID=?;"
    );

    public final SQLStmt updateCapacity = new SQLStmt (
        "UPDATE Worker SET Capacity=? WHERE WorkerID=?;"
    );

    public long run(int workerID) throws VoltAbortException {
        voltQueueSQL(getCapacity, workerID);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
            return -1;
        }
        long capacity = r.fetchRow(0).getLong(0);
        voltQueueSQL(updateCapacity, capacity + 1, workerID);
        voltExecuteSQL();
        return workerID;
    }
}