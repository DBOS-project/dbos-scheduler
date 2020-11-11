package dbos.procedures;

import org.voltdb.*;

public class PushTask extends VoltProcedure {

    final long SUCCESS = 0;
    final long NOTASK = -1;
    final long NOWORKER = -2;

    // Task states.
    final long PENDING = 1;
    final long RUNNING = 2;
    final long COMPLETE = 3;


    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity, Url FROM Worker WHERE PKey=? AND Capacity > 0 LIMIT 1;"
    );

    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    public final SQLStmt insertTask = new SQLStmt (
        "INSERT INTO Task VALUES (?, ?, ?, ?);"
    );

    public final SQLStmt insertTaskAssign = new SQLStmt (
        "INSERT INTO TaskAssign VALUES(?, ?, ?);"
    );

    public long run(int pkey, long taskID) throws VoltAbortException {
        // Select an available worker.
        voltQueueSQL(selectWorker, pkey);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
	    // If no worker is available, return NOTASK failure.
            return NOWORKER;
        }
        long workerID = r.fetchRow(0).getLong(0);
        long capacity = r.fetchRow(0).getLong(1);
        String url = r.fetchRow(0).getString(2);
        long state = RUNNING; // RUNNING

        // If a worker is available, insert task into table and update the worker capacity.
        voltQueueSQL(insertTask, taskID, workerID, state, pkey);
        voltQueueSQL(updateCapacity, capacity - 1, pkey, workerID);
        voltQueueSQL(insertTaskAssign, taskID, url, pkey);
        voltExecuteSQL();

        return SUCCESS;
    }
}
