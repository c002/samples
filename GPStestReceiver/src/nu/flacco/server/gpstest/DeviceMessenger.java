package nu.flacco.server.gpstest;

import java.io.IOException;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import nu.flacco.server.persistence.Devices;
import nu.flacco.server.persistence.Persistence;

import com.google.android.gcm.server.Message;
import com.google.android.gcm.server.MulticastResult;
import com.google.android.gcm.server.Result;
import com.google.android.gcm.server.Sender;

public class DeviceMessenger {
	
	private final org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(getClass().getName());
	//String myApiKey="AIzaSyDFaJpQG-iXHBcSD5xgRBNNOEU0UmS8RFM"; //api test2
	String myApiKey="AIzaSyDJgf-lQ6x0rx72lONwz0HckC4dwnaZ4Os";
	Persistence persist=null;
	List <String> devices=null;
	
	
	public DeviceMessenger(Persistence persist, String myApiKey) 
	{
		this.persist=persist;
		this.myApiKey=myApiKey;
		
	}
	public DeviceMessenger(Persistence persist) 
	{
		this.persist=persist;

	}
	public boolean sendMessageToAllDevices(String msg)
	  {
		if (devices==null)
			devices =  getAllDevices();
					
		if (devices==null || devices.size()==0) return false;
		
		try {
	          logger.debug("sendMessageToAllDevices() 0: " + devices.get(0) +"\napikey="+myApiKey);
	          Sender sender = new Sender(myApiKey);
	          logger.debug("sendMessageToAllDevices(): 1a");
	          //Message message = new Message.Builder().build();
	          
	         	DateFormat df = new SimpleDateFormat("ddMMMyy kk:mm:ss");
	        	String sdate = df.format(new Date());

	        	
	          Message message = new Message.Builder()
	          		.addData("msg", String.format("%s [d=%s]", msg,sdate))
	          		.build();
	          		
              
	          logger.debug("sendMessageToAllDevices(): 2"); 
	          Result result = sender.send(message, devices.get(0), 5);
	          logger.debug("sendMessageToAllDevices() results: id=" + result.getMessageId()+",err="+ result.getErrorCodeName()); 
	         // List <String> mres = result.getResults();
//	          logger.debug("sendMessageToAllDevices() results: suc=" + 
//	        		  result.getSuccess() + ", fail=" + result.getFailure() + 
//	        		  ", snt=" + result.getTotal() );
	          return(true);
		} catch (IOException ioe) {
	          logger.debug("error sending message to Device(s)");
	          ioe.printStackTrace();
	          return(false);
	      }
		
	  }
	
	public List<String> getAllDevices()
	{
	    ArrayList <String> gcmIdlist = new ArrayList<String>();
	    List <Devices> dlist;
	    dlist=persist.loadDevices();
		for (Devices dev : dlist) {
			gcmIdlist.add(dev.getGcmid());
		}
		
	    return(gcmIdlist);
			
		
	}
	
	// Find specific device
	 public List<String> findDevice() 
	 {
		 return(null);
	 }
	 
	 // Return devices that match criteria
	 public List<String> findMatchingDevices()
	 {
		 return(null);
	 }
}
