package dbos.procedures;

import org.voltdb.*;

public class ReceiveMessage extends VoltProcedure {

    public final SQLStmt receiveMessage = new SQLStmt (
      "SELECT SenderID, MessageID, Data FROM Message WHERE ReceiverID=?;"
    );

    public final SQLStmt ackMessage = new SQLStmt (
      "DELETE FROM Message WHERE ReceiverID=?;"
    );

    public VoltTable[] run(int receiverID) throws VoltAbortException {
      voltQueueSQL(receiveMessage, receiverID);
      VoltTable[] results = voltExecuteSQL();
      VoltTable r = results[0];
      if (r.getRowCount() < 1) {
        // No message.
        return results;
      }

      voltQueueSQL(ackMessage, receiverID);
      voltExecuteSQL();

      return results;
    }
}
