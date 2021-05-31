package dbos.procedures;

import java.lang.*;
import org.voltdb.*;

public class Spin extends VoltProcedure {
    public long run(int pkey, long spin_us) throws VoltAbortException {
        long start = System.nanoTime();
        while (System.nanoTime() < start + 1000 * spin_us);

        return (System.nanoTime() - start);
    }
}
