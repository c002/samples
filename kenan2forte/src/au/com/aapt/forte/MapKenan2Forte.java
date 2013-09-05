package au.com.aapt.forte;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;

import org.apache.log4j.Logger;

public class MapKenan2Forte {

    ArrayList <KenanRecord> kenanRecordList = new ArrayList<KenanRecord>();
    ArrayList <ForteRecord> forteRecordList = new ArrayList<ForteRecord>();

    protected static final Logger log = Logger.getLogger("au.com.aapt.forte.MapKenan2Forte");

    public MapKenan2Forte(ArrayList <KenanRecord> kenanRecordList) throws K2FException
    {

        for (KenanRecord kr : kenanRecordList) {
            Date nowdate = new Date();
            String concvalbig = whackyForteConcval(nowdate);
            String concval = concvalbig.substring(0,8);

            if (AppProps.debuglevel>1)
                System.out.println(kr.toString());

            ForteRecord fr = new ForteRecord();

            /* vforge_client */
            fr.getFclient().setConcurrencyvalue(concval);
            fr.getFclient().setClientid(mkForteCustId(kr.getAccountNumber()));
            fr.getFclient().setSalesforceId(maxLen(kr.getSalesforceId(),10,true,"AN99999999"));
            fr.getFclient().setCreationdate(fr.str2Date(kr.getActiveDate()));
            fr.getFclient().setModifydate(nowdate);
           // fr.getFclient().setStatus(statusMap(kr.getAccountStatus()));
            fr.getFclient().setStatus("NEW");
            fr.getFclient().setAccountManager("unmanaged");
            fr.getFclient().setRouteType("standard");
            fr.getFclient().setCustCareRep("Fulfilment");
            fr.getFclient().setCategory(categoryMap(kr.getAccountCategory()));
            fr.getFclient().setName(maxLen(kr.getCompanyName(),150,false, kr.summary()));
            fr.getFclient().setSalesrep("unmanaged");
            fr.getFclient().setModifyUser("Auto");
            fr.getFclient().setPaymentterm(14);

            /* orders_f.customer*/
            fr.getFcustomer().setAccountManager("Unmanaged");
            fr.getFcustomer().setIsdeleted(0);
            fr.getFcustomer().setCategory(categoryMap(kr.getAccountCategory()));
            fr.getFcustomer().setConcurrencyvalue(concval);
            fr.getFcustomer().setCustomerId(mkForteCustId(kr.getAccountNumber()));
            fr.getFcustomer().setModifyDate(nowdate);
            fr.getFcustomer().setModifyuser("Auto");
            fr.getFcustomer().setName(maxLen(kr.getCompanyName(),150,false, kr.summary()));
            fr.getFcustomer().setStatus("New");

            /* vforge_contact */
            fr.getFcontact().setClientId(mkForteCustId(kr.getAccountNumber()));
            fr.getFcontact().setConcurrencyvalue(concval);
            fr.getFcontact().setContactId(whackyForteId(concvalbig,kr.getRecordNumber()));
            fr.getFcontact().setTitle(maxLen(kr.getSalutation(),5,true,null));
            fr.getFcontact().setRole("Invoice");
            fr.getFcontact().setFirstName(maxLen(kr.getBillFname(),20,true,null));  // Field 10
            fr.getFcontact().setLastName(maxLen(kr.getBillLname(),40,true,null));
            fr.getFcontact().setWorkphone("0000000000");
            fr.getFcontact().setModifyDate(fr.str2Date(kr.getChangeDate()));

            /* vforge_address - Invoice Address */
            fr.getFaddrinvoice().setClientid(mkForteCustId(kr.getAccountNumber()));
            fr.getFaddrinvoice().setConcurrencyvalue(concval);
            fr.getFaddrinvoice().setAddressid(whackyForteId(concvalbig,kr.getRecordNumber(),1));
            fr.getFaddrinvoice().setModifyuser("Auto");
            fr.getFaddrinvoice().setModifydate(nowdate);
            fr.getFaddrinvoice().setAddresstype("Invoice");
            fr.getFaddrinvoice().setSuburb(maxLen(kr.getBillSuburb(),50,true,null));
            fr.getFaddrinvoice().setState(maxLen(kr.getBillState(),4,true,null));
            fr.getFaddrinvoice().setPostcode(maxLen(kr.getBillPostCode(),6,true,null));
            fr.getFaddrinvoice().setDetail(maxLen(mkAddress(kr.getBillAddress1(),
                                                            kr.getBillAddress2(),
                                                            kr.getBillAddress3()),100,true,null));
          
            /* vforge_address - Street Address */
            if (kr.getCustAddress1()!=null && kr.getCustAddress1().trim().length()>0) {
                fr.getFaddrstreet().setClientid(mkForteCustId(kr.getAccountNumber()));
                fr.getFaddrstreet().setAddressid(whackyForteId(concvalbig,kr.getRecordNumber(),2));
                fr.getFaddrstreet().setConcurrencyvalue(concval);
                fr.getFaddrstreet().setModifyuser("Auto");
                fr.getFaddrstreet().setModifydate(nowdate);
                fr.getFaddrstreet().setAddresstype("Street");
                fr.getFaddrstreet().setSuburb(maxLen(kr.getCustSuburb(),50,true,null));
                fr.getFaddrstreet().setState(maxLen(kr.getCustState(),4,true,null));
                fr.getFaddrstreet().setPostcode(maxLen(kr.getCustPostCode(),6,true,null));
                fr.getFaddrstreet().setDetail(maxLen(mkAddress(kr.getCustAddress1(),
                                                    kr.getCustAddress2(),
                                                    kr.getCustAddress3()),100,true,null));
                /* vforge_address - Postal Address */
                fr.getFaddrpostal().setClientid(mkForteCustId(kr.getAccountNumber()));
                fr.getFaddrpostal().setAddressid(whackyForteId(concvalbig,kr.getRecordNumber(),3));
                fr.getFaddrpostal().setConcurrencyvalue(concval);
                fr.getFaddrpostal().setModifyuser("Auto");
                fr.getFaddrpostal().setModifydate(nowdate);
                fr.getFaddrpostal().setAddresstype("Postal");
                fr.getFaddrpostal().setSuburb(maxLen(kr.getCustSuburb(),50,true,null));
                fr.getFaddrpostal().setState(maxLen(kr.getCustState(),4,true,null));
                fr.getFaddrpostal().setPostcode(maxLen(kr.getCustPostCode(),6,true,null));
                fr.getFaddrpostal().setDetail(maxLen(mkAddress(kr.getCustAddress1(),
                                                           kr.getCustAddress2(),
                                                           kr.getCustAddress3()),100,true,null));
            } else {	// No address info, use blling address info
            	 fr.getFaddrstreet().setClientid(fr.getFaddrinvoice().getClientid());
            	 fr.getFaddrstreet().setAddressid(whackyForteId(concvalbig,kr.getRecordNumber(),2));
            	 fr.getFaddrstreet().setConcurrencyvalue(fr.getFaddrinvoice().getConcurrencyvalue());
            	 fr.getFaddrstreet().setModifyuser(fr.getFaddrinvoice().getModifyuser());
            	 fr.getFaddrstreet().setModifydate(nowdate);
            	 fr.getFaddrstreet().setAddresstype("Street");
            	 fr.getFaddrstreet().setSuburb(fr.getFaddrinvoice().getSuburb());
            	 fr.getFaddrstreet().setState(fr.getFaddrinvoice().getState());
            	 fr.getFaddrstreet().setPostcode(fr.getFaddrinvoice().getPostcode());
            	 fr.getFaddrstreet().setDetail(fr.getFaddrinvoice().getDetail());
   
              	 fr.getFaddrpostal().setClientid(fr.getFaddrinvoice().getClientid());
            	 fr.getFaddrpostal().setAddressid(whackyForteId(concvalbig,kr.getRecordNumber(),3));
            	 fr.getFaddrpostal().setConcurrencyvalue(fr.getFaddrinvoice().getConcurrencyvalue());
            	 fr.getFaddrpostal().setModifyuser(fr.getFaddrinvoice().getModifyuser());
            	 fr.getFaddrpostal().setModifydate(nowdate);
            	 fr.getFaddrpostal().setAddresstype("Postal");
            	 fr.getFaddrpostal().setSuburb(fr.getFaddrinvoice().getSuburb());
            	 fr.getFaddrpostal().setState(fr.getFaddrinvoice().getState());
            	 fr.getFaddrpostal().setPostcode(fr.getFaddrinvoice().getPostcode());
            	 fr.getFaddrpostal().setDetail(fr.getFaddrinvoice().getDetail());
            }
                
             /* vforge_company */
            fr.getFcompany().setClientId(mkForteCustId(kr.getAccountNumber()));
            fr.getFcompany().setCompanyId(whackyForteId(concvalbig,kr.getRecordNumber()));
            fr.getFcompany().setAbn("22052082416");
            fr.getFcompany().setType("Company");
            fr.getFcompany().setConcurrencyvalue(concval);
            fr.getFcompany().setModifyDate(nowdate);
            fr.getFcompany().setModifyUser("Auto");

            /* customerdetails */
            fr.getFcustomerdetails().setFkcustomerid(mkForteCustId(kr.getAccountNumber()));
            fr.getFcustomerdetails().setConcurrencyvalue(concval);
            fr.getFcustomerdetails().setCustomerdetailsId(whackyForteId(concvalbig,kr.getRecordNumber()));
            fr.getFcustomerdetails().setModifyDate(nowdate);
            fr.getFcustomerdetails().setIsdeleted(0);
            fr.getFcustomerdetails().setModifyUser("Auto");
            fr.getFcustomerdetails().setTaxclass("GST Applicable");
            fr.getFcustomerdetails().setPathtomarket("Direct");
            fr.getFcustomerdetails().setPartner("900000");
            if (fr.getFclient().getCategory().compareTo("ISP")==0) {
                    fr.getFcustomerdetails().setPathtomarket("Wholesale");
            }

            /* master_group_link */
            fr.getFmastergrouplink().setMastergroupId(39);
            fr.getFmastergrouplink().setClientid(mkForteCustId(kr.getAccountNumber()));
            fr.getFmastergrouplink().setLastUpdateBy("Auto");
            fr.getFmastergrouplink().setLastUpdateDate(nowdate);
            fr.getFmastergrouplink().setFromdate(nowdate);

            if (AppProps.debuglevel>1) {
                System.out.println(fr.toString());
            }
            forteRecordList.add(fr);
        }

    }
    /**
    // id is the default value if null is Ok
    // id is an exception msg if null is not Ok.

     * @param str   String to check
     * @param maxlen   Max permitted length of String
     * @param nullOk   Is Null Ok for String?
     * @param id       Default value, if null is Ok , Id of record if not.
     * @return
     * @throws K2FException
     **/
    private String maxLen(String str, int maxlen, boolean nullOk, String id) throws K2FException
    {

        if (str==null && nullOk) {
            if (id!=null) return (id);
            return null;
        }
        if (str==null) throw new K2FException("Field is not permitted to be null in record:" + id);

        if (str.length()>maxlen) {
            log.warn("Data length exceeded maxlen, truncated:" + id);
            return(str.substring(0, maxlen-1));
        }
        return(str);
    }

