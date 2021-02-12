package dbos.procedures;

import org.voltdb.*;

public class ReceiveMessage extends VoltProcedure {

    public final SQLStmt receiveMessage = new SQLStmt (
      "SELECT SenderID, MessageID, Data FROM Message WHERE ReceiverID=? AND Received=0 LIMIT 1;"
    );

    public final SQLStmt ackMessage = new SQLStmt (
      "DELETE FROM Message WHERE ReceiverID=? AND MessageID=?;"
    );

    public VoltTable[] run(int receiverID) throws VoltAbortException {
      voltQueueSQL(receiveMessage, receiverID);
      VoltTable[] results = voltExecuteSQL();
      VoltTable r = results[0];
      if (r.getRowCount() < 1) {
        // No message.
        return results;
      }

      long messageID = r.fetchRow(0).getLong(1);
      voltQueueSQL(ackMessage, receiverID, messageID);
      voltExecuteSQL();

      return results;
    }
}
