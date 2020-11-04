package dbos.procedures;

import org.voltdb.*;

// Update a task state on a worker.
public class WorkerUpdateTask extends VoltProcedure {

    // Task states.
    final long PENDING = 1;
    final long RUNNING = 2;
    final long COMPLETE = 3;

    // status
    final long SUCCESS = 0;

    public final SQLStmt updateTask = new SQLStmt(
        "UPDATE Task SET State=? WHERE PKey=? AND TaskID=? AND WorkerID=?;"
    );

    public long run(int pkey, long workerId, long taskId, long taskState) throws VoltAbortException {
        // TODO: add sanity check that taskState is valid?
        voltQueueSQL(updateTask, taskState, pkey, taskId, workerId);
        voltExecuteSQL();
        return SUCCESS;
    }
}
