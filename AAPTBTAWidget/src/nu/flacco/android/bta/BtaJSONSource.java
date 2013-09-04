package nu.flacco.android.bta;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;

import nu.flacco.android.bta.EncryptUtil;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.StatusLine;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.protocol.BasicHttpContext;
import org.apache.http.protocol.HttpContext;
import org.json.JSONObject;

import android.content.SharedPreferences;
import android.net.http.AndroidHttpClient;
import android.util.Log;
import android.widget.Toast;

// Example custs:
// 137839 do=2336807
// 143156 do=693308

public class BtaJSONSource {

    private static final String LOG = "BtaJSONSource";
    //String url="http://192.94.41.3:8080/jsontest/JsonServlet";
    //String url="http://192.94.41.3:8080/jsonbta";
    //String url="http://tools.connect.com.au:80/jsonbta";
    private String url="http://192.94.41.138/jsonbta";
    private String status=null;

    SharedPreferences preferences;

    public BtaJSONSource()
    {

    }
    public String getBTAData() {

        String username ="10264@customer.connect.com.au";
        String password ="10264";
        String dataorder="123";

        return(getBTAData(username,  password,  dataorder));
    }

    public String getBTAData(String username, String password, String dataorder) {
        StringBuilder builder = new StringBuilder();
        HttpClient client = new DefaultHttpClient();
        //HttpClient client = AndroidHttpClient.newInstance("phone");

      //  HttpGet httpGet = new HttpGet(
      //          "http://inkata.off.connect.com.au:8080/jsontest/JsonServlet");

        /*
         * // proxy
private static final String PROXY = "123.123.123.123";
// proxy host
private static final HttpHost PROXY_HOST = new HttpHost(PROXY, 8080);
HttpParams httpParameters = new BasicHttpParams();
DefaultHttpClient httpClient = new DefaultHttpClient(httpParameters);
httpClient.getParams().setParameter(ConnRoutePNames.DEFAULT_PROXY, PROXY_HOST);
         */

        try {

            String getparameter= "user="+username+"&password="+password+"&dataorder="+dataorder;

            // Encode url with username password + secret.
            // user=fred&password=&fred3&dataorder=123
            EncryptUtil eu = new EncryptUtil();
            String secretkey="key phrase used for XOR-ing";
            String encoded=eu.encodeXor64( getparameter, secretkey );

            String safeUrl = URLEncoder.encode(encoded);
            //String encodedurl=url+"?customer="+encoded;
            String encodedurl=url+"?customer="+safeUrl;

          //  Log.d(LOG, "SendParams="+getparameter+"\n");
          //  System.out.println("rawenc="+encoded+"\n");
          //  System.out.println("safenc="+safeUrl+"\n");
            Log.d(LOG, "Send Safe Url="+encodedurl+"\n");

            HttpGet httpGet = new HttpGet(encodedurl);
            HttpResponse response = client.execute(httpGet);

            StatusLine statusLine = response.getStatusLine();
            int statusCode = statusLine.getStatusCode();
            status = String.valueOf(statusCode);

            Log.d(LOG, "Response StatusCode="+statusCode);

            if (statusCode == 200) {
                HttpEntity entity = response.getEntity();
                InputStream content = entity.getContent();
                BufferedReader reader = new BufferedReader(
                        new InputStreamReader(content));
                String line;
                while ((line = reader.readLine()) != null) {
                    builder.append(line);
                    Log.d(LOG, "line=" + line);
                   // System.out.println("line=" + line);
                   // text5.append("line="+line+"\n");
                }
            } else {
                builder.append("{\"error\": \"HTTP Status "+statusCode+"\",");
                builder.append("\"detail\" : \""+url+"\"}");
            }

        } catch (ClientProtocolException cpe) {
             Log.i(LOG, "ClientProtocolException - Data is Unreacbale");
             cpe.printStackTrace();
             builder.append("{\"error\": \"Unreachable\",");
             builder.append("\"detail\" : \""+url+"\"}");
             Log.d(LOG, "ClientProtocolException="+builder.toString());

        } catch (IOException ioe) {
            Log.i(LOG, "IOException - IO Problem");
            ioe.printStackTrace();
            builder.append("{\"error\": \"IO Problem\",");
            builder.append("\"detail\" : \""+url+"\"}");
            Log.d(LOG, "IOException="+builder.toString());

         }catch (Exception e) {
             Log.i(LOG, "Exception - Unexpected");
             e.printStackTrace();
         }

        return builder.toString();
    }
    public String getUrl() {
        return url;
    }
    public void setUrl(String url) {
        this.url = url;
    }
    public String getStatus() {
        return status;
    }
    public void setStatus(String status) {
        this.status = status;
    }

}
