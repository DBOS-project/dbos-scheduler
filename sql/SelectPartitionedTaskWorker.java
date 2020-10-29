package dbos.procedures;

import org.voltdb.*;

public class SelectPartitionedTaskWorker extends VoltProcedure {

    final long SUCCESS = 0;
    final long NOTASK = -1;
    final long NOWORKER = -2;

    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity FROM Worker WHERE PKey=? AND Capacity > 0 LIMIT 1;"
    );

    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    public final SQLStmt selectTask = new SQLStmt (
        "SELECT TaskID FROM Task WHERE PKey=? LIMIT 1;"
    );

    public final SQLStmt updateTask = new SQLStmt (
        "UPDATE Task SET WorkerID=? WHERE PKey=? AND TaskID=?;"
    );

    public long run(int pkey) throws VoltAbortException {
	// Select an available Task.
        voltQueueSQL(selectTask, pkey);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
	    // If no task is available, return NOTASK failure.
            return NOTASK;
        }
        long taskID = r.fetchRow(0).getLong(0);

	// Try to find a worker in  the same partition.
        voltQueueSQL(selectWorker, pkey);
        results = voltExecuteSQL();
        r = results[0];
        if (r.getRowCount() < 1) {
	    // If no worker is available, return NOWORKER failure.
            return NOWORKER;
        }
        // If a worker is available, update its capacity and the task table.
        long workerID = r.fetchRow(0).getLong(0);
        long capacity = r.fetchRow(0).getLong(1);
        voltQueueSQL(updateCapacity, capacity - 1, pkey, workerID);
        // voltExecuteSQL();
        // No need to execute SQL here. can push multiple sql queries then execute in a batch.

        voltQueueSQL(updateTask, workerID, pkey, taskID);
        voltExecuteSQL();
        return 0;
    }
}
