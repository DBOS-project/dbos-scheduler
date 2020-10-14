package dbos.procedures;

import org.voltdb.*;

public class InsertTask extends VoltProcedure {

    public final SQLStmt insert = new SQLStmt (
        "INSERT INTO Task VALUES (?, ?, ?, ?);"
    );

    public long run(int taskID, int workerID, int state, int pkey) throws VoltAbortException {
        voltQueueSQL(insert, taskID, workerID, state, pkey);
        voltExecuteSQL();
        return 0;
    }
}
