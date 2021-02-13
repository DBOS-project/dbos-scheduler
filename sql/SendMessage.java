package dbos.procedures;

import org.voltdb.*;

public class SendMessage extends VoltProcedure {
    final long SUCCESS = 0;

    public final SQLStmt sendMessage = new SQLStmt (
        "INSERT INTO Message VALUES(?,?,?,?);"
    );

    public long run(int receiverID, int senderID, long messageID, String data) throws VoltAbortException {
        voltQueueSQL(sendMessage, receiverID, senderID, messageID, data);
        voltExecuteSQL();
        return SUCCESS;
    }
}
