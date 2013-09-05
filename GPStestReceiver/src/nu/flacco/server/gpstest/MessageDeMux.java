package nu.flacco.server.gpstest;

import java.sql.SQLException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Random;

import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonParser;

public class MessageDeMux {

      private final org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(getClass().getName());
      String jsonResponse=null;
      UpdateRecord updateRecord;

      public void demux(String indata)
      {

          try {

                 Gson gson = new Gson();

                 JsonParser parser = new JsonParser();
                 JsonArray array = parser.parse(indata).getAsJsonArray();
                 String type = gson.fromJson(array.get(0), String.class);

                 logger.info("Request is: "+ type);
                 System.out.println("Request is: "+ type);

                 if (type.compareTo("update")==0) {
                    updateRecord = gson.fromJson(array.get(1), UpdateRecord.class);
/*
                    jsonResponse = gson.toJson(list);

*/
            //     } else if (type.toUpperCase().compareTo("NEWGAMEREQUEST")==0) {

                 }

          //  } catch (GPServerException ge) {
                // TODO Auto-generated catch block
          //      ge.printStackTrace();
            } catch (Exception e) {
                e.printStackTrace();
            }
      }

          /*
          DBConnect db;

          db = new DBConnect("scotyard");
             ngrDB.update(db);

                try {
                    db.c.commit();
                    db.c.close();
                } catch (SQLException e) {
                    throw new AIAHException(e);
                }
            */


      // Convert Seconds into nice time string.
      public String mkTimeString(int duration)
      {
                int seconds=duration;
                int minutes=0;
                int hours=0;
                int days=0;
                if (seconds>59) {
                    minutes = seconds / 60; // min
                    seconds = seconds % 60;

                       if (minutes>59) {
                           hours  = minutes / 60;
                           minutes = minutes % 60;
                           if (hours>23) {
                               days=hours/24;
                               hours= hours % 24;
                           }
                       }
                }

                String s;
                if (days>0)
                    s = String.format("%d days, %d hrs, %d min, %d sec", days, hours, minutes, seconds);
                else if (hours>0)
                    s = String.format("%d hrs, %d min, %d sec", hours, minutes, seconds);
                else if (minutes>0)
                    s = String.format("%d min, %d sec", minutes, seconds);
                else
                    s = String.format("%d sec", seconds);
                return(s);

      }

      /* Build a response Object for client */
      public String getResponse()
      {
          return(jsonResponse);
      }




    private int getTimestamp()
      {
            Calendar cal = new GregorianCalendar();
            cal.setTime(new Date());
            return (new Long(cal.getTimeInMillis() / 1000).intValue());

      }
}
