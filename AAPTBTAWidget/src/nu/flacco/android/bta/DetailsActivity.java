package nu.flacco.android.bta;

import org.json.JSONObject;

import android.app.Activity;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.RemoteViews;
import android.widget.TextView;
import nu.flacco.android.bta.R;
import nu.flacco.android.bta.CustomerDataOrder;

public class DetailsActivity extends Activity {
    private static final String LOG = "DetailsActivity";
    SharedPreferences preferences;
    String url=null;
    String status=null;
    String lastupdate=null;
    String key;
    String value=null;
    int mAppWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(LOG, "start inv widget=" + mAppWidgetId);
        setContentView(R.layout.detailslayout);
        CustomerDataOrder cdo=null;
        JSONObject jsonObj=null;

        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(this
                .getApplicationContext());
        ComponentName thisWidget = new ComponentName(getApplicationContext(),
                AaptBtaAppWidgetProvider.class);
//        int[] allWidgetIds = appWidgetManager.getAppWidgetIds(thisWidget);

//       Context context = getApplicationContext();
        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        if (extras != null) {
              mAppWidgetId = extras.getInt(
                     AppWidgetManager.EXTRA_APPWIDGET_ID,
                     0);

            Log.d(LOG, "Extras mAppWidgetId:" +mAppWidgetId);
//            Log.d(LOG, "Extras mydata:" +mydata);
        }
/*
       final int N = allWidgetIds.length;
       for (int i=0; i<N; i++) {
           Log.d(LOG, "widgetN: " + allWidgetIds[i] + ", N="+N);
        }
*/

        preferences = PreferenceManager.getDefaultSharedPreferences(this);

        if (savedInstanceState==null) {
            Log.d(LOG, "null");
        }


        Log.d(LOG, "widgetid:" +mAppWidgetId);

        String key = new Integer(mAppWidgetId).toString();
        String datakey = "data"+(new Integer(mAppWidgetId).toString());
        String value = preferences.getString(key, null);
//      Log.d(LOG, "key="+key+ ",value: " + value);

         String data[] = value.split(":::");
         String username=data[0];
         String password=data[1];
         String dataorder=data[2];

         if (preferences.contains(datakey)) {
             String userdata = preferences.getString(datakey, null);
             Log.d(LOG, "userdata:" +userdata);
             try {
                 jsonObj = new JSONObject(userdata);
                 cdo = new CustomerDataOrder(jsonObj.getJSONObject("dataorder"));
             } catch (Exception e) {
                 e.printStackTrace();
             }
         }
         if (preferences.contains("url"))
             url = preferences.getString("url", null);

         if (preferences.contains("lastupdate"))
              lastupdate = preferences.getString("lastupdate", null);
         if (preferences.contains("status"))
              status = preferences.getString("status", null);

       //  TextView t2 = (TextView) findViewById(R.id.textView2);
       // t2.setText("WidgetID: " + mAppWidgetId);
        View layout = findViewById(R.id.details_layout);

        if (cdo!=null) {
            Log.d(LOG, "custname:" +cdo.getCustomerName());
            TextView t = (TextView) findViewById(R.id.textView1);
            t.setText(cdo.getCustomer() + " - " + cdo.getCustomerName());

            Log.d(LOG, "MB:" +cdo.getToUsage());

            //  TextView t4 = (TextView) findViewById(R.id.textView4);
            //t4.setText("\t"+String.valueOf(cdo.getToUsage()));
            TextView tTrafIncluded = (TextView) findViewById(R.id.textView_trafincluded);
            long trafficincluded = cdo.getTrafficIncluded();
            float tf;
            if (trafficincluded>=1000) {
                tf = trafficincluded / 1000;
                tTrafIncluded.setText("Traffic Included:" + String.format("%4.0f" , tf) +"GBytes");
            } else {
                tf = trafficincluded;
                tTrafIncluded.setText("Traffic Included:" + String.format("%4.0f", tf) +"MBytes");
            }

            TextView tTrafficTo = (TextView) findViewById(R.id.textView_trafficto);
            float trafficto = (float) cdo.getToUsage();
            float trafto;
            if (trafficto>1000) {
                trafto = trafficto / 1000;
                tTrafficTo.setText("Current Usage: " + String.format("%4.1f" , trafto) +" GBytes");
            } else {
                trafto = trafficto;
                tTrafficTo.setText("Current Usage: " + String.format("%4.0f", trafto) +" MBytes");
            }

            TextView tUsername = (TextView) findViewById(R.id.textView_username);
            tUsername.setText("Username: " +username);
            TextView tDataorderid = (TextView) findViewById(R.id.textView_dataorderid);
            tDataorderid.setText("Dataorder id: " + dataorder);
            TextView tDataorderName = (TextView) findViewById(R.id.textView_dataordername);
            tDataorderName.setText("DataOrder Name: " + cdo.getDataOrderName());
            TextView tUrl = (TextView) findViewById(R.id.textView_url);
            tUrl.setText("Data URL: " + url);
            TextView tLastUpdate = (TextView) findViewById(R.id.textView_lastupdate);
            tLastUpdate.setText("Last Update attempt: " + lastupdate);
            TextView tResponse = (TextView) findViewById(R.id.textView_response);
            try {
                tResponse.setText("Last HTTP Response: " + httpResponse(new Integer(status)));
            } catch (Exception e) {
                tResponse.setText("Last HTTP Response: " + status);
            }
            //t6.setText( "\t" + String.valueOf(cdo.getTrafficIncluded()));

        }

        layout.setClickable(true);

        layout.setOnClickListener(handleClick);
        Log.d(LOG, "end");
    }

    private OnClickListener handleClick = new OnClickListener() {
        public void onClick(View v) {
             Log.d(LOG, "OnClickListener");
             finish();
        }
    };

    private String httpResponse(Integer status)
    {
        switch (status) {
            case  200 : return("200 Ok");
            case  401 : return("401 Unauthorized");
            case  403 : return("403 Forbidden");
            case  404 : return("404 Not found");
            default : return String.valueOf(status);
        }
    }
}
