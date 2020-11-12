package dbos.procedures;

import org.voltdb.*;

public class AssignUnassignedTask extends VoltProcedure {

    final long SUCCESS = 0;
    final long NOTASK = -1;
    final long NOWORKER = -2;

    public final SQLStmt selectUnassignedTask = new SQLStmt (
        "SELECT TaskID FROM TaskQueue WHERE PKey=? LIMIT 1;"
    );

    public final SQLStmt removeUnassignedTask = new SQLStmt (
        "DELETE FROM TaskQueue WHERE PKey=? AND TaskID=?;"
    );
    
    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity FROM Worker WHERE PKey=? AND Capacity > 0 LIMIT 1;"
    );
    
    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    public final SQLStmt insertTask = new SQLStmt (
        "INSERT INTO Task VALUES (?, ?, ?, ?);"
    );

    public long run(int workerPKey, int schedulerPKey) throws VoltAbortException {
        // Select an available Worker.
        voltQueueSQL(selectUnassignedTask, schedulerPKey);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
            // If no unassigned task is available, return NOTASK failure.
            return NOTASK;
        }

        long taskID = r.fetchRow(0).getLong(0);
        
        voltQueueSQL(selectWorker, workerPKey);
        results = voltExecuteSQL();
        r = results[0];
        if (r.getRowCount() < 1) {
            // If no worker is available, return NOWORKER failure.
            return NOWORKER;
        }

        long workerID = r.fetchRow(0).getLong(0);
        long capacity = r.fetchRow(0).getLong(1);
        long state = 1; //RUNNING
        
        // If a worker is available, insert task into table, update the worker capacity,
        // and remove task from queue.
        voltQueueSQL(insertTask, taskID, workerID, state, workerPKey);
        voltQueueSQL(updateCapacity, capacity - 1, workerPKey, workerID);
        voltQueueSQL(removeUnassignedTask, schedulerPKey, taskID);
        voltExecuteSQL();

        return SUCCESS;
    }
}