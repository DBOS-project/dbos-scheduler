package dbos.procedures;

import org.voltdb.*;

public class NestedSelectWorkerA extends VoltProcedure {
    
    public final SQLStmt selectWorker = new SQLStmt (
        "SELECT WorkerID, Capacity FROM Worker WHERE PKey=? AND Capacity > 0 LIMIT 1;"
    );

    public VoltTable[] run(int pkey) throws VoltAbortException {
        voltQueueSQL(selectWorker, pkey);
        VoltTable[] results = voltExecuteSQL();
        return results;
    }
}
