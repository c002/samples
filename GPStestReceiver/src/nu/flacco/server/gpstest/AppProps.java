package nu.flacco.server.gpstest;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Properties;

import org.apache.log4j.Logger;

public class AppProps {

    private static Logger  log = Logger.getLogger("nu.flacco.aiah.server.AppProps");

    public static AppProps theInstance=null;
    public Properties props=null;
    static String config=null;
    static int debuglevel=0;

    private String propertiespath=null;
    private String builddate="NA";

    private String scot_dburl;
    private String scot_dbuser;
    private String scot_dbpwd;

    private boolean doInsert=true;

    public static AppProps getInstance() {
        if (theInstance==null) {
            theInstance = new AppProps();
        }

        return  theInstance;
    }
/*
    public void loadProperties(String proppath) throws IOException, AppException {

        setPropertiesPath(getPropertiesFromClassPath(proppath));

        if (this.getPropertiesPath()==null)
            throw new AppException(AppException.FATAL, "Unable to find properties file: " + proppath + " in classpath");

        System.out.println("path=" + this.getPropertiesPath());

        setPropertyVars();

       // in.close();
    }
*/
    public void setConfig(String config)
    {
        this.config=config;
    }
    public String getConfig()
    {
        return this.config;
    }

    public void loadConfig() throws IOException, GPServerException {
        this.setConfig(getPropertiesFromClassPath("application.properties"));
        log.info("properties=" + this.config);
        setConfig();
    }

    public void loadConfig(String sResource) throws IOException, GPServerException {
//        String resource=null;
        URL url=null;

//        try {
        log.debug("loadProps()");

         props = new Properties();
        // if (getDebuglevel()>3)
         log.info("config=" + sResource);

        InputStream in = new FileInputStream(new File(this.getConfig()));

        props.load(in);

        setConfig();

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

    public void setConfig() throws IOException, GPServerException
    {

        this.setScot_dburl(props.getProperty("scot.dburl","error"));
        this.setScot_dbuser(props.getProperty("scot.dbuser","error"));
        this.setScot_dbpwd(props.getProperty("scot.dbpwd","error"));

        this.setBuilddate(props.getProperty("builddate","na"));

    }

    public void dump()
    {
        System.out.println("scot_url=" + getScot_dburl());
        System.out.println("scot_user=" + getScot_dbuser());
        System.out.println("scot_pwd=" + getScot_dbpwd());

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

    public String getScot_dburl() {
        return scot_dburl;
    }

    public void setScot_dburl(String scot_dburl) {
        this.scot_dburl = scot_dburl;
    }

    public String getScot_dbuser() {
        return scot_dbuser;
    }

    public void setScot_dbuser(String scot_dbuser) {
        this.scot_dbuser = scot_dbuser;
    }

    public String getScot_dbpwd() {
        return scot_dbpwd;
    }

    public void setScot_dbpwd(String scot_dbpwd) {
        this.scot_dbpwd = scot_dbpwd;
    }

    public boolean isDoInsert() {
        return doInsert;
    }

    public void setDoInsert(boolean doInsert) {
        this.doInsert = doInsert;
    }

}
