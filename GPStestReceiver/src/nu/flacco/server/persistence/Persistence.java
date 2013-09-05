package nu.flacco.server.persistence;

import java.sql.Timestamp;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

import nu.flacco.server.wire.AppError;
import nu.flacco.server.wire.LocationData;

import org.hibernate.HibernateException;
import org.hibernate.Query;
import org.hibernate.Session;
import org.hibernate.SessionFactory;

import org.hibernate.cfg.Configuration;
import org.hibernate.service.ServiceRegistry;
import org.hibernate.service.ServiceRegistryBuilder;

public class Persistence {

	private static SessionFactory sessionFactory;
	private static ServiceRegistry serviceRegistry;
    private final org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(getClass().getName());
    
    private AppError error=null;
        
	public Persistence()
	{
	    System.out.println("Working Directory = " + System.getProperty("user.dir"));
	    //Session session = sessions.openSession();
		sessionFactory = configureSessionFactory();
	}

	private static SessionFactory configureSessionFactory() throws HibernateException {
	    System.out.println("configureSessionFactory()");
	    Configuration configuration = new Configuration();
	    
	    
	    //configuration.configure("config/hibernate.cfg.xml");
	    configuration.configure("hibernate.cfg.xml");
	    //configuration.configure();
	    serviceRegistry = new ServiceRegistryBuilder().applySettings(configuration.getProperties()).buildServiceRegistry();        
	    sessionFactory = configuration.buildSessionFactory(serviceRegistry);
	    return sessionFactory;
	}
	/*
	public Session xgetSession() {

	    if (session == null) {
	        session = sessionFactory.openSession();
	    }
	    return session;
	}
*/
    public static SessionFactory getSessionFactory() {
        return sessionFactory;
    }

    public boolean updateTrackindata(LocationData locdata) throws Exception
    {
    	 logger.debug("updateTrackindata(): " + locdata);
    	 Devices device = LocationData2Devices(locdata);
    	 Locations location = LocationData2Locations(locdata);
    
         if (locdata.getDeviceId()!=null) {
            
             Devices aDevice = getDevice(locdata.getDeviceId());
             if (aDevice==null) {
             	logger.debug("updateTrackindata() add");
                 addDevice(device);
             } else {
             	logger.debug("updateTrackindata() update");
                 UpdateDevice(aDevice.getDeviceid(), device);
             }
             logger.debug("addLocation(): " + location);
             
             if (location.getTimestamp()>0)
            	 addLocation(location);
         }
  
    	 return true;
    }
    
    public void addDevice(Devices device) throws Exception
    {
    	logger.debug("addDevice(): " + device.getDeviceid());
        if (device!=null) {
            System.out.println("addDevice()");
            Timestamp ts = new Timestamp(new Date().getTime());
            Session session = getSessionFactory().openSession();

            session.beginTransaction();
            session.save(device);

            session.getTransaction().commit();
            session.close();
        }
    }
    
    public void addLocation(Locations loc) throws Exception
    {
    	System.out.println("addLocation(): " + loc);
    	logger.debug("addLocation(): " + loc.getDeviceid());
        if (loc!=null) {
            System.out.println("addLocation() 2");
            Timestamp ts = new Timestamp(new Date().getTime());
            Session session = getSessionFactory().openSession();

            session.beginTransaction();
            loc.setCreated(new Date());
            session.save(loc);

            session.getTransaction().commit();
            session.close();
        }
    }
    
    public Locations getLastLocation(String iddevice)
    {
        System.out.println("getLastLocation(): " + iddevice);
        logger.debug("getLastLocation(): ");
        //Session session = getSessionFactory().getCurrentSession();
        Session session = getSessionFactory().openSession();
        session.beginTransaction();
        
        Locations loc; 
        //Query q = session.createQuery("from Devices as devices where devices.deviceid=:deviceid and max(timestamp)==timestamp");
        Query q = session.createQuery("from Locations as locations where locations.deviceid=:deviceid");
        //long id = q.
        loc= (Locations) q.setString("deviceid",iddevice).uniqueResult();
        session.getTransaction().commit();
        session.close();
        
        return(loc);
    }

    public void addDevice(String deviceid) throws Exception
    {
        System.out.println("addDevice() str: "+deviceid);
        logger.debug("addDevice(): " + deviceid);
    	Timestamp ts = new Timestamp(new Date().getTime());
    	Session session = getSessionFactory().openSession();
    	
    //    try {	
    	    session.beginTransaction();

    	    Devices aDevice = new Devices();
    	    aDevice.setDeviceid(deviceid);
    	    aDevice.setActive(true);
            aDevice.setUpdated(ts);
            session.save(aDevice);

            session.getTransaction().commit();
    //    } catch( HibernateException e ) {
    //        session.getTransaction().rollback();
    //        throw new Exception("Could not add entry ",e);
    //    } finally {
            session.close();
    //    }
        
       // getSessionFactory().close();
    }
    
