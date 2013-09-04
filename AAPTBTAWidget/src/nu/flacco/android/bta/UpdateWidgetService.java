package nu.flacco.android.bta;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Map;
import java.util.Random;

import nu.flacco.android.bta.CustomerDataOrder;

import org.json.JSONObject;

import android.app.PendingIntent;
import android.app.Service;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.ProgressBar;
import android.widget.RemoteViews;
import nu.flacco.android.bta.R;

    public class UpdateWidgetService extends Service {
        private static final String LOG = "UpdateWidgetService";
        SharedPreferences preferences;

        String username = null;
        String password = null;
        String dataorder = null;

        @Override
        public void onStart(Intent intent, int startId) {
            Log.i(LOG, "OnStart()");

            // don't try update if network unavailable,
            if (isNetworkAvailable()==false ) {
                Log.d(LOG, "Network Unavailable()");
                stopSelf();
                 return;
            }

            AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(this
                    .getApplicationContext());

            int[] allWidgetIds = intent
                    .getIntArrayExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS);

            preferences = PreferenceManager.getDefaultSharedPreferences(this);


            for (int widgetId : allWidgetIds) {
                RemoteViews remoteViews = new RemoteViews(this
                        .getApplicationContext().getPackageName(),
                R.layout.widget_layout);
                String key = new Integer(widgetId).toString();

                if (! preferences.contains(key)) {
                    continue;   // zombie, no config data
                }

                String value = preferences.getString(key, null);
//                Log.d(LOG, "key="+key+ ",value: " + value);

                String data[] = value.split(":::");
                username=data[0];
                password=data[1];
                dataorder=data[2];
                Log.d(LOG, "Fetching for: Widget="+ key +",u="+username+",p="+password+",d="+dataorder);

                // For each dataorder, get the usage.
                BtaJSONSource bta = new BtaJSONSource();
                String btajsonstr =null;
                btajsonstr = bta.getBTAData(username, password, dataorder);

                // Store result in prefs for later use.
                Editor edit = preferences.edit();
                String datakey="data"+key;
                edit.putString(datakey,btajsonstr);
                edit.putString("lastupdate",nowDate());
                edit.putString("url",bta.getUrl());
                edit.putString("status",bta.getStatus());
                edit.commit();

                String error=null;
                String errorDetail=null;
                JSONObject jsonObj=null;

                try {
                    Log.i(LOG, "jsonstr=" + btajsonstr);
                    jsonObj = new JSONObject(btajsonstr);
                    if (jsonObj==null)
                        error="Error decoding JsonObj";
                    else if (jsonObj.has("error")) {
                        error = (String) jsonObj.get("error");
                        if (jsonObj.has("detail"))
                            errorDetail =  (String) jsonObj.get("detail");
                    }
                     // Handle Error from Server End.
                    if (error!=null) {
                        remoteViews.setTextViewText(R.id.textView1, error);
                    } else {

                        CustomerDataOrder cdo = new CustomerDataOrder(jsonObj.getJSONObject("dataorder"));
                        String dataorderstr=null;


                        if (cdo.getDataOrderName()!=null && cdo.getDataOrderName().length()>22)
                            dataorderstr = cdo.getDataOrderName().substring(0, 22)+ "...";
                        else if (cdo.getDataOrderName()==null || cdo.getDataOrderName().length()==0)
                            dataorderstr = cdo.getDataOrder() + " - " + "Data";
                        else
                            dataorderstr = cdo.getDataOrderName();

                        remoteViews.setTextViewText(R.id.textView2,dataorderstr);
                        //Log.i(LOG, "DataOrdrSt=" + dataorderstr);

                        if (cdo.getTrafficIncluded()>0 && cdo.getToUsage()>0) {

                            float percent = ((float) cdo.getToUsage()/ (float)cdo.getTrafficIncluded()) * 100;
                            if (percent>999) percent=999;
                            remoteViews.setTextViewText(R.id.update, String.format("%3.0f%%",percent));
                        } else {
                            remoteViews.setTextViewText(R.id.update, "0%");
                        }

                        remoteViews.setTextViewText(R.id.textView3,nowDate());

                        String trafficstr =  String.format("%4.1f /%4.1f GB", (float)cdo.getToUsage()/1000, (float) cdo.getTrafficIncluded()/1000);
                        remoteViews.setTextViewText(R.id.textView1,trafficstr);

                        remoteViews.setProgressBar(R.id.progressBar1, cdo.getTrafficIncluded(),
                                (int) cdo.getToUsage(), false);

                        remoteViews.setProgressBar(R.id.progressBar2,
                                days_in_month(true),
                                days_in_month(false), false);

                      Intent clickIntent = new Intent(this.getApplicationContext(),
                              DetailsActivity.class);

                      clickIntent.setData(ContentUris.withAppendedId(Uri.EMPTY, widgetId));


                   // clickIntent.setAction(AppWidgetManager.ACTION_APPWIDGET_UPDATE);
                    clickIntent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS,
                            allWidgetIds);
                    Log.d(LOG, "Make Clickable widgetId=" + widgetId);
                    clickIntent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, widgetId);
                    clickIntent.putExtra("mydata", widgetId);

  //                  PendingIntent pendingIntent = PendingIntent.getBroadcast(
  //                          getApplicationContext(), 0, clickIntent,
                   //         PendingIntent.FLAG_UPDATE_CURRENT);
                    PendingIntent pendingIntent = PendingIntent.getActivity( getApplicationContext(), 0, clickIntent, 0);

                   remoteViews.setOnClickPendingIntent(R.id.btawidget, pendingIntent);

                    appWidgetManager.updateAppWidget(widgetId, remoteViews);

                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }

                appWidgetManager.updateAppWidget(widgetId, remoteViews);    // update also when exception
            }

            Log.d(LOG, "StopSelf()");
            stopSelf();
           // super.onStart(intent, startId);
        }

        private boolean isNetworkAvailable() {

            ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo ni = cm.getActiveNetworkInfo();

            return ni!=null;
        }

        @Override
        public IBinder onBind(Intent intent) {
            return null;
        }

        private int days_in_month(boolean max)
        {
            Calendar cal = new GregorianCalendar();

            Date nowtime = new Date();
            cal.setTime(nowtime);
            int nowday=cal.get(cal.DAY_OF_MONTH);
            int days = cal.getActualMaximum(Calendar.DAY_OF_MONTH);

            if (max==true)
                return(days);
            else
                return(nowday);
        }

        private String nowDate()
        {
            Date nowtime = new Date();
            DateFormat df = new SimpleDateFormat("HH:mm dd-MMM-yyyy");
            return df.format(nowtime);
        }
}
