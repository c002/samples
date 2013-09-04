package nu.flacco.android.bta;

import java.util.ArrayList;
import java.util.Map;
import java.util.Random;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.appwidget.AppWidgetProviderInfo;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.RemoteViews;
import android.widget.Toast;

public class AaptBtaAppWidgetProvider extends AppWidgetProvider   {
//public class AaptBtaAppWidgetProvider extends BroadcastReceiver   {

    private static final String LOG = "AaptBtaAppWidgetProvider";
    private static final String ACTION_CLICK = "ACTION_CLICK";

    String username = null;
    String password = null;
    String dataorder = null;

    ArrayList <Integer> zombies=new ArrayList<Integer>();

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager,
            int[] appWidgetIds)
    {
        super.onUpdate(context,appWidgetManager, appWidgetIds);
        // Setup but NOT if config Activity

        // updatePeriodMillis
        Log.i(LOG, "AaptBtaAppWidgetProvider - onUpdate()");

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);

        Log.i(LOG, "onUpdate() appWidgetIds.len:" + appWidgetIds.length);
        final int N = appWidgetIds.length;
        for (int i=0; i<N; i++) {
                int appWidgetId = appWidgetIds[i];
                Log.d(LOG, "widget: " + appWidgetId);
                String key = new Integer(appWidgetId).toString();
                if (! preferences.contains(key)) {
                    zombies.add(appWidgetId);
                }
                String value = preferences.getString(key, null);
                Log.d(LOG, "value: " + value);

        }
        // remove any zombies created during incomplete preferences.
        if (zombies.size()>0) {
            int [] zombiesArray = new int[zombies.size()];
            for (int i=0; i<zombies.size() ; i++) {
                zombiesArray[i]=zombies.get(i).intValue();
            }
            Log.d(LOG, "remove zombies: " + zombies.size());
            onDeleted(context, zombiesArray);
            zombies.clear();
        }

/*
     // make this thing clickable!
        Intent intWidgetClicked = new Intent(context,DetailsActivity.class);

        intWidgetClicked.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId);
        PendingIntent pi = PendingIntent.getActivity(context, 0, intWidgetClicked, 0);
        RemoteViews views = new
            RemoteViews(context.getPackageName(),R.layout.widget_layout);
        Log.d(LOG, "widget 2: " + appWidgetId);
        views.setOnClickPendingIntent(R.id.btawidget, pi);
      //  appWidgetManager.updateAppWidget(appWidgetId,views);
*/

        // Build the intent to call the service
        Intent intent = new Intent(context.getApplicationContext(),
                UpdateWidgetService.class);
        //intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS, allWidgetIds);
        intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS, appWidgetIds);

        Log.d(LOG, "startService()");
        context.startService(intent);
     }
/*
    @Override
    public void onReceive(Context context, Intent intent) {
       // String username = null;


        Log.d(LOG, "AaptBtaAppWidgetProvider - onReceive(): " +  intent.getAction());
        Bundle extras = intent.getExtras();
        if (extras != null) {
            username = extras.getString("username");
            password = extras.getString("password");
            dataorder = extras.getString("dataorder");
            Log.d(LOG, "onReceive() user=" + username);
            Log.d(LOG, "onReceive() password=" + password);
            Log.d(LOG, "onReceive() dataorder=" + dataorder);
            context.startService(intent);

        }
        // SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        Bundle extras = intent.getExtras();
        if (extras != null) {
            username = extras.getString("username");
            Log.d(LOG, "AaptBtaAppWidgetProvider - onReceive() user=" + username);
        }

  //      SharedPreferences preferences = PreferenceManager.getSharedPreferences("",0);
        if (username!=null)
            context.startService(intent);
            //context.bindService(intent);
        else
            Log.i(LOG, "AaptBtaAppWidgetProvider - onReceive() NOT READY");

        super.onReceive(context,intent);

    }
*/

    // Delete service when all instances of widget have been removed.
    public void onDisabled(Context context) {
        context.stopService(new Intent(context,UpdateWidgetService.class));
        super.onDisabled(context);
        Log.d(LOG, "onDisabled()");

    }
    public void onEnabled(Context context) {
        context.stopService(new Intent(context,UpdateWidgetService.class));
        Log.d(LOG, "onEnabled()");
        super.onDisabled(context);

   }
    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(LOG, "onReceive() - " +  intent.getAction());
        super.onReceive(context,intent);
    }

 //   @Override
 //   public void onResume(Context context, Intent intent) {
 //       super.onResume(context,intent);
 //       Log.d(LOG, "AaptBtaAppWidgetProvider - onResume()");
 //   }


    @Override
    public void onDeleted(Context context, int[] appWidgetIds) {
        Log.d(LOG, "onDeleted()");
        // When the user deletes the widget, delete the preference associated with it.
        final int N = appWidgetIds.length;
       for (int i=0; i<N; i++) {
           Log.d(LOG, "del widget: " + appWidgetIds[i] + ", N="+N);
           MyPreferencesActivity.deleteTitlePref(context, appWidgetIds[i]);
        }

       // Delete Prefs
       //SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);

    }


  //  @Override
  //  public PreferenceManager.OnActivityStopListener()
  //  {

  //  }

    static void deleteTitlePref(Context context, int appWidgetId) {
    }


/*
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.mainmenu, menu);
        Toast.makeText(this, "onCreateOptionsMenu()",
                Toast.LENGTH_LONG).show();
        return true;
    }

    // This method is called once the menu is selected
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        // We have only one menu option
        case R.id.preferences:
            // Launch Preference activity
            Intent i = new Intent(AAPTUsageMonitorActivity.this, MyPreferencesActivity.class);
            startActivity(i);
            // Some feedback to the user
            Toast.makeText(AAPTUsageMonitorActivity.this, "Enter your user credentials.",
                Toast.LENGTH_LONG).show();
            break;

        }
        return true;
    }
   */

   // @Override
 //   public void onDeleted(Context context, int[] iarray)
  //  {
   //     super.onDeleted(context, iarray);
        // when deleted
  //  }

}
