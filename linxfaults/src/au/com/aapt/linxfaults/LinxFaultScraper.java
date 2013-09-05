package au.com.aapt.linxfaults;

import gnu.getopt.Getopt;
import gnu.getopt.LongOpt;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.util.*;
import java.text.DateFormat;
import java.text.SimpleDateFormat;

import com.gargoylesoftware.htmlunit.WebClient;
import com.gargoylesoftware.htmlunit.attachment.Attachment;
import com.gargoylesoftware.htmlunit.attachment.AttachmentHandler;
import com.gargoylesoftware.htmlunit.attachment.CollectingAttachmentHandler;
import com.gargoylesoftware.htmlunit.html.DomAttr;
import com.gargoylesoftware.htmlunit.html.DomElement;
import com.gargoylesoftware.htmlunit.html.DomNode;
import com.gargoylesoftware.htmlunit.html.HtmlAnchor;
import com.gargoylesoftware.htmlunit.html.HtmlButton;
import com.gargoylesoftware.htmlunit.html.HtmlCheckBoxInput;
import com.gargoylesoftware.htmlunit.html.HtmlElement;
import com.gargoylesoftware.htmlunit.html.HtmlForm;
import com.gargoylesoftware.htmlunit.html.HtmlHtml;
import com.gargoylesoftware.htmlunit.html.HtmlImageInput;
import com.gargoylesoftware.htmlunit.html.HtmlInput;
import com.gargoylesoftware.htmlunit.html.HtmlPage;
import com.gargoylesoftware.htmlunit.html.HtmlLink;
import com.gargoylesoftware.htmlunit.html.HtmlRadioButtonInput;
import com.gargoylesoftware.htmlunit.html.HtmlSelect;
import com.gargoylesoftware.htmlunit.html.HtmlStrong;
import com.gargoylesoftware.htmlunit.html.HtmlSubmitInput;
import com.gargoylesoftware.htmlunit.html.HtmlTable;
import com.gargoylesoftware.htmlunit.html.HtmlTableCell;
import com.gargoylesoftware.htmlunit.html.HtmlTableRow;
import com.gargoylesoftware.htmlunit.html.HtmlTextArea;
import com.gargoylesoftware.htmlunit.html.HtmlFieldSet;
import com.gargoylesoftware.htmlunit.javascript.host.Event; //import com.gargoylesoftware.htmlunit.util.NameValuePair;
import com.gargoylesoftware.htmlunit.AlertHandler;
import com.gargoylesoftware.htmlunit.NicelyResynchronizingAjaxController;
import com.gargoylesoftware.htmlunit.Page;
import com.gargoylesoftware.htmlunit.ProxyConfig;
import com.gargoylesoftware.htmlunit.BrowserVersion;
import com.gargoylesoftware.htmlunit.ElementNotFoundException;
import com.gargoylesoftware.htmlunit.ScriptResult; //import com.sun.xml.internal.bind.v2.schemagen.xmlschema.List;
import com.gargoylesoftware.htmlunit.TextPage;
import com.gargoylesoftware.htmlunit.WebWindow;
import com.gargoylesoftware.htmlunit.WebWindowEvent;
import com.gargoylesoftware.htmlunit.WebWindowListener;
//import com.sun.java.swing.plaf.windows.resources.windows;

import org.apache.log4j.*;


public class LinxFaultScraper implements AlertHandler {

    final static String rcsid = "$Source$";
    final static String rcssrc = "$Id$";

    private final org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(getClass().getName());
    private final long _31days=2678400; // in seconds

    enum MODE { STATUS, REPORT, CSVFILE };
    static MODE mode=MODE.REPORT;
    static boolean verbose=false;          // info output.

    static Properties props = null;
    static String config = null;
    String targetpage = null;
    String targetpagetitle = null;

    String proxyhost = null;
    String keystore = null;
    String keystorePassword = null;

    Integer proxyport = null;

    WebClient webclient = null;
    URL url = null;

    HtmlPage nextPage = null;
    HtmlPage rootPage = null;
    static long timeout = 0;

    Integer debuglevel = new Integer(2);
    String pagetracedir = null;

    Integer popupix=1;

    final List<Attachment> attachments = new ArrayList<Attachment>();
    final LinkedList<WebWindow> windows = new LinkedList<WebWindow>();

