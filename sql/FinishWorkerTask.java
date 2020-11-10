package dbos.procedures;

import org.voltdb.*;

public class FinishWorkerTask extends VoltProcedure {
    
    public final SQLStmt getCapacity = new SQLStmt (
        "SELECT Capacity FROM Worker WHERE PKey=? AND WorkerID=?;"
    );

    public final SQLStmt updateCapacity = new SQLStmt (
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    public final SQLStmt updateState = new SQLStmt (
        "UPDATE Task SET State=0 WHERE PKey=? AND taskID=?;"
    );
    public long run(int workerID, int taskID, int pkey) throws VoltAbortException {
        voltQueueSQL(getCapacity, pkey, workerID);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
            return -1;
        }
        long capacity = r.fetchRow(0).getLong(0);
        voltQueueSQL(updateCapacity, capacity + 1, pkey, workerID);
        voltQueueSQL(updateState, pkey, taskID);
        voltExecuteSQL();
        return 0;
    }
}
