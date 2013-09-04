package au.com.aapt;

import net.sourceforge.jradiusclient.RadiusClient;

public class Authenticate {

    AppProps appprops=null;
    int radiusAuthPort = 0;
    int radiusAcctPort = 0;
    String radiusSecret   = null;
    String radiusServer   = null;

    public Authenticate() {
        try {
            appprops = AppProps.getInstance();
            appprops.loadProperties("application.properties");
        } catch (Exception e) {
            e.printStackTrace();
        }
 
        radiusAuthPort = appprops.getRadius1AuthPort();
        radiusAcctPort = appprops.getRadius1AcctPort();
        radiusSecret   = appprops.getRadius1Secret();
        radiusServer   = appprops.getRadius1Server();

    }
 
    public boolean login(String user, String passwd)
    {
        int authenticated=0;
        try {
        	if (user.indexOf("@")==-1) {
        		user=user+"@customer.connect.com.au";
        	}
            RadiusClient rc = new RadiusClient(radiusServer, radiusAuthPort, radiusAcctPort, radiusSecret, user, true);
            authenticated = rc.authenticate(passwd);
            
            if (authenticated == RadiusClient.ACCESS_ACCEPT) {
                System.out.println("Login successful for : " + user);
                return(true);
            } 
            
        } catch (java.net.SocketTimeoutException ste) {
            // java.net.SocketTimeoutException: Receive timed out
            //  Create file to mark switch to secondary radius server

            System.out.println("Timeout on primary radius server: " + radiusServer);
           
        } catch (Exception e) {
            e.printStackTrace();
        }
     
        return(false); 
    } 
    
}
