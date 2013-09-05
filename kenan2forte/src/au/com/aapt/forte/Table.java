package au.com.aapt.forte;

import java.lang.reflect.Method;
import java.sql.ResultSet;
import java.sql.Statement;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;

import org.apache.log4j.Logger;

public abstract class Table {

    static protected ResultSet results=null;
    static protected Statement stmt = null;
    static private  DateFormat df = new SimpleDateFormat("yyyyMMdd");

    protected static final Logger log = Logger.getLogger("au.com.aapt.forte.ForteTable");

    public void update(DBConnect db) throws K2FException
    {}

    public void runQueries(DBConnect db, String squery, String upquery, String inquery) throws K2FException
    {
        boolean doInsert=false;
        boolean doUpdate=false;
        String query=null;

        log.debug(this.getClass().getName() +", DEBUG SQL: " + squery);
        log.debug(this.getClass().getName() +", DEBUG SQL: " + inquery);
        log.debug(this.getClass().getName() +", DEBUG SQL: " + upquery);

        try {
            query=squery;
            log.info(this.getClass().getName() +" SQL: " + squery);
            stmt = db.c.createStatement();
            results= stmt.executeQuery(squery);
            while (results!=null && results.next())  {
                if (results.getInt(1)==0)
                    doInsert=true;
                else if (results.getInt(1)>0)
                    doUpdate=true;
            }

            if (doInsert==true) {
                query=inquery;
                log.info(this.getClass().getName() +" SQL: " + inquery);
                results= stmt.executeQuery(inquery);
            } else if (doUpdate==true && upquery!=null) {
                query=upquery;
                log.info(this.getClass().getName() +" SQL: " + upquery);
                results= stmt.executeQuery(upquery);
            }

            stmt.close();
        } catch (Exception e) {
            log.error("Exception on: " + query);
            throw new K2FException(e);
        }
    }

    public String toString()
    {
        StringBuilder s= new StringBuilder();

        try {
            Class<?> c = Class.forName(this.getClass().getName());
            Method[] methodList = c.getMethods();
            Method m1 = c.getMethod("getClass",( java.lang.Class[]) null );

            s.append("   +-- " + m1.invoke(this,(Object[]) null)  + "\n");
            for (Method m : methodList) {
                if (m.getName().compareTo("getClass")==0) continue;
                if (m.getName().startsWith("get")) {
                    s.append("   |  " + m.getName().substring(3) + " = " + m.invoke(this, (Object[])null) + "\n");
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return(s.toString());
    }


    protected Date str2Date(String sdate)
    {
        if (sdate==null) return(null);

        Date date;// = new Date();
      //  DateFormat df = new SimpleDateFormat("yyyyMMdd");
        try {
            date = df.parse(sdate);
        } catch (ParseException pe) {
            return(null);
        }

        return(date);
    }

    protected String date2str(Date d)
    {
        if (d==null) return null;

      //  DateFormat df = new SimpleDateFormat("yyyyMMdd");
        return(df.format(d));
    }

    protected String date2str(Date d, String format)
    {
        if (d==null) return null;

        DateFormat df = new SimpleDateFormat(format);
        return(df.format(d));
    }
    protected String date2epochstr(Date d)
    {
        if (d==null) return null;
        Calendar cal = new GregorianCalendar();
        cal.setTime(d);
        String epochstr = String.valueOf(cal.getTimeInMillis());
        return(epochstr);
    }

    protected String escquote(String s)
    {
        return s.replaceAll("'", "''");
    }
}
