package dbos.procedures;

import org.voltdb.*;

public class BroadcastMessage extends VoltProcedure {
    final long SUCCESS = 0;

    public final SQLStmt broadcastMessage = new SQLStmt (
        "INSERT INTO Broadcast VALUES(?);"
    );

    public long run(int msg) throws VoltAbortException {
        voltQueueSQL(broadcastMessage, msg);
        voltExecuteSQL();
        return SUCCESS;
    }
}