    private String mkAddress(String s1, String s2, String s3)
    {
        StringBuilder address= new StringBuilder();
        if (s1==null && s2==null && s3==null) return null;
        if (s1!=null && s1.length()>0)
            address.append(s1);
        if (s2!=null && s2.length()>0) {
            if (s1!=null && s1.length()>0) address.append(",");
            address.append(s2);
        } if (s3!=null && s3.length()>0) {
            if ((s1!=null && s1.length()>0) || (s2!=null && s2.length()>0)) address.append(",");
            address.append(s3);
        }

        return address.toString();
    }

    private int mkForteCustId(int account) throws K2FException
    {
        if (account>2000000000)
            return( account - 2000000000 + 200000);
        else
            return(account);
    }

    private String statusMap(String kstatus)
    {
        String fstatus=null;
        if (kstatus==null) return(null);
        if (kstatus.compareToIgnoreCase("Disc Done")==0)
            fstatus="Cancelled";
        else if (kstatus.compareToIgnoreCase("Disc Req")==0)
            fstatus="Suspended";
        else if (kstatus.compareToIgnoreCase("Active")==0)
            fstatus="Active";

        return(fstatus);
    }

    /* 8 digit concurrency value is calculated as:
       sysdate - 1-jan-1998' , most sign 8 digits.
       I am includeing milliseconds also.  This is truncated to 8 for
       concval but used to create unique id's(+recordnumber)
    */
    protected String whackyForteConcval(Date d)
    {
        if (d==null) return null;
        Date d2;
        long old_epoch=0;

        Calendar cal = new GregorianCalendar();
        cal.setTime(d);
        long now_epoch = cal.getTimeInMillis();

        //parseDate(String datestr)
        DateFormat df = new SimpleDateFormat("yyyyMMdd");
        try {
            d2 = df.parse("19980101");
            cal.setTime(d2);
            old_epoch = cal.getTimeInMillis();
        } catch (Exception e) {
        }
        String epoch_str = String.valueOf(( now_epoch - old_epoch));
        String concval = epoch_str.substring(epoch_str.length()-11, epoch_str.length());

        return(concval);
    }

