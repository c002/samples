package au.com.aapt.forte;

import org.apache.log4j.Logger;

import au.com.aapt.forte.AppProps;
import gnu.getopt.Getopt;
import gnu.getopt.LongOpt;

public class Main {
    static private AppProps appProps=null;
    private static final Logger log = Logger.getLogger("au.com.aapt.forte.Main");
    private static String props = "application.properties";
    DBConnect db = null;

    public Main()
    {
        try {
            log.info("Run start");

            //appProps = AppProps.getInstance();
            appProps.loadConfig();

            KenanDataSource kd = new KenanDataSource();
            if (appProps.importfile==null)
                kd.loadFromWareHouse();
            else
                kd.loadFromFile();

            MapKenan2Forte mapk2f = new MapKenan2Forte(kd.getKenanRecordList());

         //   if (appProps.isDoInsert()) {
            db = new DBConnect("forte");

            log.info(mapk2f.getForteRecordList().size() + " records to insert.");
            StringBuilder summary=new StringBuilder();
            
            for (ForteRecord fr : mapk2f.getForteRecordList()) {
                    fr.update(db);
                    summary.append(fr.summary() + "\n");
            }
            
            if (appProps.isDoInsert()) { 
                db.c.commit();
                log.info("committed into Forte");
            } else
                log.info("No Records where inserted into Forte ( no --insert option)");
            // }
            
            new SendMail("kenan2forte new customers",summary.toString());
            
        } catch (Exception e) {
        	new SendMail("kenan2forte error",e.getMessage());
            e.printStackTrace();
            log.error(e.getMessage());
            try {
                if (db!=null && db.c!=null) {
                    log.warn("Data Rollback ...");
                    db.c.rollback();
                } else
                    log.warn("Connection closed.");
            } catch (Exception e2) {
                log.warn("Rollback Failed");
                e2.printStackTrace();
            }
        } finally {
            try {
                if (db!=null) {
                    db.c.close();
                    log.info("Closed ForteDB");
                }
            } catch (Exception e) {
                e.printStackTrace();
                log.error("Failed to close ForteDB.");
            }
        }

        log.info("Run Complete");

    }

    public static void main(String[] argv)
    {
        String arg;
        int c;
        boolean haveFromdate=false;
        boolean haveTodate=false;
        boolean haveFile=false;

        LongOpt[] longopts = new LongOpt[10];

        longopts[0] = new LongOpt("help", LongOpt.NO_ARGUMENT, null, 'h');
        longopts[1] = new LongOpt("config", LongOpt.REQUIRED_ARGUMENT, null, 'c');
        longopts[2] = new LongOpt("fromdate", LongOpt.REQUIRED_ARGUMENT, null, 'f');
        longopts[3] = new LongOpt("todate", LongOpt.REQUIRED_ARGUMENT, null, 't');
        longopts[4] = new LongOpt("import", LongOpt.REQUIRED_ARGUMENT, null, 'm');
        longopts[5] = new LongOpt("export", LongOpt.REQUIRED_ARGUMENT, null, 'x');
        longopts[6] = new LongOpt("insert", LongOpt.NO_ARGUMENT, null, 'i');
        longopts[7] = new LongOpt("debug", LongOpt.REQUIRED_ARGUMENT, null, 'd');

        appProps = AppProps.getInstance();

        Getopt g = new Getopt("kenan2forte", argv, "f:ht:im:x:d:", longopts);

        try {
          while ((c = g.getopt()) != -1)
            switch(c)
            {
                 case 'h':
                   usage();
                   return;

                 case 'c':
                     arg = g.getOptarg();
                     appProps.setConfig(arg.toString());
                     break;
                 case 'd':
                     arg = g.getOptarg();
                     appProps.setDebuglevel(new Integer(arg.toString()));
                     break;
                 case 'f':
                     arg = g.getOptarg();
                     appProps.setFromDate(arg.toString());
                     haveFromdate=true;
                     break;
                 case 'i':
                     arg = g.getOptarg();
                     appProps.setDoInsert(true);
                     break;

                case 't':
                     arg = g.getOptarg();
                     appProps.setToDate(arg.toString());
                     haveTodate=true;
                     break;
                case 'm':
                    arg = g.getOptarg();
                    haveFile=true;
                    appProps.setImport(arg.toString());
                    break;
                case 'x':
                    arg = g.getOptarg();
                    appProps.setExport(arg.toString());
                    break;
            }
        } catch (K2FException ke) {
            ke.printStackTrace();

        } catch (NullPointerException ne) {
            usage();
            return;
        }
/****** test */
       // haveFile=true;
       // appProps.setImport("etc/kenandata.txt");
       // appProps.setDoInsert(true);
       // appProps.setDebuglevel(3);
/*******/

        if ((haveFromdate && haveTodate) || haveFile)
            new Main();
        else
            usage();
    }

    public static void usage()
    {
        System.out.println("kenan2forte [--insert] --fromdate=yyyymmdd --todate=yyyymmdd | --import=importfile [--export=exportfile]");
        System.out.println("\t--insert   , Insert data into Forte, otherwise only exercise mappings.");
        System.out.println("\t--fromdate , Startdate for records to select.");
        System.out.println("\t--todate   , Enddate for records to select.");
        System.out.println("\t--import   , Use data from file instead, Format as per docco.");
        System.out.println("\t--export   , Export data source to txt file. Format as per docco.");
        System.out.println("\t--debug    , Create noisy output. For mapping debugging use 1 .");

    }

}
