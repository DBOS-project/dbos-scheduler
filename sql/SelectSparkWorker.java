package dbos.procedures;

import org.voltdb.*;

public class SelectSparkWorker extends VoltProcedure {
    
    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity FROM Worker WHERE PKey=? AND Capacity > 0 AND DataShards=? LIMIT 1;"
    );

    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    public long run(int pkey, int targetData) throws VoltAbortException {
        voltQueueSQL(selectWorker, pkey, targetData);
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