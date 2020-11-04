package dbos.procedures;

import org.voltdb.*;

// Select top-K tasks that are pending and assigned to this worker.
// Update task status to running.
public class WorkerSelectTask extends VoltProcedure {

    // Task states.
    final long PENDING = 1;
    final long RUNNING = 2;
    final long COMPLETE = 3;

    public final SQLStmt selectPendingTasks = new SQLStmt (
        "SELECT TaskID FROM Task WHERE PKey=? AND WorkerID=? AND State=1 LIMIT ?;"
    );
    
    public final SQLStmt updateTaskRunning = new SQLStmt(
        "UPDATE Task SET State=2 WHERE PKey=? AND TaskID=? AND WorkerID=?;"
    );

    public VoltTable[] run(int pkey, long workerId, long topk) throws VoltAbortException {
        // Select pending topk tasks.
        voltQueueSQL(selectPendingTasks, pkey, workerId, topk);
        VoltTable[] results = voltExecuteSQL();
        VoltTable r = results[0];
        int numTasks = r.getRowCount();
        if (numTasks < 1) {
            // Return empty table.
            return results;
        }
        
        // Iteratively update tasks.
        for (int i = 0; i < numTasks; i++) {
          long taskId = r.fetchRow(i).getLong(0);
          voltQueueSQL(updateTaskRunning, pkey, taskId, workerId);
        }
        
        voltExecuteSQL();

        // Return task ids.
        return results;
    }
}