    public void deleteDevice(String deviceid) throws Exception
    {
        System.out.println("deleteDevice()");
        logger.debug("deleteDevice(): " + deviceid);
        //Session session = getSessionFactory().getCurrentSession();
        Session session = getSessionFactory().openSession();
        
        try {
            session.beginTransaction();
            Query q = session.createQuery("from Devices as devices where devices.deviceid=:deviceid");
            //long id = q.
            List list = q.setString("deviceid",deviceid).list();
            
            Iterator i = list.iterator();
            while(i.hasNext()) {
            Devices dev = (Devices)i.next();
                System.out.println("Device="+dev);
                session.delete(dev);
            }
            
            session.getTransaction().commit();
        } catch( HibernateException e ) {
            session.getTransaction().rollback();
                throw new Exception("Could not delete entry ",e);
        } finally {
            session.close();
        }
    }
    
    public int rowsDevices(String deviceid)
    {
    
        System.out.println("rowsDevices()");
        logger.debug("rowsDevices(): " + deviceid);
        Session session = getSessionFactory().openSession();
        session.beginTransaction();
        
        Query q = session.createQuery("from Devices as devices where devices.deviceid=:deviceid");
        //long id = q.
        List list = q.setString("deviceid",deviceid).list();
        
        session.close();
        
        return list.size();
    }
    
    public boolean dumpDevices()
    {
    	List dlist = loadDevices();
    	if (dlist==null)
    		return(false);
    	 Iterator i = dlist.iterator();
         while(i.hasNext()) {
         Devices dev = (Devices)i.next();
             System.out.println("Device="+dev);
         }
    	return true;
    }
    
    public List loadDevices()
    {
        System.out.println("loadDevices()");
        logger.debug("loadDevices(): ");
        Session session = getSessionFactory().openSession();
        session.beginTransaction();
        List list=null;
        
        try {
            list = session.createQuery("from Devices").list();
            session.getTransaction().commit();
        } catch ( HibernateException e ) {
             e.printStackTrace();
        } finally {
            session.close();
        }
        
        return(list);
    }
    
    public Devices getDevice(String iddevice)
    {
        System.out.println("getDevice(): " + iddevice);
        logger.debug("getDevice(): ");
        //Session session = getSessionFactory().getCurrentSession();
        Session session = getSessionFactory().openSession();
        session.beginTransaction();
        
        Devices aDevice; // = new Devices();
        Query q = session.createQuery("from Devices as devices where devices.deviceid=:deviceid");
        //long id = q.
        aDevice= (Devices) q.setString("deviceid",iddevice).uniqueResult();
        session.getTransaction().commit();
        session.close();
        
        return(aDevice);
    }
    
    public Devices UpdateDevice(String deviceid, Devices aDevice)
    {
        System.out.println("UpdateDevice(): " + deviceid);
    	logger.debug("UpdateDevice(): " + deviceid);
    	//Session session = getSessionFactory().getCurrentSession();
    	Session session = getSessionFactory().openSession();
        session.beginTransaction();
    	

    	Devices tDevice; // = new Devices();
    	Query q = session.createQuery("from Devices as devices where devices.deviceid=:deviceid");

    	//long id = q.
    	tDevice= (Devices) q.setString("deviceid",deviceid).uniqueResult();
    	logger.debug("UpdateDevice() tDevice= " + tDevice);
    	//System.out.println("1="+aDevice);
    		
    	//aDevice.setActive(false);
    	////aDevice.setFlags(16);
    	//aDevice.setGcmid("googleid02");
    	
    	if (tDevice!=null) {
    	    tDevice.copyNotNull(aDevice);
    	    tDevice.setUpdated(new Date());
    	    session.update(tDevice);
    	    session.getTransaction().commit();
    	    System.out.println("2="+tDevice);
    	} else 
    		logger.debug("UpdateDevice() tDevice==null: " + deviceid);
    	
    	session.close();

    	return(aDevice);
    }
 
    
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
       if (locdata.getCoord()!=null) loc.setLongitude(locdata.getCoord().getLon());
       if (locdata.getCoord()!=null) loc.setLattitude(locdata.getCoord().getLat());
       if (locdata.getProvider()!=null) loc.setProvider(locdata.getProvider());
       if (locdata.getTimestamp()>0) loc.setTimestamp(locdata.getTimestamp());
       
       return(loc);
    }
    public AppError getError() {
        return error;
    }

    public void setError(AppError error) {
        this.error = error;
    }

}
