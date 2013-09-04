package au.com.aapt;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Properties;


public class AppProps {

    public static AppProps theInstance=null;
    public Properties props=null;

    private String propertiespath=null;
    private String builddate="NA";

    private String bta_dburl;
    private String bta_dbuser;
    private String bta_dbpwd;

    private String ordersf_dburl;
    private String ordersf_dbuser;
    private String ordersf_dbpwd;

    private String btausage_dburl;
    private String btausage_dbuser;
    private String btausage_dbpwd;

    private String snmp_dburl;
    private String snmp_dbuser;
    private String snmp_dbpwd;

    private String radius_dburl;
    private String radius_dbuser;
    private String radius_dbpwd;

    private String radius1Server;
    private int radius1AuthPort;
    private int radius1AcctPort;
    private String radius1Secret;

    public static AppProps getInstance() {
        if (theInstance==null) {
            theInstance = new AppProps();
        }

        return  theInstance;
    }

    public void loadProperties(String proppath) throws IOException, BTAException {

        setPropertiesPath(getPropertiesFromClassPath(proppath));

        if (this.getPropertiesPath()==null)
            throw new BTAException(BTAException.FATAL, "Unable to find properties file: " + proppath + " in classpath");

        System.out.println("path=" + this.getPropertiesPath());

        setPropertyVars();

       // in.close();
    }

    private String getPropertiesFromClassPath(String propertyFile){
        String propertiesFile = null;
        props = new Properties();

        URL url = this.getClass().getClassLoader().getResource(propertyFile);

        if (null != url) {
            InputStream is = this.getClass().getClassLoader().getResourceAsStream(propertyFile);
            try {
                props.load(is);
                propertiesFile = url.toString();

                is.close();
            } catch (Exception e) {
                //throw new BTAException("Unable to load properties file from classpath: " + propertyFile.toString());
                e.printStackTrace();
            } finally {
                safeCloseStream(is);
            }
        }

        return propertiesFile;
    }

    public void setPropertyVars() throws IOException
    {

        this.setBta_dburl(props.getProperty("bta4.dburl","error"));
        this.setBta_dbuser(props.getProperty("bta4.dbuser","error"));
        this.setBta_dbpwd(props.getProperty("bta4.dbpwd","error"));

        this.setBtausage_dburl(props.getProperty("btausage.dburl","error"));
        this.setBtausage_dbuser(props.getProperty("btausage.dbuser","error"));
        this.setBtausage_dbpwd(props.getProperty("btausage.dbpwd","error"));

        this.setOrdersf_dburl(props.getProperty("ordersf.dburl","error"));
        this.setOrdersf_dbuser(props.getProperty("ordersf.dbuser","error"));
        this.setOrdersf_dbpwd(props.getProperty("ordersf.dbpwd","error"));

        this.setSnmp_dburl(props.getProperty("snmp.dburl","error"));
        this.setSnmp_dbuser(props.getProperty("snmp.dbuser","error"));
        this.setSnmp_dbpwd(props.getProperty("snmp.dbpwd","error"));

        this.setRadius_dburl(props.getProperty("radius.dburl","error"));
        this.setRadius_dbuser(props.getProperty("radius.dbuser","error"));
        this.setRadius_dbpwd(props.getProperty("radius.dbpwd","error"));

        this.setRadius1AcctPort(new Integer(props.getProperty("radius1.AcctPort","error")));
        this.setRadius1AuthPort(new Integer( props.getProperty("radius1.AuthPort","error")));
        this.setRadius1Server(props.getProperty("radius1.Server","error"));
        this.setRadius1Secret(props.getProperty("radius1.Secret","error"));

        this.setBuilddate(props.getProperty("builddate","na"));

    }

    public void dump()
    {
        System.out.println("bta_url=" + getBta_dburl());
        System.out.println("bta_user=" + getBta_dbuser());
        System.out.println("bta_pwd=" + getBtausage_dbpwd());

        System.out.println("btausage_url=" + getBtausage_dburl());
        System.out.println("btausage_user=" + getBtausage_dbuser());
        System.out.println("btausage_pwd=" + getBtausage_dbpwd());

        System.out.println("snmp_url=" + getSnmp_dburl());
        System.out.println("snmp_user=" + getSnmp_dbuser());
        System.out.println("snmp_pwd=" + getSnmp_dbpwd());

        System.out.println("radius_url=" + getRadius_dburl());
        System.out.println("radius_user=" + getRadius_dbuser());
        System.out.println("radius_pwd=" + getRadius_dbpwd());

    }

