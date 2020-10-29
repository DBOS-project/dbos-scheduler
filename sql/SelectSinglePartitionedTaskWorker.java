package dbos.procedures;

import org.voltdb.*;

public class SelectSinglePartitionedTaskWorker extends VoltProcedure {

    final long SUCCESS = 0;
    final long NOWORKER = -2;

    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity FROM Worker WHERE PKey=? AND Capacity > 0 LIMIT 1;"
    );
    
    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    public final SQLStmt insertTask = new SQLStmt (
        "INSERT INTO Task VALUES (?, ?, ?, ?);"
    );

    public long run(int pkey, long taskID) throws VoltAbortException {
    // Select an available Worker.
        voltQueueSQL(selectWorker, pkey);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        if (r.getRowCount() < 1) {
        // If no worker is available, return NOWORKER failure.
            return NOWORKER;
        }
        
        long workerID = r.fetchRow(0).getLong(0);
        long capacity = r.fetchRow(0).getLong(1);
        long state = 1; //RUNNING?
        
    // If a worker is available, insert task into table and update the worker capacity.	
        voltQueueSQL(insertTask, taskID, workerID, state, pkey);
        // voltExecuteSQL();

        voltQueueSQL(updateCapacity, capacity - 1, pkey, workerID);
        voltExecuteSQL();

        return SUCCESS;
    }
}