    static class Indata {
        String fromdate;
        String todate;
        boolean dogenerate=false;
        boolean dofetch=false;
        String keystore=null;
        String keystorepwd=null;

        public String toString() {
            String s=String.format("fromdate=%s, todate=%s, generate=%s, fetch=%s, keystore=%s",
                    fromdate, todate, dogenerate, dofetch, keystore);
            return(s);
        }
    }

    public static void main(String argv[])
    {

        String arg;
        int c;
        boolean dohelp=false;
        Indata indata = new Indata();

        LongOpt[] longopts = new LongOpt[10];

        longopts[0] = new LongOpt("help", LongOpt.NO_ARGUMENT, null, 'h');
        longopts[1] = new LongOpt("fromdate", LongOpt.REQUIRED_ARGUMENT, null, 'f');
        longopts[2] = new LongOpt("todate", LongOpt.REQUIRED_ARGUMENT, null, 't');
        longopts[3] = new LongOpt("verbose", LongOpt.NO_ARGUMENT, null, 'v');
        longopts[4] = new LongOpt("fetch", LongOpt.NO_ARGUMENT, null, 'e');
        longopts[5] = new LongOpt("generate", LongOpt.NO_ARGUMENT, null, 'g');
        longopts[6] = new LongOpt("keystore", LongOpt.REQUIRED_ARGUMENT, null, 'k');
        longopts[7] = new LongOpt("keystorepwd", LongOpt.REQUIRED_ARGUMENT, null, 'p');

        Getopt g = new Getopt("linxfaults", argv, "vf:ht:egk:p:", longopts);

        try {
        while ((c = g.getopt()) != -1)
            switch(c)
            {
                 case 'h':
                   arg = g.getOptarg();
                   dohelp=true;
                    break;
                 case 'v':
                     arg = g.getOptarg();
                     verbose=true;
                      break;
                 case 'f':
                     arg = g.getOptarg();
                     indata.fromdate= arg.toString();
                     break;
                case 't':
                     arg = g.getOptarg();
                     indata.todate=arg.toString();
                     break;
                case 'e':
                    arg = g.getOptarg();
                    indata.dofetch=true;
                    break;
                case 'g':
                    arg = g.getOptarg();
                    indata.dogenerate=true;
                    break;
                case 'k':
                    arg = g.getOptarg();
                    indata.keystore=arg.toString();
                    break;
                case 'p':
                    arg = g.getOptarg();
                    indata.keystorepwd=arg.toString();
                    break;
            }
        } catch (NullPointerException ne) {
            ne.printStackTrace();
            usage();
            return;
        }

        if (dohelp==true) {
            usage();
            System.exit(0);
        }

        if (indata.keystore!=null) {
            File f = new File(indata.keystore);
            if (f.exists()==false) {
                System.err.println("keystore not found: " + indata.keystore);
                System.exit(0);
            }

        }

        if (indata.dogenerate==true && indata.dofetch==true) {
            System.err.println("Must give either --generate or --fetch");
            System.exit(0);
        }

        new LinxFaultScraper(indata);
    }

