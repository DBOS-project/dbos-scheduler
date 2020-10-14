package dbos.procedures;

import org.voltdb.*;

public class TruncateTaskTable extends VoltProcedure {

    public final SQLStmt truncateTaskTable = new SQLStmt (
        "TRUNCATE TABLE Task;"
    );

    public long run() throws VoltAbortException {
        voltQueueSQL(truncateTaskTable);
        voltExecuteSQL();
        return 0;
    }
}
