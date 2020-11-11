package dbos.procedures;

import org.voltdb.*;

public class SelectDataShardPartition extends VoltProcedure {
    
    public final SQLStmt selectPKey = new SQLStmt (
        "SELECT DISTINCT PKey FROM DataLocation WHERE DataShard=?;"
    );

    public VoltTable[] run(int dataShard) throws VoltAbortException {
        voltQueueSQL(selectPKey, dataShard);
        return voltExecuteSQL();
    }
}