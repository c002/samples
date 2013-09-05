package nu.flacco.server.tests;

import static org.junit.Assert.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;
import nu.flacco.server.persistence.Devices;
import nu.flacco.server.persistence.Locations;
import nu.flacco.server.persistence.Persistence;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;


public class testDatabase {
    
    static Persistence persist;
    
	@BeforeClass
    public static void setUp() {
        System.out.println("@Before");
        persist = new Persistence();
    }
  
    @AfterClass
    public static void tearDown() {
        System.out.println("@After");
    }
	
    @Test
	public void testDatabase00() throws Exception
	{
    	Locations location = persist.getLastLocation("356402040782458");
		
		assertThat(location.getId(), equalTo(4));
		
	}
    @Test
	public void testDatabase01() throws Exception
	{
		System.out.println("testDatabase01");

		persist.deleteDevice("DeviceTest01");
		int rows = persist.rowsDevices("DeviceTest01");
		assertThat(rows, equalTo(0));
		
		persist.deleteDevice("DeviceTest02");
        rows = persist.rowsDevices("DeviceTest02");
        assertThat(rows, equalTo(0));
	}
    
    @Test
    public void testDatabase02() throws Exception
    {
        System.out.println("testDatabase02");
        
        Devices device = new Devices();
        device.setActive(true);
        device.setDeviceid("DeviceTest02");
        device.setFlags(10);
        device.setGcmid("GoogleId");
        device.setLabel("a label");
        device.setLinenumber("035559999");
       
      //  persist.addDevice("DeviceTest01");
        persist.addDevice(device);
        int rows = persist.rowsDevices("DeviceTest02");
        assertThat(rows, equalTo(1));

    }
    
    @Test	
    public void testDatabase03() throws Exception
    {
        System.out.println("testDatabase02");
        //Persistence persist = new Persistence();
        
        Devices aDevice = persist.getDevice("DeviceTest02");
        assertThat(aDevice ,notNullValue() );
        assertThat(aDevice.isActive(), is(true));
        Devices newDevice = new Devices();
        newDevice.setDeviceid("DeviceTest02");
        newDevice.setActive(false);
        newDevice.setFlags(11);
        newDevice.setGcmid("SomeLongGoogleId");
        aDevice = persist.UpdateDevice("DeviceTest02", newDevice);
        
        assertThat(aDevice.isActive(), is(false));
    }
    
    @Test
	public void testDatabase04() throws Exception
	{
    	testDatabase01();
	}
}
