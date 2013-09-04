package nu.flacco.android.bta;

import java.util.Map;

import android.app.Activity;
import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.PopupWindow;
import android.widget.RemoteViews;
import android.widget.TextView;
import android.widget.Toast;
import nu.flacco.android.bta.R;

public class MyPreferencesActivity extends PreferenceActivity {
    int mAppWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID;
    public static final String PREFS_PREFIX = "AAPTBTAprefs";
    String prefsfile;
    Intent resultValue;

    EditText mAppWidgetPrefix;
    private static final String LOG = "MyPreferencesActivity";
    SharedPreferences preferences;
    int warned=0;   // Warning on blank preferences.

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        resultValue = new Intent();
        resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
        resultValue.putExtra(AppWidgetManager.ACTION_APPWIDGET_DELETED, mAppWidgetId);
        resultValue.setComponent(new ComponentName("nu.flacco.android.bta","nu.flacco.android.bta.AaptBtaAppWidgetProvider"));
        setResult(RESULT_CANCELED, resultValue);
       // Log.d(LOG, "Interupted Delete widget:" + mAppWidgetId);
       // setResult(RESULT_CANCELED, resultValue);    // on abort

        Log.d(LOG, "MyPreferencesActivity - onCreate()");
        Log.d(LOG, "CP 01");
       // setContentView(R.xml.preferences);

        addPreferencesFromResource(R.xml.preferences);
        setContentView(R.layout.prefslayout);

       // TextView tv= this.
      //  final Button btnSaveSite = (Button) findViewById(R.id.savebutton);
      //  btnSaveSite.setOnClickListener(onClick);

        preferences = PreferenceManager.getDefaultSharedPreferences(this);

        Log.d(LOG, "CP 010");
        // 1. Get AppWidget from Intent that launched this
        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        if (extras != null) {
            mAppWidgetId = extras.getInt(
                    AppWidgetManager.EXTRA_APPWIDGET_ID,
                    AppWidgetManager.INVALID_APPWIDGET_ID);
        }
        prefsfile = String.format("%s%d", PREFS_PREFIX, mAppWidgetId);
        Log.d(LOG, "prefs=" + prefsfile);

        //2. App Widget config here


        if (mAppWidgetId == AppWidgetManager.INVALID_APPWIDGET_ID) {
           Log.i(LOG, "CP 03");
           finish();
        } else
            Log.d(LOG, "mAppWidgetId:" +  mAppWidgetId);

        Log.d(LOG, "onCreate() - end");
    }


    View.OnClickListener mOnClickListener = new View.OnClickListener() {
        public void onClick(View v) {
            Log.d(LOG, "mOnClickListener()");
        }
    };

    public void onClickHandler(View v) {
        switch (v.getId()) {
            case R.id.savebutton: {
                Log.d(LOG, "SaveButton");
                if (preferences.contains("username") && (preferences.getString("username", null).length()>0) &&
                        preferences.contains("password") && (preferences.getString("password", null).length()>0) &&
                        preferences.contains("dataorder") && (preferences.getString("dataorder", null).length()>0)) {

                    String key = new Integer(mAppWidgetId).toString();
                    String value=preferences.getString("username",null)+":::" +
                                 preferences.getString("password",null)+":::" +
                                 preferences.getString("dataorder",null);
                   // SharedPreferences private_preferences = this.getSharedPreferences("userdetails", MODE_PRIVATE);
                    Editor edit = preferences.edit();
                    edit.putString(key,value);
                    edit.commit();

                    final Context context = MyPreferencesActivity.this;
                    AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
                    // 4. Update Main widget
                    RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.widget_layout);
                    appWidgetManager.updateAppWidget(mAppWidgetId, views);

                    Intent resultValue = new Intent();
                    resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
                    resultValue.putExtra(AppWidgetManager.ACTION_APPWIDGET_UPDATE, mAppWidgetId);
                   // resultValue.putExtra(AppWidgetManager.ACTION_APPWIDGET_ENABLED, mAppWidgetId);

                    resultValue.setComponent(new ComponentName("nu.flacco.android.bta","nu.flacco.android.bta.AaptBtaAppWidgetProvider"));

                    setResult(RESULT_OK, resultValue);
                    Log.d(LOG, "saving mAppWidgetId="+mAppWidgetId);


                    updateAllWidgets();
                    Log.d(LOG, "finish");
                    finish();

                } else {
                         Log.d(LOG, "Not Ready");
                         Toast.makeText(this, "All Fields are required. Press Back to abort widget add.", Toast.LENGTH_LONG).show();
                    }
                }

                }
            return;
     }



    private void updateAllWidgets(){
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(getApplicationContext());
        int[] appWidgetIds = appWidgetManager.getAppWidgetIds(new ComponentName(this, AaptBtaAppWidgetProvider.class));
        if (appWidgetIds.length > 0) {
            Log.d(LOG, "updateAllWidgets()");
            new AaptBtaAppWidgetProvider().onUpdate(this, appWidgetManager, appWidgetIds);
        }
    }

    public void onDestroy() {
        super.onDestroy();
        Log.d(LOG, "onDestroy()");
            if (!isFinishing()) {
                Log.d(LOG, "onDestroy() - Not finishing");

                Intent ret = new Intent();
                ret.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
                setResult(Activity.RESULT_CANCELED,ret);
            }
    }

    static public void deleteTitlePref(Context context, int appWidgetId) {
        Log.d(LOG, "MyPreferencesActivity - deleteTitlePref() wid=" + appWidgetId);
        //final Context context = MyPreferencesActivity.this;
        SharedPreferences settings =  PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = settings.edit();
        String key = new Integer(appWidgetId).toString();
        editor.remove(key);
        editor.commit();
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        Log.d(LOG, "MyPreferencesActivity - onRestoreInstanceState()");
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        Log.d(LOG, "MyPreferencesActivity - onSaveInstanceState()");
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
            //menu.add(Menu.NONE, 0, 0, "Show current settings");
            Log.d(LOG, "MyPreferencesActivity - onCreateOptionsMenu()");
            return super.onCreateOptionsMenu(menu);
    }

    public boolean onOptionsItemSelected(MenuItem item) {
          switch (item.getItemId()) {
              case 0:
                  //startActivity(new Intent(this, ShowSettingsActivity.class));
                 Log.d(LOG, "MyPreferencesActivity - onOptionsItemSelected()");
                 return true;
         }
         return false;
    }
}