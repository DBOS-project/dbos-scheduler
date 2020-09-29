package dbos.procedures;

import org.voltdb.*;

public class TruncateWorkerTable extends VoltProcedure {
    
    public final SQLStmt truncateWorkerTable = new SQLStmt (
        "TRUNCATE TABLE Worker;"
    );


    public long run() throws VoltAbortException {
        voltQueueSQL(truncateWorkerTable);
        voltExecuteSQL();
        return 0;
    }
}