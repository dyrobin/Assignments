package com.example.test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.support.v7.app.ActionBarActivity;
import android.content.Context;
import android.content.ContextWrapper;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;


public class MainActivity extends ActionBarActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final TextView txt = (TextView) findViewById(R.id.textview);
        final Button button = (Button) findViewById(R.id.scan);
        final Context mContext = this;
        final int times = 15;
        final int interval = 5;
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	new Thread(new Runnable() { 
					public void run(){
						startScan(mContext, times, interval);
	                    }
	            }).start();
            }
        });
    }

    // scan wifi results write to /sdcard/tmp and vibrate after finished
    public void startScan(Context mContext, int times, int interval){
        String path = "/sdcard/tmp";
        Log.i("path:", path);//data/data/com.example.test/files
        WifiManager manager;
        manager = (WifiManager) getSystemService(WIFI_SERVICE);
    	String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(Calendar.getInstance().getTime()); 
    	File file_json = new File(path + "/result_"+timeStamp+".json");
    	File file_csv = new File(path + "/result_"+timeStamp+".csv");
    	String APinfoString="";
    	int i=1;
    	JSONObject multi_location = new JSONObject();
        try {
            while (true) {
            	List<ScanResult> results = manager.getScanResults();
            JSONObject available = new JSONObject();
              try {
            	  int j=1;
      			for (ScanResult scanResult : results) {
      				
      				JSONObject accessPoint = new JSONObject();
      				accessPoint.put("BSSID", scanResult.BSSID);
      				accessPoint.put("SSID", scanResult.SSID);
      				accessPoint.put("frequency", scanResult.frequency);
      				accessPoint.put("level", scanResult.level);
      				accessPoint.put("capabilities", scanResult.capabilities);
//      				available.put(accessPoint);
      				available.put("ap"+j, accessPoint);
      				APinfoString+="BSSID,"+scanResult.BSSID+",SSID,"+scanResult.SSID+",level,"+Integer.toString(scanResult.level)+"\n";
      				j++;
      			}
          		} catch (JSONException e) {
          			e.printStackTrace();
          		}
//              multi_location.put("id",);
              	try {
              		manager.startScan();
              		multi_location.put("measurement "+i, available);
              		Log.i("wifi available:", i+"th data set");
              		Log.i("wifi available:", available.toString());
              		Log.i("wifi multi:", multi_location.toString());
				} catch (JSONException e2) {
					// TODO Auto-generated catch block
					e2.printStackTrace();
				}
                Thread.sleep(interval * 1000);
                i=i+1;
                if (i>times){
                	try {
            			FileOutputStream stream_json = new FileOutputStream(file_json);
            			stream_json.write(multi_location.toString().getBytes());
            			FileOutputStream stream_csv = new FileOutputStream(file_csv);
            			stream_csv.write(APinfoString.getBytes());
            			
            		} catch (FileNotFoundException e1) {
            			// TODO Auto-generated catch block
            			e1.printStackTrace();
            		} catch (IOException e) {
            			// TODO Auto-generated catch block
            			e.printStackTrace();
            		}
                	vibrate(mContext, 1);
                	break;
                }
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    
    public void vibrate(Context mContext, int seconds){
    	Vibrator v = (Vibrator) mContext.getSystemService(Context.VIBRATOR_SERVICE);
   	 // Vibrate for 500 milliseconds
    	v.vibrate(seconds*1000);
    }
    
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
