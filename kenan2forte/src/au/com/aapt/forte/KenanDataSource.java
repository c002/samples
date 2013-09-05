package au.com.aapt.forte;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.sql.ResultSet;
import java.sql.Statement;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import org.apache.log4j.Logger;

public class KenanDataSource {

    static private ResultSet results=null;
    static private Statement stmt = null;
    DBConnect db = null;
    int recordNumber=0;
    AppProps appProps=null;

    private static Logger  log = Logger.getLogger("au.com.aapt.forte.KenanDataSource");

    private ArrayList <KenanRecord> kenanRecordList = new ArrayList<KenanRecord>();

    public KenanDataSource()
    {
        appProps = AppProps.getInstance();
    }

    public void loadFromFile()
    {
        String s;
        String [] tokens=null;

        try {
            BufferedReader in = new BufferedReader(new FileReader(appProps.getImport()));
            do {
               s = in.readLine();
               if (s==null) continue;
               //System.out.println("line=" + s);
              // System.out.println("s=" + s);
               //tokens=s.split("^\"|\",\"|\"$");
               tokens=s.split("\\|");
              // System.out.println("len="+tokens.length);
              // if (tokens.length<17)
              //     continue;

               if (tokens[0].compareToIgnoreCase("D")!=0)
                   continue;

               KenanRecord kr = new KenanRecord();
               recordNumber=recordNumber+1;
               kr.setRecordNumber(recordNumber);
               for (int i=0; i<tokens.length;i++) {
                   //System.out.print(",t="+tokens[i]);
                    //System.out.print("tokens[ " + i + "]=" + tokens[i] +",");
                    if (tokens[i].equalsIgnoreCase("null")==true)
                       tokens[i]=null;
                    switch (i) {
                        case (0) : kr.setRecordType(tokens[i]); break;
                        case (1) : kr.setAccountNumber(new Integer(tokens[i])); break;
                        case (2) : kr.setSalesforceId(tokens[i]); break;
                        case (3) : kr.setActiveDate(tokens[i]); break;
                        case (4) : kr.setChangeDate(tokens[i]); break;
                        case (5) : kr.setAccountStatus(tokens[i]); break;
                        case (6) : kr.setAccountCategory(tokens[i]); break;
                        case (7) : kr.setCompanyName(tokens[i]); break;
                        case (8) : kr.setSalutation(tokens[i]); break;
                        case (9) : kr.setBillFname(tokens[i]); break;
                        case (10): kr.setBillLname(tokens[i]); break;
                        case (11): kr.setBillAddress1(tokens[i]); break;
                        case (12): kr.setBillAddress2(tokens[i]); break;
                        case (13): kr.setBillAddress3(tokens[i]); break;
                        case (14): kr.setBillSuburb(tokens[i]); break;
                        case (15): kr.setBillState(tokens[i]); break;
                        case (16): kr.setBillPostCode(tokens[i]); break;
                        case (17): kr.setCustAddress1(tokens[i]); break;
                        case (18):  kr.setCustAddress2(tokens[i]); break;
                        case (19):  kr.setCustAddress3(tokens[i]); break;
                        case (20):  kr.setCustSuburb(tokens[i]); break;
                        case (21):  kr.setCustState(tokens[i]); break;
                        case (22):  kr.setCustPostCode(tokens[i]); break;
                    }
               }
              // System.out.println(kr.toString());
               kenanRecordList.add(kr);


           } while (s!=null);

           in.close();

           if (appProps.getExport()!=null)
               doExport();

           if (appProps.getExport()!=null) {
               BufferedWriter out = new BufferedWriter(new FileWriter(appProps.getExport()));

               String header="Record Type|Account Number|Salesforce ID|Active Date|Status Change Date|Account Status|Account Category|Company Name|Bill Salutation|Bill Fname|Bill Lname|Bill Address1|Bill Address2|Bill Address3|Bill suburb|Bill State|Bill post code|Cust Address1|Cust Address2|Cust Address3|Cust suburb|Cust State|Cust post code\n";
               String trailer="T|Kenan|Forte|0001|" + date2str(new Date(),"yyyyMMdd hh:mm:ss") + "|" + kenanRecordList.size() + "\n"; //  20120306 15:27:12|48\n";

               out.write(header);
               for (KenanRecord k : kenanRecordList) {
                   out.write(k.toCSV("|"));
               }
               out.write(trailer);
               out.close();
           }

        } catch (Exception e) {
           e.printStackTrace();
        }
    }

