package dbos.voltdb.exportclient;

import java.io.*;

import java.net.URL;
import java.net.HttpURLConnection;
import java.util.Properties;

import java.time.LocalTime;

import org.voltdb.VoltType;

import org.voltcore.logging.VoltLogger;

import org.voltdb.client.Client;
import org.voltdb.client.ClientImpl;
import org.voltdb.export.AdvertisedDataSource;
import org.voltdb.exportclient.ExportRow;
import org.voltdb.exportclient.ExportClientBase;
import org.voltdb.exportclient.ExportDecoderBase;

public class HttpTaskExporter extends ExportClientBase {
  private static final VoltLogger m_logger = new VoltLogger("ExportClient");

  @Override
  public void configure(Properties config) throws Exception
  {
    m_logger.debug("Configuring HttpTaskExporter");
    //no-op
  }

  @Override
  public ExportDecoderBase constructExportDecoder(AdvertisedDataSource source)
  {
    HttpTaskDecoder dec = new HttpTaskDecoder(source);
    return dec;
  }

  class HttpTaskDecoder extends ExportDecoderBase {

    public HttpTaskDecoder(AdvertisedDataSource source)
    {
      super(source);
      m_logger.debug("HttpTaskDecoder");
    }

    @Override
    public void sourceNoLongerAdvertised(AdvertisedDataSource source)
    {
      m_logger.debug("sourceNoLongerAdvertised");
    }

    @Override
    public void onBlockStart(ExportRow row) throws RestartBlockException
    {
      m_logger.debug("onBlockStart");
    }

    @Override
    public void onBlockCompletion(ExportRow row) throws RestartBlockException
    {
      m_logger.debug("onBlockCompletion");
    }

    @Override
    public boolean processRow(ExportRow row) throws RestartBlockException
    {
      m_logger.debug("processRow");
      try {
        String url = "";
        int taskID = 0;	
	String insertion_time = "";
	for (int i =0; i < row.values.length; i++) {
          String column_name = row.names.get(i);
	  switch (column_name) {
            case "TASKID":
              taskID = (int) row.values[i];
	      break;
	    case "URL":
	      url = (String) row.values[i];
	      break;
	    case "MYTIME":
              insertion_time = row.values[i].toString();
              break;
            default:
	      break;
	  }
	}
        URL urlObj = new URL(url);
        HttpURLConnection httpCon = (HttpURLConnection) urlObj.openConnection();
	httpCon.setRequestProperty("taskID", String.valueOf(taskID));
	httpCon.setRequestProperty("creation_timestamp", insertion_time);
	httpCon.setRequestProperty("export_timestamp", String.valueOf(LocalTime.now()));

        int responseCode = httpCon.getResponseCode();
        if (responseCode != HttpURLConnection.HTTP_OK) {
          m_logger.error("Not OK HTTP Response code");
          return false;
        }
      } catch (Exception e) {
	m_logger.error("Exception during HTTP", e);
	return false;
      }
      return true;
    }
  }
}