    public LinxFaultScraper(Indata indata) {
        HtmlPage currentPage = null;
        
         try {
           setConfig(indata);
           if (verbose==true)
               System.out.println("Running in mode: " + mode);
           if (mode==MODE.REPORT) {

               // Check date range and before we go too far
               DateUtil du = new DateUtil();
               long fdate = du.str2epoch(indata.fromdate, "dd/MM/yyyy");
               long tdate=0;
               if (indata.todate!=null)
                   tdate = du.str2epoch(indata.todate, "dd/MM/yyyy");
               else
                   tdate = du.str2epoch(indata.todate, "dd/MM/yyyy");
               if (verbose==true) System.out.println("date range=" + fdate + " - " + tdate);
               if ( ((tdate - fdate) > _31days) ||  ((tdate - fdate) < 0) ) {
                   System.err.println("Data range must be  0 < days < 31 ");
                   System.exit(-1);
               }
           }
        } catch (IOException ioe) {
               logger.error("Problem loading application properties", ioe);
               System.err.println("Problem loading application properties");
               System.exit(-1);
        } catch (Exception e) {
               logger.error("Error in date format, expecting DD/MM/YYYY, got: " + indata);
               e.printStackTrace();
               System.out.println("Error in date format, expecting DD/MM/YYYY, got: " + indata);
               System.exit(-1);
        }

        try {
               logger.info("Using parameters: " + indata);

               TimeZone.setDefault(TimeZone.getTimeZone("Etc/GMT-12"));
               
               // There appears to be some data validation issue at the Telstra Site
               // if pretending to be IE8.  IE7 and FF3.6 work Ok.
               if (getProxyhost() != null && getProxyport() != null) {
                   logger.info("Using Proxy: " + getProxyhost() + ":" + getProxyport());
                   //webclient = new WebClient(BrowserVersion.INTERNET_EXPLORER_8, getProxyhost(), getProxyport());
                   webclient = new WebClient(BrowserVersion.FIREFOX_3_6, getProxyhost(), getProxyport());
               } else {
                   logger.info("Not Using Proxy.");
                   //webclient = new WebClient(BrowserVersion.INTERNET_EXPLORER_7);
                   webclient = new WebClient(BrowserVersion.FIREFOX_3_6);
                  // webclient = new WebClient(BrowserVersion.INTERNET_EXPLORER_7);
               }
               File keyStore = new File( indata.keystore);
               URL certurl = keyStore.toURL();
            
            //   webclient.setSSLClientCertificate(url, "testing", "JKS");
               webclient.getOptions().setTimeout(getTimeout().intValue());
               webclient.getOptions().setThrowExceptionOnScriptError(false); // ignore javascript errors
            // webclient.setRedirectEnabled(true);
               webclient.getOptions().setPopupBlockerEnabled(false);
               webclient.getOptions().setSSLClientCertificate(certurl, indata.keystorepwd, "JKS");
               webclient.setAttachmentHandler(new CollectingAttachmentHandler(attachments));
               webclient.setAlertHandler(this);
            /*
            // Now look for PopUps
             webclient.addWebWindowListener( new WebWindowListener() {
                public void webWindowClosed(WebWindowEvent event) {
                    System.out.println("a window is CLOSED: " +  event.getOldPage());
                    windows.add(event.getWebWindow());
                }
                public void webWindowContentChanged(WebWindowEvent event) {
                    System.out.println("Window content change: " + event.getNewPage());

                    try {
                        handlePopUps(event.getWebWindow().getEnclosedPage());
                        windows.add(event.getWebWindow());
                    } catch (IOException ioe) {
                        logger.error("IOException in PopPup handler");
                    }
                }
                public void webWindowOpened(WebWindowEvent event) {
                        System.out.println("a NEW window opened: " + event.getNewPage());

                }
                });
*/

            currentPage = webclient.getPage(targetpage);
            if (checkAvailable(currentPage) == false)
                System.out.println("Site unavailable");

            currentPage = handlePage01(currentPage, indata);
            if (indata.dogenerate==true) {
                currentPage = handlePage02(currentPage, indata);
                currentPage = handlePage03(currentPage, indata);
            } else if (indata.dofetch==true) {
                currentPage = handlePage04(currentPage, indata);
                currentPage = handlePage05(currentPage, indata);
            }

        } catch (ElementNotFoundException enf) {
            logger.error("Problem finding Link", enf);
        } catch (IOException ioe) {
            logger.error("Problem finding clicking button", ioe);
        }

        if (verbose==true) System.out.println("Done");
    }

    // Javascript Alerts go here
    public void handleAlert(Page page, String message) 
    {
        logger.error(String.format("Alert caught in: %s = %s",page.getUrl(),message));
    }

    private static void usage()
    {
         /* command line options */
         System.err.println("linxfault [-v] [--keystore=fname [--keystorepwd=passwd]] --fromdate=DD/MM/YYYY --todate=DD/MM/YYYY ( --fetch | --generate) ");
         System.err.println("\tRetrieve the Telstra LOLO Linx open Fault reports.");
         System.err.println("\t--fromdate and --todate are the inclusive date range for the report.");
         System.err.println("\t--generate , Generate report for this cert.");
         System.err.println("\t--fetch, Get all the generated reports for this cert.");
         System.err.println("\t--keystore , Keystore file containing single cert, default is from properties.");
         System.err.println("\t--keystorepwd , Keystore password.");
         System.err.println("\t--verbose , Verbose output");
    }

    private void handleAttachment(HtmlPage page) throws IOException, ElementNotFoundException {

    }

