package nu.flacco.server.gpstest;

import java.io.*;
import java.net.URLDecoder;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Timer;

import javax.servlet.http.*;
import javax.servlet.*;

import nu.flacco.server.persistence.Devices;
import nu.flacco.server.persistence.Persistence;
import nu.flacco.server.wire.Acknowledge;
import nu.flacco.server.wire.AppError;
import nu.flacco.server.wire.Coordinate;
import nu.flacco.server.wire.GsonSerialiser;
import nu.flacco.server.wire.LocationData;

import com.google.android.gcm.server.*;

public class JsonServlet extends  HttpServlet {

   /**
     * 
     */
   private static final long serialVersionUID = 1L;

   static private AppProps appProps=null;
   private static String props = "application.properties";
   private final org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(getClass().getName());

   private int UPDATE_INTERVAL=60000;		// 60secs
   private enum GAMESTATE {NONE, NEW, WAITJOIN, MRXTURN, PLAYERTURN, END};
   private String myApiKey=null;
   
		   //private static Database tracking_db=null;
   private static Persistence persist;
   
   public int test1=0;
   public static int test2=0;

  // private AppError error=null;

   //MessageDeMux muxer=null;
   Timer timer;
   GAMESTATE gamestate;
   //ArrayList <int> runningGames;
   //Persistence persist;

   public void init(ServletConfig config) throws ServletException
   {
       logger.info("init()");

     //   tracking_db = new Database("tracking");
        persist = new Persistence();
        
        try {
          // tracking_db.getJdbcConnection(true);		// Initialize Connection pool
        } catch (Exception se) {
           logger.error(se.getMessage());
        }

        // Start task manager.
   }


   @Override
  public void doGet(HttpServletRequest request,  HttpServletResponse response)
   throws ServletException,IOException{
     // PrintWriter out = response.getWriter();
     // JSONObject jsonObj=new JSONObject();

      logger.debug("doGet() test1=" + test1++ + ", test2="+test2++);

      try {
        appProps = AppProps.getInstance();
        appProps.loadConfig();
    } catch (GPServerException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
    }

      if (persist==null) {
    	  persist=new Persistence();
          //tracking_db = new Database("tracking");

      } else {
         AppError error= new AppError();
         int status = decodeGetRequest(request, error);
         logger.debug("doGet() status=" + status + ", error is " + (error==null ? "null" : "not null"));
         logger.error("xxx error=" + error);
         sendAck(request, response, status, error);
     }
  }

  @Override
  public void doPost(HttpServletRequest request,  HttpServletResponse response)
           throws ServletException,IOException {

               logger.debug("doPost()");

            try {
                  appProps = AppProps.getInstance();
                appProps.loadConfig();
            } catch (GPServerException e1) {
                // TODO Auto-generated catch block
                e1.printStackTrace();
            }

            String inPostdata = getDataAsString(request, response);

            logger.debug("Request=" + inPostdata);

            if (persist==null) 
          	  persist=new Persistence();

            if (persist==null) {
                  sendAck(request, response, -1, new AppError("Database","Unable to Connect"));
            } else {
                   AppError error= new AppError();
                   int status = decodePostRequest(request, inPostdata, error);
                   sendAck(request, response, status, error);
            }
  }
  private void sendMessageToDevices()
  {
      try {
          logger.debug("sendMessageToDevice()");
          Sender sender = new Sender(myApiKey);
          Message message = new Message.Builder().build();
          List <String> devices = new ArrayList<String>();
          MulticastResult result = sender.send(message, devices, 5);
      } catch (IOException ioe) {
          logger.debug("error sending message to Device(s)");
          ioe.printStackTrace();
      }
  }
  private void sendResponse(HttpServletRequest request,  HttpServletResponse response, String jsonResponse)
  {
      try {
          PrintWriter out = response.getWriter();

          logger.debug("send=" + jsonResponse);

          out.println(jsonResponse);
            out.close();

      } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
  }



  private void sendAck(HttpServletRequest request,  HttpServletResponse response, int status, AppError error)
  {
      logger.debug("sendAck()");
      try {
          PrintWriter out = response.getWriter();
          long timestamp=909;
          String jsonstr;

          if (status>=0) {
              Acknowledge ack = new Acknowledge();

              ack.setStatus(Acknowledge.STATUS_OK);
              if (status==1) ack.setStatus(Acknowledge.STATUS_CONFIRM);
              if (status==2) ack.setStatus(Acknowledge.STATUS_FAIL);
              ack.setMessage("NA");
              ack.setTimestamp(timestamp);

              GsonSerialiser <Acknowledge> gser = new GsonSerialiser<Acknowledge>(Acknowledge.class);
              jsonstr = gser.pack(ack);

          } else {
              GsonSerialiser <AppError> gser = new GsonSerialiser<AppError>(AppError.class);
              jsonstr = gser.pack(error);
          }

          logger.debug("response=" + jsonstr);
          out.println(jsonstr);
            out.close();

      } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
  }


