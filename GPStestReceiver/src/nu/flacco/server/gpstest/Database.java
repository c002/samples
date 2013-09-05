package nu.flacco.server.gpstest;

import java.io.*;
import java.util.*;
import java.util.Date;
import java.sql.*;

import nu.flacco.server.persistence.Devices;
import nu.flacco.server.persistence.Locations;
import nu.flacco.server.persistence.Persistence;
import nu.flacco.server.wire.AppError;
import nu.flacco.server.wire.LocationData;

import com.jolbox.bonecp.BoneCP;
import com.jolbox.bonecp.BoneCPConfig;

public class Database {

   
    public static Database theInstance=null;
    
  //  BoneCPConfig bconfig=null;
  //  BoneCP connectionPool=null;

    static Persistence persist;
    
    private AppError error=null;
    
   // private String lastError=null;
    
    String dbstr="tracking";
    private final org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(getClass().getName());

    public Database(String dbstr)
    {
    	logger.debug("Database()");
    	this.dbstr=dbstr;
    	persist = new Persistence();	
    }
/*
    private Devices LocationData2Devices(LocationData locdata)
    {
    	Devices device = new Devices();
    	
        if (locdata.getGCMId()!=null) device.setGcmid(locdata.getGCMId());
        if (locdata.getLinenumber()!=null) device.setLinenumber(locdata.getLinenumber());
        if (locdata.getSubscriberid()!=null) device.setSubscriberid(locdata.getSubscriberid());
        if (locdata.getCountryIso()!=null) device.setCountryIso(locdata.getCountryIso());
        if (locdata.getOpname()!=null) device.setOpname(locdata.getOpname());
        if (locdata.getDeviceId()!=null) device.setDeviceid(locdata.getDeviceId());
        device.setActive(true);

        return device;
    }
    private Locations LocationData2Locations(LocationData locdata)
    {
    	 logger.debug("LocationData2Locations(): " + locdata);
    	Locations loc = new Locations();
    	
       if (locdata.getDeviceId()!=null) loc.setDeviceid(locdata.getDeviceId());
       if (locdata.getAccuracy()>0) loc.setAccuracy(locdata.getAccuracy());
       if (locdata.getCoord()!=null) loc.setLongitude(locdata.getCoord().getDoubleLon());
       if (locdata.getCoord()!=null) loc.setLattitude(locdata.getCoord().getDoubleLat());
       if (locdata.getProvider()!=null) loc.setProvider(locdata.getProvider());
       if (locdata.getTimestamp()>0) loc.setTimestamp(locdata.getTimestamp());
       
       return(loc);
    }
 */
    /*       
    public boolean updateTrackindata(LocationData locdata) throws Exception
    {
    	 logger.debug("updateTrackindata(): " + locdata);
    	 Devices device = LocationData2Devices(locdata);
    	 Locations location = LocationData2Locations(locdata);
    
         if (locdata.getDeviceId()!=null) {
            
             Devices aDevice = persist.getDevice(locdata.getDeviceId());
             if (aDevice==null) {
             	logger.debug("updateTrackindata() add");
                 persist.addDevice(device);
             } else {
             	logger.debug("updateTrackindata() update");
                 persist.UpdateDevice(aDevice.getDeviceid(), device);
             }
             logger.debug("addLocation(): " + location);
             
             if (location.getTimestamp()>0)
            	 persist.addLocation(location);
         }
  
    	 return true;
    }
    
*/
       public void finalize() 
        {
    	  /*
                if (connectionPool!=null) {
                    connectionPool.shutdown();
                    connectionPool=null;
                }
                */
         }
 
    public AppError getError() {
        return error;
    }

    public void setError(AppError error) {
        this.error = error;
    }

}
