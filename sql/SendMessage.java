package dbos.procedures;

import org.voltdb.*;

public class SendMessage extends VoltProcedure {
    final long SUCCESS = 0;

    public final SQLStmt sendMessage = new SQLStmt (
        "INSERT INTO Message VALUES(?);"
    );

    public long run(int pkey) throws VoltAbortException {
        voltQueueSQL(sendMessage, pkey);
	voltExecuteSQL();
	return SUCCESS;
    }
}