    private void safeCloseStream(InputStream is) {
        if (null != is) {
            try {
                is.close();
            } catch (Exception e) {
            }
        }
    }

    public String getBuilddate() {
        return builddate;
    }

    public void setBuilddate(String builddate) {
        this.builddate = builddate;
    }

    public String getBta_dburl() {
        return bta_dburl;
    }

    public void setBta_dburl(String btaDburl) {
        bta_dburl = btaDburl;
    }

    public String getBta_dbuser() {
        return bta_dbuser;
    }

    public void setBta_dbuser(String btaDbuser) {
        bta_dbuser = btaDbuser;
    }

    public String getBta_dbpwd() {
        return bta_dbpwd;
    }

    public void setBta_dbpwd(String btaDbpwd) {
        bta_dbpwd = btaDbpwd;
    }

    public String getBtausage_dburl() {
        return btausage_dburl;
    }

    public void setBtausage_dburl(String btausageDburl) {
        btausage_dburl = btausageDburl;
    }

    public String getBtausage_dbuser() {
        return btausage_dbuser;
    }

    public void setBtausage_dbuser(String btausageDbuser) {
        btausage_dbuser = btausageDbuser;
    }

    public String getBtausage_dbpwd() {
        return btausage_dbpwd;
    }

    public void setBtausage_dbpwd(String btausageDbpwd) {
        btausage_dbpwd = btausageDbpwd;
    }

    public String getOrdersf_dburl() {
        return ordersf_dburl;
    }

    public void setOrdersf_dburl(String ordersf_dburl) {
        this.ordersf_dburl = ordersf_dburl;
    }

    public String getOrdersf_dbuser() {
        return ordersf_dbuser;
    }

    public void setOrdersf_dbuser(String ordersf_dbuser) {
        this.ordersf_dbuser = ordersf_dbuser;
    }

    public String getOrdersf_dbpwd() {
        return ordersf_dbpwd;
    }

    public void setOrdersf_dbpwd(String ordersf_dbpwd) {
        this.ordersf_dbpwd = ordersf_dbpwd;
    }

    public String getSnmp_dburl() {
        return snmp_dburl;
    }

    public void setSnmp_dburl(String snmpDburl) {
        snmp_dburl = snmpDburl;
    }

    public String getSnmp_dbuser() {
        return snmp_dbuser;
    }

    public void setSnmp_dbuser(String snmpDbuser) {
        snmp_dbuser = snmpDbuser;
    }

    public String getSnmp_dbpwd() {
        return snmp_dbpwd;
    }

    public void setSnmp_dbpwd(String snmpDbpwd) {
        snmp_dbpwd = snmpDbpwd;
    }
    public String getRadius_dburl() {
        return radius_dburl;
    }

    public void setRadius_dburl(String radiusDburl) {
        radius_dburl = radiusDburl;
    }

    public String getRadius_dbuser() {
        return radius_dbuser;
    }

    public void setRadius_dbuser(String radiusDbuser) {
        radius_dbuser = radiusDbuser;
    }

    public String getRadius_dbpwd() {
        return radius_dbpwd;
    }

    public void setRadius_dbpwd(String radiusDbpwd) {
        radius_dbpwd = radiusDbpwd;
    }
    public void setPropertiesPath(String propspath)
    {
        this.propertiespath=propspath;
    }
    public String getPropertiesPath()
    {
        return this.propertiespath;
    }

    public String getRadius1Server() {
        return radius1Server;
    }

    public void setRadius1Server(String radius1Server) {
        this.radius1Server = radius1Server;
    }

    public int getRadius1AuthPort() {
        return radius1AuthPort;
    }

    public void setRadius1AuthPort(int radius1AuthPort) {
        this.radius1AuthPort = radius1AuthPort;
    }

    public int getRadius1AcctPort() {
        return radius1AcctPort;
    }

    public void setRadius1AcctPort(int radius1AcctPort) {
        this.radius1AcctPort = radius1AcctPort;
    }

    public String getRadius1Secret() {
        return radius1Secret;
    }

    public void setRadius1Secret(String radius1Secret) {
        this.radius1Secret = radius1Secret;
    }

}
