package au.com.aapt;

import java.io.*;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

import javax.servlet.*;
import javax.servlet.http.*;

import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

public class MainJson {

      public MainJson()
      {

          BtaDataSource btads = new  BtaDataSource();
          CustomerDataOrder cdo = btads.getDataOrderTraffic(137839, 2336807);

          JSONArray arrayObj=new JSONArray();
          JSONObject jsonObj=new JSONObject();

       /* test
       arrayObj.add("MCA");
      arrayObj.add("Amit Kumar");
      arrayObj.add("19-12-1986");
      arrayObj.add(24);
      arrayObj.add("Scored");
      arrayObj.add(new Double(66.67));
      */

      jsonObj.put("dataorder", cdo);

      // arrayObj.add(cdo);

       System.out.println(jsonObj.toString());

       CustomerDataOrder cdo2= new CustomerDataOrder(jsonObj.getJSONObject("dataorder"));

       float percent = (float) 12.34;
       String s = String.format("%.1f%% %.1fGB/%.0fGB", percent,
               (float) cdo.getToUsage()/1000,
               (float) cdo.getTrafficIncluded()/1000);


      // JSONObject jsondo = jsonObj.getJSONObject("dataorder");
     //  String custname = jsondo.getString( "customer" );


       System.out.println(s);

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
                  System.out.println("param="+param);
                  String name = param.split("=")[0];
                  String value = param.split("=")[1];
                  map.put(name, value);
              }
              return map;
          } catch (Exception e) {
              return null;  // bad encoding
          }
      }

      public static void main2(String argv[])
      {
          String user=null;
          String dataorder=null;
          String password=null;

          Map <String, String>params= getQueryMap("user=fred&password=fred3&dataorder=123");
          if (params!=null) {
              Set<String> keys = params.keySet();
              for (String key : keys) {
                  if (key.compareTo("user")==0) user=params.get(key);
                  if (key.compareTo("password")==0) password=params.get(key);
                  if (key.compareTo("dataorder")==0) dataorder=params.get(key);
              }
          }
          System.out.println(user +","+ password + ","+dataorder);
      }

      public static void main6(String argv[])
      {
        Calendar cal = new GregorianCalendar();

        Date nowtime = new Date();
        cal.setTime(nowtime);
        int nowday=cal.get(cal.DAY_OF_MONTH);
        int days = cal.getActualMaximum(Calendar.DAY_OF_MONTH);

        System.out.println("day="+nowday + "days="+days);

        DateFormat df = new SimpleDateFormat("dd-MMM-yyyy HH:mm");

        System.out.println("date="+df.format(nowtime));
      }

      public static void main7(String argv[])
      {
          //int number =10;
          //String result =String.format("%3d%%", number);
          //System.out.println(result);
          
          float percent = ((float) 93.1 / (float) 100) * 100;
          System.out.println(String.format("%3.0f",percent));

          
      }

      public static void main(String argv[])
      {
          EncryptUtil eu = new EncryptUtil();

          String url="user=10264@customer.connect.com.au&password=10264&dataorder=123";
// HhYcUk1ZQlNFUWAWBhYQTwsKAA47IDxDDA0TRQYWTV4JB0cDBFMGBAoWRFteQhJue3RJCBoGBBcdRQJVQ1NA   // apache Base64
// HhYcUk1ZQlNFUWAWBhYQTwsKAA47IDxDDA0TRQYWTV4JB0cDBFMGBAoWRFteQhJue3RJCBoGBBcdRQJVQ1NA   // android Base64

          String key="key phrase used for XOR-ing";

          String estring = eu.encodeXor64(url, key);
          System.out.println("xor64=" + estring);
          byte[] b = estring.getBytes();
          for (int i=0; i<b.length;i++) {
              System.out.print(String.format("%d ", b[i]));
          }
          System.out.println("decode=" + eu.decodeXor64(estring, key));

         // new MainJson();
      }
      public static void main4(String argv[])
      {
          Authenticate auth = new Authenticate();
          System.out.println(auth.login("10264@customer.connect.com.au", "10264"));
      }
}