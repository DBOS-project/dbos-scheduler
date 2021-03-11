package dbos.procedures;

import org.voltdb.*;
import dbos.procedures.NestedSelectWorkerA;

public class NestedSelectWorkerB extends VoltProcedure {
    
    public final SQLStmt updateCapacity = new SQLStmt(
        "UPDATE Worker SET Capacity=? WHERE PKey=? AND WorkerID=?;"
    );

    // Would not compile, cannot use EXEC here.
    /*
    public final SQLStmt selectWorker = new SQLStmt(
      "exec NestedSelectWorkerA ?;"
    );
    */

    public long run(int pkey) throws VoltAbortException {
        // Pass the same pkey to NestedSelectWorkerA. Otherwise, undefined.
        // Issue: got null pointer exception. Maybe because we must run in a runner.
        // final NestedSelectWorkerA procA = new NestedSelectWorkerA();
        // VoltTable[] results = procA.run(pkey);
        voltQueueSQL(selectWorker, pkey);
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