    private void handlePopUps(Page page) throws IOException, ElementNotFoundException {
       // HtmlPage currentPage = null;

        if (page == null) {
            System.out.println("PopUp: Page is null");
            return;
        }

        if (page.getClass().isInstance(HtmlPage.class)==true) {
            HtmlPage hpage = (HtmlPage) page;
            logger.info("PopUp start Title:" + hpage.getTitleText());
            String fname = "popup" + (popupix++).toString() + ".txt";
            if (debuglevel > 1) write2file(fname, hpage.asXml());
        } else if (page.getClass().isInstance(HtmlPage.class)==true) {
            TextPage tpage = (TextPage) page;
            logger.info("PopUp start Title:" + tpage.toString());
            String fname = "popup" + (popupix++).toString() + ".txt";
            if (debuglevel > 1) write2file(fname, tpage.toString());
        }

        return;

    }


    private HtmlPage handlePage01(HtmlPage page, Indata indata) throws IOException, ElementNotFoundException {
        HtmlPage currentPage = null;
        logger.info("Page 01 Start: " + getTargetpage());

        if (page == null)
            System.out.println("Page01: Page is null");
        else
            logger.info("Page01: " + page.getUrl());

        if (debuglevel > 1)
            write2file("page01.txt", page.asXml());

        dumpAnchors(page);

        currentPage = page.getAnchorByHref("/wss/home/queryReport").click();

        logger.info("Page 01 Response: " + getResponse(currentPage));

        return(currentPage);

    }

    private HtmlPage handlePage02(HtmlPage page, Indata indata) throws IOException, ElementNotFoundException {
        HtmlPage currentPage = null;

        if (page == null)
            System.out.println("Page02: Page is null");
        else
            logger.info("Page02: " + page.getUrl());
        logger.info("Page 02 Start: " + page.getTitleText());
        logger.info("Page 02 indata: " + indata);

        if (debuglevel > 1)
            write2file("page02.txt", page.asXml());

        HtmlForm form = page.getFormByName("QueryReport");
        dumpAnchors(page);

        // Enter the "Dates"
        form.getInputByName("firstdate").setValueAttribute(indata.fromdate);  // 01/09/2011
        form.getInputByName("seconddate").setValueAttribute(indata.todate);  // 30/09/2011

        // Checkbox input
        //HtmlCheckBoxInput checkbox = page.getElementByName("REPORT_ACTIVE");
        HtmlCheckBoxInput checkbox = page.getElementByName("REPORT_ALL");
        checkbox.setChecked(true);
        
        //Submit button
        currentPage = form.getInputByName("I3").click();
        // Workaround: create a 'fake' button and add it to the form.
        //form.getInputByName("seconddate").click();
       // HtmlButton submitButton = (HtmlButton) page.createElement("button");
       // submitButton.setAttribute("type", "submit");
       // form.appendChild(submitButton);
        //currentPage = submitButton.click();
        
        return(currentPage);

    }

    private HtmlPage handlePage03(HtmlPage page, Indata indata) throws IOException, ElementNotFoundException {
        HtmlPage currentPage = null;

        if (page == null)
            System.out.println("Page03: Page is null");
        else
            logger.info("Page03: " + page.getUrl());
        logger.info("Page 03 Start: " + page.getTitleText());

        if (debuglevel > 1)
            write2file("page03.txt", page.asXml());

        dumpAnchors(page);
       // dumpTable(page);

        HtmlAnchor anchor01 = page.getAnchorByHref("/wss/home/returnHome");
        currentPage = anchor01.click();

        logger.info("Page 03 Response:" + getResponse(currentPage));

        return(currentPage);
    }

    // Go back to back into query Reports to get the generated reports(s)
    private HtmlPage handlePage04(HtmlPage page, Indata indata) throws IOException, ElementNotFoundException {
        HtmlPage currentPage = null;

        if (page == null)
            System.out.println("Page04: Page is null");
        else
            logger.info("Page04: " + page.getUrl());
        
        logger.info("Page 04 Start: " + page.getTitleText());

        if (debuglevel > 1)
            write2file("page04.txt", page.asXml());

        dumpAnchors(page);

        HtmlAnchor anchor01 = page.getAnchorByHref("/wss/home/displayReport");
        //HtmlAnchor anchor01 = page.getAnchorByHref("/wss/home/queryReport");
        currentPage = anchor01.click();

        logger.info("Page 04 Response:" + getResponse(currentPage));

        return(currentPage);

    }

