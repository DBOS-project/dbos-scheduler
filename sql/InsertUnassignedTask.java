package dbos.procedures;

import org.voltdb.*;

public class InsertUnassignedTask extends VoltProcedure {

    public final SQLStmt insert = new SQLStmt (
        "INSERT INTO TaskQueue VALUES (?, ?);"
    );


    public long run(int taskID, int pKey) throws VoltAbortException {
        voltQueueSQL(insert, taskID, pKey);
        voltExecuteSQL();
        return 0;
    }
}