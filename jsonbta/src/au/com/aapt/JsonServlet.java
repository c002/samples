package au.com.aapt;

import java.io.*;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import javax.servlet.*;
import javax.servlet.http.*;
import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

import au.com.aapt.EncryptUtil;

public class JsonServlet extends  HttpServlet {

  public void doGet(HttpServletRequest request,  HttpServletResponse response)
   throws ServletException,IOException{
      PrintWriter out = response.getWriter();
      JSONObject jsonObj=new JSONObject();

      request.getAttributeNames();
      String encodedCustomer = request.getParameter("customer");
      System.out.println("encodedCustomer="+ encodedCustomer);

      EncryptUtil encUtil = new EncryptUtil();
      String secretkey="key phrase used for XOR-ing";
      String parameters=encUtil.decodeXor64(encodedCustomer,secretkey);

      System.out.println("Incoming parameters="+ parameters);

      String user=null;
      String dataorder=null;
      String password=null;
      int cid=0;

      Map <String, String>params= getQueryMap(parameters);
      if (params!=null) {
          Set<String> keys = params.keySet();
          for (String key : keys) {
              if (key.compareTo("user")==0) user=params.get(key);
              if (key.compareTo("password")==0) password=params.get(key);
              if (key.compareTo("dataorder")==0) dataorder=params.get(key);
          }
      }

      if (user==null || dataorder==null || password==null)
          response.sendError(404, "Invalid arguments");
      else {

          jsonObj=new JSONObject();
         // jsonObj.put("user", user);
         // jsonObj.put("password", password);
         // jsonObj.put("dataorder", dataorder);

          Authenticate auth = new Authenticate();
          // ((password.compareTo("htest")==0) ||
          if (auth.login(user, password)==true) {

              System.out.println("Login " + user + " Success!");

              BtaDataSource btads = new  BtaDataSource();
              CustomerDataOrder cdo=null;

              // Get customerid from username
              String loginName;
              if (user.indexOf('@') == -1) {
                  loginName = user;
              } else {
                  loginName = user.substring(0, user.indexOf('@'));
              }
              try {
                  // try it as a number
                  cid = Integer.parseInt(loginName);
              }
              catch (NumberFormatException e) {
                  // login contains non numeric characters so
                  // it must be of the form <sunid><cid>

                  // locate first numeric character
                  int pos = 0;
                  while ( pos < loginName.length() && !(loginName.charAt(pos) >= '0' && loginName.charAt(pos) <= '9')) {
                      System.out.println("POS : " + pos);
                      pos++;
                  }
                  if (pos < user.length()) {
                      System.out.println("FOUND : " + loginName.substring(pos) + " from " + loginName);
                      try {
                          cid = Integer.parseInt(loginName.substring(pos));
                      }
                      catch (NumberFormatException e1) {
                          System.out.println("NO cid found in " + loginName);
                          // do nothing
                      }
                  }
                  else {
                      System.out.println("NO cid found in : " + loginName);
                  }
              }

              cdo = btads.getDataOrderTraffic(new Integer(cid), new Integer(dataorder));
              if (cdo!=null) {
                  jsonObj.put("dataorder", cdo);
                  System.out.println("Sending: " + cdo);
              } else
                  jsonObj.put("error", "Invalid DataOrder");

          } else {
              System.out.println("Login " + user + " FAIL!");
              jsonObj.put("error", "Failed to Authenticate");
          }
      }
      out.println(jsonObj.toString());
      out.close();

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