    // SHould be an extra button now.
    private HtmlPage handlePage05(HtmlPage page, Indata indata) throws IOException, ElementNotFoundException {
        HtmlPage currentPage = null;

        if (page == null)
            System.out.println("Page05: Page is null");
        else
            logger.info("Page05: " + page.getUrl());
        
        logger.info("Page 05 Start: " + page.getTitleText());

        if (debuglevel > 1)
            write2file("page05.txt", page.asXml());

        dumpAnchors(page);

        // Don't know link names, but know pats.

        List <HtmlAnchor>anchorlist = page.getAnchors();
        for (HtmlAnchor anc : anchorlist) {

            String anchstring = anc.getHrefAttribute();
            logger.info("Page 05 anchor:" +  anchstring );
            if (anchstring.matches(".*\\.csv$")==true ) {

                fetchTarget(anchstring);
            }

           // logger.info("Line=" + anc.getEndLineNumber());
           // logger.info("\threfattrib=" + anc.getHrefAttribute());
           // logger.info("\tText=" + anc.getTextContent());

        }

        HtmlAnchor anchor01 = page.getAnchorByHref("/wss/home/returnHome");
        currentPage = anchor01.click();

        logger.info("Page 05 Response:" + getResponse(currentPage));

        return(currentPage);

    }



  public void webWindowOpened(WebWindowEvent event)
  {
          windows.add(event.getWebWindow());
  }
         private String getResponse(HtmlPage page) {
        if (page != null)
            return (page.getWebResponse().getWebRequest().getUrl().toString());
            //return (page.getWebResponse().getRequestSettings().getUrl().toString());
          //  return("na");
        return (null);
    }

    private boolean checkAvailable(HtmlPage page) throws IOException {
        boolean available = true;

        logger.info("Page CheckAvailable: " + page.getTitleText());

        return true;
        /*
        try {
            //dumpAnchors(page);
            if (page.asXml().contains("XXX TODO ") == true) {
                available = false;
                if (getDebuglevel() > 1)
                    write2file("page02b.txt", page.asXml());
            }

        } catch (ElementNotFoundException enf) {
            logger.error(enf);
        }
        return (available);
        */
    }

    // Read data from a CSV file
    private static Collection<Object[]> readDataFromCSVFile(String filename) throws FileNotFoundException, IOException
    {
        String [] tokens=null;
        //System.out.printf("OPEN: [%s]\n", filename);

        if (filename==null)
            return(null);

        File file = new File(filename);

        FileReader fr = new FileReader(file);
        BufferedReader br = new BufferedReader(fr);

        int line=0;
        ArrayList <Object[]>a = new ArrayList();
        String s;
        do {
            s = br.readLine();
            if (s==null) continue;
            if (s.length()==0) continue;
            if (s.startsWith("#")) continue;
            line++;

            tokens=s.split(",");

            for (int i=0; i<tokens.length;i++) {
                tokens[i]=tokens[i].trim();
                if (tokens[i].equalsIgnoreCase("null")==true)
                    tokens[i]=null;
            }
            a.add(tokens);

        } while(s!=null);

        fr.close();

        return(a);
    }


    // List all link/anchors on a page
    private void dumpAnchors(HtmlPage page) {
        logger.info("---- Dump Anchors ------");

        ArrayList<HtmlAnchor> anchorlist = (ArrayList) page.getAnchors();
        for (HtmlAnchor anc : anchorlist) {
            logger.info("Line=" + anc.getEndLineNumber());
            logger.info("\threfattrib=" + anc.getHrefAttribute());
            logger.info("\tText=" + anc.getTextContent());

        }
    }

    private void dumpMap(String tag, HtmlElement elem) {
        Map<String, DomAttr> items = elem.getAttributesMap();
        logger.info("---- Dump Attribute Map ------");
        for (String key : items.keySet()) {
            logger.info(tag + "key=" + key + ", val=" + items.get(key));
        }
    }

    private void dumpTable(HtmlPage page) {
        ArrayList<DomNode> tables = (ArrayList) page.getByXPath("//table");

        logger.info("---- Dump Tables ------");
        for (DomNode node : tables) {
            HtmlTable table = (HtmlTable) node;
            dumpMap(" table:", table);
            for (HtmlTableRow row : table.getRows()) {
                dumpMap(" row:", row);
                for (HtmlTableCell cell : row.getCells()) {
                    dumpMap(" cell:", cell);
                }
            }
        }
    }

