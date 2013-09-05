package au.com.aapt.forte;

import java.util.*;
import java.sql.*;
import java.io.*;

import org.apache.log4j.Logger;
import java.sql.*;
import java.io.*;
import org.apache.log4j.BasicConfigurator;

import com.sybase.jdbcx.SybDriver;

//import com.mysql.jdbc.*;

/**
 * Tools for connecting to databases
 * @author harryr
 */
public class DBConnect implements Serializable {
    final static String rcsid = "$Source: /import/cvsroot/dev/internal/kenan2forte/src/au/com/aapt/forte/DBConnect.java,v $";
    final static String rcssrc= "$Id: DBConnect.java,v 1.2 2012/03/29 05:43:02 harryr Exp $";

    //private static final String PROP_URL = "clarify";
    static private AppProps appProps=null;

    private static Logger  log = Logger.getLogger("au.com.aapt.forte.DBConnect");

    public Connection c;
    //Logger logger;

    public DBConnect() throws IOException , K2FException {
         appProps = appProps.getInstance();
        this.c= getJdbcConnection("warehouse");
    }

    /**
     * Connect to the given database
     */
    public DBConnect(String dbname) throws K2FException 
    {
       
       try { 
           appProps = appProps.getInstance();
           this.c= getJdbcConnection(dbname);

           this.c.setAutoCommit(false);
       } catch (SQLException se) {
           throw new K2FException(se);
           
       }
    }

    public static Connection getJdbcConnection(String cstr) throws K2FException
    {

       String dbUser=null, dbPass=null, dbUrl=null;
       String dbstr=null;

      // ResourceBundle rs = ResourceBundle.getBundle(PROP_URL);
      // String wsc = rs.getString("wscontainer");

       Connection physicalConnection=null;

        if (cstr==null) {
            return null;
        }

         try {
            if (cstr.compareTo("warehouse")==0) {
               Class.forName("com.sybase.jdbc2.jdbc.SybDriver");

               dbUser = appProps.getWarehouseDbuser();
               dbPass = appProps.getWarehouseDbpwd();
               dbUrl= appProps.getWarehouseDburl();
               
            } else if (cstr.compareTo("forte")==0) {
                Class.forName("oracle.jdbc.driver.OracleDriver");

                dbUser = appProps.getForteDbuser();
                dbPass = appProps.getForteDbpwd();
                dbUrl =  appProps.getForteDburl();
                
            }
            log.debug(dbUser+"/"+dbPass+"@"+dbUrl);
            return DriverManager.getConnection(dbUrl, dbUser, dbPass);

       }

       catch (Exception e) {
           throw new K2FException( e);
           // throw new K2FException("ERROR Getting db connection : " + e);
          
       }
     }
}