    public void loadFromWareHouse()
    {
       try {
            db = new DBConnect("warehouse");
            db.c.createStatement();

            Date fromdate = AppProps.getInstance().getFromDate();
            Date todate   = AppProps.getInstance().getToDate();

            String query="select count(*) as count from kenan_atomic.CUSTOMER_ID_ACCT_MAP";
            // This query is untested
            String dwquery=
                String.format(
                "SELECT Ciam1.External_id, " +
                "Ciam2.external_id, " +
                "dateformat(Ciam1.active_date, 'yyyymmdd'), " +
                "case CMF.ACCOUNT_STATUS when -2 then dateformat(cmf.account_status_dt, 'yyyymmdd') " +
                    " when -1 then NULL " +
                    " when  0 then NULL " +
                    "    else dateformat(cmf.account_status_dt, 'yyyymmdd') end, " +
                "case CMF.ACCOUNT_STATUS when -2 then 'Disc Done' " +
                    " when -1 then 'Active' " +
                    " when  0 then 'Active' " +
                    "    else 'Disc Req' end, " +
                 "CASE WHEN   ( patindex(upper(ac.ACCOUNT_CATEGORY_DISPLAY_VALUE), '%%INTERNAL%%') ) != 0 " +
                 "   THEN  'Intenal' " +
                 "   When   ( patindex(upper(ac.ACCOUNT_CATEGORY_DISPLAY_VALUE), '%%DAILY CDRS%%') ) != 0 " +
                 "   THEN 'Wholesale' " +
                 "   ELSE 'ABS' " +
                 "   END, "+
            " TRIM(cmf.bill_company), " +
            " TRIM(cmf.bill_name_pre), " +
            " TRIM(cmf.bill_fname), " +
            " TRIM(cmf.bill_lname), " +
            " TRIM(cmf.bill_address1), " +
            " TRIM(cmf.bill_address2), " +
            " TRIM(cmf.bill_address3), " +
            " TRIM(cmf.bill_city), " +
            " TRIM(cmf.bill_state), " +
            " TRIM(cmf.bill_zip), " +
            " TRIM(cmf.cust_address1), " +
            " TRIM(cmf.cust_address2), " +
            " TRIM(cmf.cust_address3), " +
            " TRIM(cmf.cust_city), " +
            " TRIM(cmf.cust_state), " +
            " TRIM(cmf.cust_zip) " +
     " FROM   kenan_atomic.customer_id_acct_map ciam1, " +
            " kenan_atomic.customer_id_acct_map ciam2, " +
            " kenan_atomic.account_category ac, " +
            " kenan_atomic.cmf cmf " +
    " WHERE   ciam1.external_id_type = 1 and " +
            " ciam1.account_no > 2000000000  and  " +
            " ciam1.account_no = ciam2.account_no and " +
            " ciam2.external_id_type = 88   and " +
            " ciam1.account_no = cmf.account_no  and " +
            " cmf.account_category = ac.account_category "+
            " and ciam1.RECORD_EXPY_DT         = '9999-12-31' " +
            " and ciam2.RECORD_EXPY_DT         = '9999-12-31' " +
            " and CMF.RECORD_EXPY_DT         = '9999-12-31' " +
            " and ac.RECORD_EXPY_DT         = '9999-12-31' " +
            " and ciam1.Record_efft_dt >= dateformat('%s', 'yyyymmdd')" +
            " and ciam1.Record_efft_dt <= dateformat('%s', 'yyyymmdd') "
            ,date2str(appProps.getFromDate(),"yyyyMMdd")
            ,date2str(appProps.getToDate(),"yyyyMMdd"));

            String dwtestquery= // NOTE Uses Type=10, just to get some data while 33 is NA
                String.format(
                "SELECT Ciam1.External_id, " +
                "Ciam2.external_id, " +
                "dateformat(Ciam1.active_date, 'yyyymmdd'),  " +
                "case CMF.ACCOUNT_STATUS when -2 then dateformat(cmf.account_status_dt, 'yyyymmdd') "+
                      "when -1 then NULL " +
                      "when  0 then NULL " +
                      "     else dateformat(cmf.account_status_dt, 'yyyymmdd') end, " +
                "case CMF.ACCOUNT_STATUS when -2 then 'Disc Done' " +
                      "when -1 then 'Active' " +
                      "when  0 then 'Active' " +
                      "        else 'Disc Req' end, " +
                "CASE WHEN   ( patindex(upper(ac.ACCOUNT_CATEGORY_DISPLAY_VALUE), '%%INTERNAL%%') ) != 0 " +
                     "THEN  'Intenal' " +
                     "When   ( patindex(upper(ac.ACCOUNT_CATEGORY_DISPLAY_VALUE), '%%DAILY CDRS%%') ) != 0 " +
                     "THEN 'Wholesale' " +
                     "ELSE 'ABS' " +
                     "END , " +
                "TRIM(cmf.bill_company), " +
                "TRIM(cmf.bill_name_pre), " +
                "TRIM(cmf.bill_fname), " +
                "TRIM(cmf.bill_lname), " +
                "TRIM(cmf.bill_address1), " +
                "TRIM(cmf.bill_address2), " +
                "TRIM(cmf.bill_address3), " +
                "TRIM(cmf.bill_city), " +
                "TRIM(cmf.bill_state), " +
                "TRIM(cmf.bill_zip), " +
                "TRIM(cmf.cust_address1), " +
                "TRIM(cmf.cust_address2), " +
                "TRIM(cmf.cust_address3), " +
                "TRIM(cmf.cust_city), " +
                "TRIM(cmf.cust_state), " +
                "TRIM(cmf.cust_zip) " +
     " FROM   kenan_atomic.customer_id_acct_map ciam1, " +
     "        kenan_atomic.customer_id_acct_map ciam2, " +
     "        kenan_atomic.account_category ac, " +
     "        kenan_atomic.cmf cmf " +
     " WHERE  " +
            " ciam1.external_id_type = 10  and " +
            " ciam1.account_no > 2000000000   and  " +
            " ciam1.account_no = ciam2.account_no and " +
            " ciam1.account_no = cmf.account_no  and " +
            " cmf.account_category = ac.account_category " +
            " and ciam1.RECORD_EXPY_DT         = '9999-12-31' " +
            " and ciam2.RECORD_EXPY_DT         = '9999-12-31' " +
            " and CMF.RECORD_EXPY_DT         = '9999-12-31' " +
            " and ac.RECORD_EXPY_DT         = '9999-12-31' " +
            " and ciam1.Record_efft_dt >= dateformat('%s', 'yyyymmdd')" +
            " and ciam1.Record_efft_dt <= dateformat('%s', 'yyyymmdd') "
            ,date2str(appProps.getFromDate(),"yyyyMMdd")
            ,date2str(appProps.getToDate(),"yyyyMMdd"));


            query=dwquery;
            log.info("DW SQL:"+query);

            stmt = db.c.createStatement();
            results= stmt.executeQuery(query);

            while (results!=null && results.next())  {

                KenanRecord kr = new KenanRecord();
                recordNumber=recordNumber+1;

                kr.setRecordNumber(recordNumber);
                kr.setRecordType( "T");
                kr.setAccountNumber(intfilter(results.getString(1)));
                kr.setSalesforceId(results.getString(2));
                kr.setActiveDate(results.getString(3));
                kr.setChangeDate(results.getString(4));
                kr.setAccountStatus(results.getString(5));
                kr.setAccountCategory(results.getString(6));
                kr.setCompanyName(results.getString(7));
                kr.setSalutation(results.getString(8));
                kr.setBillFname(results.getString(9));
                kr.setBillLname(results.getString(10));
                kr.setBillAddress1(results.getString(11));
                kr.setBillAddress2(results.getString(12));
                kr.setBillAddress3(results.getString(13));
                kr.setBillSuburb(results.getString(14));
                kr.setBillState(results.getString(15));
                kr.setBillPostCode(results.getString(16));
                kr.setCustAddress1(results.getString(17));
                kr.setCustAddress2(results.getString(18));
                kr.setCustAddress3(results.getString(19));
                kr.setCustSuburb(results.getString(20));
                kr.setCustState(results.getString(21));
                kr.setCustPostCode(results.getString(22));

                kenanRecordList.add(kr);
            }
            if (appProps.getExport()!=null)
                doExport();

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                stmt.close();
                db.c.close();
                log.info("Closed KenanDataSource()");
            } catch (Exception e) {
                log.error("Failed to close db.");
            }
        }
    }

    private int intfilter(String indata)
    {
        StringBuilder s=new StringBuilder();
        for (int i=0; i<indata.length();i++) {
            if (indata.charAt(i)>='0' && indata.charAt(i)<='9')
               s.append(indata.charAt(i));
        }
        return Integer.valueOf(s.toString());
    }

    private void doExport() throws IOException
    {
        BufferedWriter out = new BufferedWriter(new FileWriter(appProps.getExport()));

        for (KenanRecord k : kenanRecordList) {
            out.write(k.toCSV("|"));
        }
        out.close();
    }

    private String date2str(Date d, String format)
    {
        if (d==null) return null;

        DateFormat df = new SimpleDateFormat(format);
        return(df.format(d));
    }

    public ArrayList<KenanRecord> getKenanRecordList() {
        return kenanRecordList;
    }

    public void setKenanRecordList(ArrayList<KenanRecord> kenanRecordList) {
        this.kenanRecordList = kenanRecordList;
    }

}
