package dbos.procedures;

import org.voltdb.*;

public class SelectTaskWorker extends VoltProcedure {

    final long SUCCESS = 0;
    final long NOTASK = -1;
    final long NOWORKER = -2;

    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity FROM Worker WHERE Capacity > 0 LIMIT 1;"
    );

    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE WorkerID=?;"
    );

    public final SQLStmt selectTask = new SQLStmt (
        "SELECT TaskID FROM Task LIMIT 1;"
    );

    public final SQLStmt updateTask = new SQLStmt (
        "UPDATE Task SET WorkerID=? WHERE TaskID=?;"
    );

    public long run() throws VoltAbortException {
	// Select any available Task.
        voltQueueSQL(selectTask);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
            return NOTASK;
        }
        long taskID = r.fetchRow(0).getLong(0);

        voltQueueSQL(selectWorker);
        results = voltExecuteSQL();
        r = results[0];
        if (r.getRowCount() < 1) {
            return NOWORKER;
        }
        // If a worker is available, update its capacity and the task table.
        long workerID = r.fetchRow(0).getLong(0);
        long capacity = r.fetchRow(0).getLong(1);
        voltQueueSQL(updateCapacity, capacity - 1, workerID);
        voltExecuteSQL();

        voltQueueSQL(updateTask, workerID, taskID);
        voltExecuteSQL();
        return SUCCESS;
    }
}