  private String getDataAsString(HttpServletRequest request, HttpServletResponse response)
  {
      StringBuilder builder = new StringBuilder();
     // JSONObject jsonObj=new JSONObject();

      try {
          InputStream content = request.getInputStream();

            BufferedReader reader = new BufferedReader(
                new InputStreamReader(content));
            String line;

            while ((line = reader.readLine()) != null) {
                builder.append(line);
                //logger.debug("line=" + line);
            }
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        // jsonobject
        // Decode messageType

        // Write to Database

        return(builder.toString());
        // return response
  }

  private int decodePostRequest(HttpServletRequest request, String inPostdata, AppError error)
  {
      logger.debug("decodePostRequest() " +  request.getContentLength());
       LocationData locdata;
       UpdateRecord uprec=null;

      try {
         
                 if (GsonSerialiser.getType(inPostdata) == null) {
              logger.debug("Null jsonstr received, server problem.");
         // } else if (GsonSerialiser.getType(indata).compareTo("nu.flacco.server.wire.Devices")==0) {
          //    GsonSerialiser <Devices> gser = new GsonSerialiser<Devices>(Devices.class);
          //    devices = gser.unpack(indata);
          } else if (GsonSerialiser.getType(inPostdata).compareTo("nu.flacco.server.wire.LocationData")==0) {
              GsonSerialiser <LocationData> gser = new GsonSerialiser<LocationData>(LocationData.class);
              locdata = gser.unpack(inPostdata);

              logger.debug("locdata received: " + locdata);
      /*
              uprec = new UpdateRecord();
              
              uprec.setLinenumber(locdata.getLinenumber());
              uprec.setActive(true);
              uprec.setDeviceid(locdata.getDeviceId());
        */      
              //boolean status = tracking_db.updateTrackindata(uprec);
              boolean status = persist.updateTrackindata(locdata);
              if (status==false) {
                  error = persist.getError();
                  return(-1);
              }
              return(0);
          } else
              logger.debug("Unknown type received in json response:" + GsonSerialiser.getType(inPostdata));

          return(1);
      } catch (Exception e) {
          e.printStackTrace();
          return(-1);
      }
  }
 // private void decodeRequest(HttpServletRequest request, HttpServletResponse response)
  private int decodeGetRequest(HttpServletRequest request, AppError error)
  {
      boolean doConfirm=false;

      logger.debug("decodeGetRequest()");
      //request.getAttributeNames();

      if (request==null || request.getQueryString()==null) {
          if (error!=null) {
              error.setError("Local");
              error.setDetail("null request");
          }
          return(-1);
        }

      Map <String , String[]> paramMap = request.getParameterMap();
      //Map  paramMap = request.getParameterMap();
      logger.debug("querystr="+ request.getQueryString());

      try {
          String decoded = URLDecoder.decode(request.getQueryString(), "UTF-8");
        //  logger.debug("querystr dec="+ decoded);
      } catch (java.io.UnsupportedEncodingException ue) {
          ue.printStackTrace();
      }


      //UpdateRecord uprec = new UpdateRecord();
      LocationData locdata = new LocationData();
      Coordinate coord = new Coordinate();
      for (String key : paramMap.keySet()) {
          //logger.debug("key="+ key + ", Val=" + (String) paramMap.get(key));
          logger.debug("key="+ key +",val="+ paramMap.get(key)[0]);

          String val = paramMap.get(key)[0];

          if (key==null) continue;

          if (key.compareTo("cmd")==0) {	// command
               if ( val.compareTo("sendit")==0) {
                   DeviceMessenger dm = new DeviceMessenger(persist, myApiKey);
                   dm.sendMessageToAllDevices("Soup is ready");
               }
          }
          
          if (key.compareTo("id")==0)        locdata.setDeviceId(val);
          if (key.compareTo("timestamp")==0) locdata.setTimestamp(new Long(val));
          if (key.compareTo("lattitude")==0) coord.setLat(new Integer(val));
          if (key.compareTo("longitude")==0) coord.setLon(new Integer(val));
          if (key.compareTo("provider")==0)  locdata.setProvider(val);
          if (key.compareTo("accuracy")==0)  locdata.setAccuracy(new Float(val));
          if (key.compareTo("gcmid")==0)     locdata.setGCMId(val);
          if (key.compareTo("cancel")==0)    locdata.setActive(false);
          if (key.compareTo("confirm")==0)   { locdata.setActive(new Boolean(val)); doConfirm=true; }
      }
      locdata.setCoord(coord);
      
      logger.info("locdata=" + locdata);

      try {
        //  Database db = new Database("tracking");
          boolean status;
        /*
          if (doConfirm==true) {
              status = tracking_db.confirmUpdate(uprec);
              if (status==true)
                  return(1);
              else
                  return(2);
          } else {
          */
              status = persist.updateTrackindata(locdata);

              if (status==false) {
                  error = persist.getError();
                  return(-1);
              }
              return(0);
         // }
      } catch (SQLException se) {
          if (error!=null) {
              error.setError("SQLException");
              error.setDetail(se.getMessage());
          }
          logger.error("SQLException: " + error+","+error.toString());
          se.printStackTrace();
          return (-1);
      } catch (Exception e) {
          if (error!=null) {
              error.setError("Exception");
              error.setDetail(e.getMessage());
          }
          logger.error("Exception: " + error+","+error.toString());
          e.printStackTrace();
          return (-1);
      }
   }

  public static Map<String, String> getQueryMap(String query)
  {
      if (query==null)
          return(null);

      try {
          String[] params = query.split("&");
          Map<String, String> map = new HashMap<String, String>();
          for (String param : params)
          {
              String name = param.split("=")[0];
              String value = param.split("=")[1];
              map.put(name, value);
          }
          return map;
      } catch (Exception e) {
          return(null); // bad encoding
      }
  }

}
