/*
 * DateUtil.java
 *
 * Created on 22 April 2004, 12:16
 */

package au.com.aapt.linxfaults;

import java.util.*;
import java.text.*;

/**
 *
 * @author  harryr
 */
public class DateUtil {
    private Calendar cal;
    private Vector months=null;
    private int nowday=-1;
    private String nowmonth=null;
    private int nowyear=-1;
    private long convertthis=0;
    private String convertfmt="+[dd.MM HH:mm]+";
    
    /** Creates a new instance of DateUtil */
    public DateUtil() {
      cal = new GregorianCalendar();  
      this.nowday =  getThisDay();
      this.nowmonth = getThisMonth();
      this.nowyear = getThisYear();
      
      initMonths();
    }
    
    private void initMonths() {
      months = new Vector();     
      months.add("Jan"); months.add("Feb"); months.add("Mar");months.add("Apr");
      months.add("May"); months.add("Jun"); months.add("Jul");months.add("Aug");
      months.add("Sep"); months.add("Oct"); months.add("Nov");months.add("Dec");  
    }
    
    public String convert(long epoch) {
        return convert(epoch, "dd-MM-yyyy HH:mm:ss");
    }
    
    public void setConvertthis(long con) {
        this.convertthis=con;
    }
    
    public void setConvertfmt(String fmt) {
        this.convertfmt=fmt;
    }
    
    public String getConv() {
        return convert(convertthis, convertfmt);
    }
    /** 
     * Convert an epoch type of time into a string as given by fmt
     * 
     * @param epoch Seconds since 1-Jan-1970 to convert 
     * @param fmt the Format to use to convert time. This is like SimpleDateForma
t
     *
     */
    public Long str2epoch(Date date)
    {
        Calendar cal = new GregorianCalendar();
        if (date!=null) cal.setTime(date);
        return (new Long(cal.getTimeInMillis() / 1000)); 
    }
    
    public Long str2epoch(String datestr, String fmt)
    {
        Date date;
        ParsePosition pos = new ParsePosition(0);
        
        //System.err.println("d=" + datestr + ", fmt=" + fmt);
        
        Calendar cal = new GregorianCalendar();
        DateFormat df = new SimpleDateFormat(fmt);
        date = df.parse(datestr, pos);
        //Date trialTime;
        cal.setTime(date);
        return (new Long(cal.getTimeInMillis() / 1000));
    }
    
    public String convert(long epoch, String fmt) {

        Calendar cal = new GregorianCalendar();
//      int offset = cal.get((Calendar.ZONE_OFFSET) + cal.get(Calendar.DST_OFFSET));

//        DateFormat df = new SimpleDateFormat("[dd.MM HH:mm]");
        DateFormat df = new SimpleDateFormat(fmt);
        // df.setCalendar(gc);
        java.util.Date ct1 = new java.util.Date((epoch * 1000));

        return df.format(ct1);             
    }
    /* Compares d1 to d2
     *  Returns 0 if d1==d2
     *          1 if d1 > d2
     *         -1 if d1 < d2
    */
    public int CompareDates(Date d1, Date d2)
    {
        if (d1==null && d2!=null)
            return -1; 
        else if (d1!=null && d2==null)
            return 1;
        else if (d1==null && d2==null)
            return 0;
        
        Long ep1 = str2epoch(d1);
        Long ep2 = str2epoch(d2);
        
       // System.out.println("DATE COMPARE:" + ep1.longValue() + " <> " + ep2.longValue());
        
        if (ep1.longValue() >  ep2.longValue())
            return 1;
        else if (ep1.longValue() <  ep2.longValue())
            return -1;
        else 
            return 0;
    }
    
    public int offset() {
     
        return (cal.get(Calendar.ZONE_OFFSET) + cal.get(Calendar.DST_OFFSET))/1000;
    }

    public Vector getYears() 
    {
      Vector v = new Vector();    
      Date trialTime = new Date();
      cal.setTime(trialTime);  
      
      // System.out.println("YEAR: " + cal.get(cal.YEAR)); 
       
      for (int y=cal.get(cal.YEAR)-4; y<cal.get(cal.YEAR)+2; y++)
          v.add(new Integer(y));
      
      //v.add(new Integer(2004));
      
      return(v);
    }
    
    public Vector getMonths() 
    {
        if (months==null) {
            initMonths();
        }

     return(months);
    }

    public Vector getDays() 
    {
     Vector v = new Vector();   
     for (int y=1; y<32; y++)
          v.add(new Integer(y));
      return(v);
    }
    
   public int getThisDay() 
    {
      if (nowday < 0 ) {
        Date trialTime = new Date();
        cal.setTime(trialTime);
        nowday= cal.get(cal.DAY_OF_MONTH);
      }
      //return(new Integer(nowday).toString());
      return(nowday);
    }

   public String getThisMonth() 
    {
      int i=-1;
      if (months==null) {
            //System.out.println("Months is null");
            initMonths();
      }
      if (nowmonth==null) {
            Date trialTime = new Date();
            cal.setTime(trialTime);
            i = cal.get(cal.MONTH);
            nowmonth = (String) months.get(i);  
      }

      return(nowmonth);
    }
   
    public int getThisYear() 
    {
      if (nowyear <0) {
        Date trialTime = new Date();
        cal.setTime(trialTime);
        nowyear = cal.get(cal.YEAR);
      }  
      return(nowyear);
    }
    
    public static void xmain(String[] argv) 
    {
        DateUtil dutil = new DateUtil();
        Vector v = dutil.getYears();
        
        for (int i=0; i< v.size(); i++)
            System.out.println( (v.get(i)).toString() );
    }
}