    // pull out some runtime vars from application.properties
    private void setConfig(Indata indata) throws IOException {


        setTargetpage(powertel.common.ApplicationProperties.getInstance().getProperty("LinxFaults.scraper.targetpage", "http://www.telstrawholesale.com.au/misc/redirect.cfm?rid=/linxonline/ordering.htm"));
        setTargetpagetitle(powertel.common.ApplicationProperties.getInstance().getProperty("LinxFaults.scraper.targetpagetitle", "No Target page title defined"));

        setProxyhost(powertel.common.ApplicationProperties.getInstance().getProperty("LinxFaults.scraper.proxyhost", null));
        setProxyport(new Integer(powertel.common.ApplicationProperties.getInstance().getProperty("LinxFaults.scraper.proxyport", "8080")));

        setPagetracedir(powertel.common.ApplicationProperties.getInstance().getProperty("LinxFaults.scraper.pagetracedir", "/tmp"));
        setKeystore(powertel.common.ApplicationProperties.getInstance().getProperty("LinxFaults.scraper.keystore", null));
        setKeystorePassword(powertel.common.ApplicationProperties.getInstance().getProperty("LinxFaults.scraper.keystorePassword", null));

        System.setProperty("javax.net.ssl.keyStore",getKeystore());
        System.setProperty("javax.net.ssl.keyStorePassword",getKeystorePassword());
        indata.keystorepwd=getKeystorePassword();
        
        if (indata.keystore!=null)
            System.setProperty("javax.net.ssl.keyStore",indata.keystore);
        if (indata.keystorepwd!=null)
            System.setProperty("javax.net.ssl.keyStorePassword",indata.keystorepwd);

        if (verbose==true) {
            System.out.println("Target: " + getTargetpage());
            System.out.println("Proxy: " + getProxyhost() + ":" + getProxyport());
            System.out.println("Tracedir: " + getPagetracedir());
            System.out.println("Keystore: " + getKeystore());
        }
     }

    public void setTargetpage(String targetpage) {
        this.targetpage = targetpage;
    }

    public String getTargetpagetitle() {
        return targetpagetitle;
    }
    public void setTargetpagetitle(String targetpagetitle) {
        this.targetpagetitle = targetpagetitle;
    }

    public Integer getDebuglevel() {
        return debuglevel;
    }

    public void setDebuglevel(Integer debuglevel) {
        this.debuglevel = debuglevel;
    }

    public String getProxyhost() {
        return proxyhost;
    }

    public void setProxyhost(String proxyhost) {
        this.proxyhost = proxyhost;
    }

    public Integer getProxyport() {
        return proxyport;
    }

    public void setProxyport(Integer proxyport) {
        this.proxyport = proxyport;
    }

    public void setTimeout(Long timeout) {
        this.timeout = timeout;
    }
    public Long getTimeout() {
        return this.timeout;
    }
    public String getPagetracedir() {
        return pagetracedir;
    }

    public void setPagetracedir(String pagetracedir) {
        this.pagetracedir = pagetracedir;
    }
    public String getTargetpage() {
        return targetpage;
    }
    public String getKeystore() {
        return keystore;
    }

    public void setKeystore(String keystore) {
        this.keystore = keystore;
    }

    public String getKeystorePassword() {
        return keystorePassword;
    }

    public void setKeystorePassword(String keystorePassword) {
        this.keystorePassword = keystorePassword;
    }

    private void write2file(String fname, String text) {
        String path = getPagetracedir() + "/" + fname;
        write2file(path, text, true);
    }


    private void write2file(String path, String text, boolean isfullpath) {
        try {
            FileWriter file = new FileWriter(path, false);
            file.write(text);
            file.close();
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
    }

    private void fetchTarget(String anchor)
    {
        logger.info("fetchTarget: " + anchor);
        try {
            Page x = webclient.getPage(anchor);
            InputStream is = x.getWebResponse().getContentAsStream();

            String[] parts = anchor.split("/");

            String path=null;
            path = getPagetracedir() + "/" + parts[parts.length-1];

            logger.info("save as: " + path);

            OutputStream out = new FileOutputStream(new File(path));

            int read=0;
            byte[] bytes = new byte[1024];

            while((read = is.read(bytes))!= -1){
                out.write(bytes, 0, read);
            }

            is.close();
            out.flush();
            out.close();

        } catch (Exception e ) { //MalformedURLException e) {
            e.printStackTrace();
        }

    }
}