    // This to get unique Id's for Forte
    protected String whackyForteId(String concval, int recnum, int instance)
    {
        String whackyid = concval + String.format("%01d%01d", recnum%10, instance);
        return(whackyid);
    }

    protected String whackyForteId(String concval, int recnum)
    {
        String whackyid = concval + String.format("%02d", recnum%100);
        return(whackyid);
    }

    private String categoryMap(String kcat)
    {
        String fcat=null;

        if (kcat==null) return null;

        if (kcat.compareToIgnoreCase("WHOLESALE")==0)
            fcat="ISP";
        if (kcat.compareToIgnoreCase("ABS")==0)
            fcat="Corporate";
        if (kcat.compareToIgnoreCase("INTERNAL")==0 ||
            kcat.compareToIgnoreCase("INTENAL")==0)
                fcat="Non-Billable";
        return(fcat);
    }

    public ArrayList<KenanRecord> getKenanRecordList() {
        return kenanRecordList;
    }

    public void setKenanRecordList(ArrayList<KenanRecord> kenanRecordList) {
        this.kenanRecordList = kenanRecordList;
    }

    public ArrayList<ForteRecord> getForteRecordList() {
        return forteRecordList;
    }

    public void setForteRecordList(ArrayList<ForteRecord> forteRecordList) {
        this.forteRecordList = forteRecordList;
    }


}
