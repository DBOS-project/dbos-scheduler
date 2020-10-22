package dbos.procedures;

import org.voltdb.*;

public class ScanPartitionedTaskWorker extends VoltProcedure {

    final long SUCCESS = 0;
    final long NOWORKER = -2;

    public final SQLStmt scanTaskWorker = new SQLStmt (
        "SELECT agg.WorkerID "
        + "FROM (SELECT WorkerID, COUNT(*) AS cnt "
        + "FROM Task WHERE WorkerID >= 0 AND PKey=? "
        + "GROUP BY WorkerID) as agg "
        + "ORDER BY agg.cnt DESC LIMIT 1;"
    );

    // TODO: probably return VoltTable[] is more reasonable.
    public long run(int pkey) throws VoltAbortException {
        // Find the worker with most tasks and get the task count.
        long workerId = NOWORKER;
        voltQueueSQL(scanTaskWorker, pkey);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
	          // If no worker found, return 
            return NOWORKER;
        }
        workerId = r.fetchRow(0).getLong(0);
        return workerId;
    }
}
