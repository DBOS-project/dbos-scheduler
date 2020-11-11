package dbos.procedures;

import org.voltdb.*;

public class TruncateWorkerTable extends VoltProcedure {
    
    public final SQLStmt truncateWorkerTable = new SQLStmt (
        "TRUNCATE TABLE Worker;"
    );

    public final SQLStmt truncateDataShardTable = new SQLStmt (
        "TRUNCATE TABLE DataLocation;"
    );

    public long run() throws VoltAbortException {
        voltQueueSQL(truncateWorkerTable);
        voltExecuteSQL();
        voltQueueSQL(truncateDataShardTable);
        voltExecuteSQL();
        return 0;
    }
}