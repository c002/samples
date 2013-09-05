package nu.flacco.server.tests;

import static org.junit.Assert.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;
import nu.flacco.server.gpstest.DeviceMessenger;
import nu.flacco.server.persistence.Devices;
import nu.flacco.server.persistence.Locations;
import nu.flacco.server.persistence.Persistence;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;


public class testSendMessage {
	private final org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(getClass().getName());

	static Persistence persist;
	 private String myApiKey="AIzaSyDJgf-lQ6x0rx72lONwz0HckC4dwnaZ4Os";  // api test1
	 //  private String myApiKey="AIzaSyDFaJpQG-iXHBcSD5xgRBNNOEU0UmS8RFM";   // api test 2 proj=899559802508
	   	
	@BeforeClass
    public static void setUp() {
        System.out.println("@Before");
        persist = new Persistence();
    }
	
	@Test
	public void test01()
	{
		System.out.println("test01");
		logger.debug("test01");
		DeviceMessenger dm = new DeviceMessenger(persist, myApiKey);
		boolean status = dm.sendMessageToAllDevices("soup is ready");
		assertThat(status, is(true));
	}
	
	
	
}