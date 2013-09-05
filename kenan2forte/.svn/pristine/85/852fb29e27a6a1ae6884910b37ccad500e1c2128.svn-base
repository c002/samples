package au.com.aapt.forte;

import java.util.Date;

public class ForteCustomerDetails extends Table {

  //  static private ResultSet results=null;
  //  static private Statement stmt = null;

    private String customerdetailsId=null;
    private int fkcustomerid=0;
    private String pathtomarket=null;
    private String partner=null;
    private Date modifyDate=null;
    private String modifyUser=null;
    private String taxclass=null;
    private String concurrencyvalue=null;
    private int isdeleted=0;

    private boolean doInsert=false;
    private boolean doUpdate=false;
    private static String TABLE="customerdetails";

    public ForteCustomerDetails() throws K2FException
    {
        super();
    }

    public void update(DBConnect db) throws K2FException
    {
        String query=null;
        String squery=
                String.format("select count(*) from %s where " +
                        "fkcustomerid='%s'"
                        , TABLE
                        ,String.valueOf(getFkcustomerid()));    // id is a string!

        String inquery=
            String.format("insert into %s " +
                    " (customerdetailsId, concurrencyvalue, fkcustomerid, " +
                    " pathtomarket,partner, modifydate, modifyuser,taxclass, isdeleted) values " +
                    " ('%s', '%s','%s' ,'%s','%s',sysdate,'%s','%s',%d)"
                    , TABLE
                    ,getCustomerdetailsId()         // 13 chars long
                    ,getConcurrencyvalue()
                    ,String.valueOf(getFkcustomerid())    // Yep its a string here.
                    ,getPathtomarket()
                    ,getPartner()
                    ,getModifyUser()
                    ,getTaxclass()
                    ,getIsdeleted());

        if (AppProps.doInsert==false)
            return;

        runQueries(db, squery, null, inquery);
        
    }

    public String getCustomerdetailsId() {
        return customerdetailsId;
    }

    public void setCustomerdetailsId(String customerdetailsId) {
        this.customerdetailsId = customerdetailsId;
    }

    public String getPathtomarket() {
        return pathtomarket;
    }

    public void setPathtomarket(String pathtomarket) {
        this.pathtomarket = pathtomarket;
    }

    public int getFkcustomerid() {
        return fkcustomerid;
    }

    public void setFkcustomerid(int fkcustomerid) {
        this.fkcustomerid = fkcustomerid;
    }

    public String getPartner() {
        return partner;
    }

    public void setPartner(String partner) {
        this.partner = partner;
    }

    public Date getModifyDate() {
        return modifyDate;
    }

    public void setModifyDate(Date modifyDate) {
        this.modifyDate = modifyDate;
    }

    public String getModifyUser() {
        return modifyUser;
    }

    public void setModifyUser(String modifyUser) {
        this.modifyUser = modifyUser;
    }

    public String getTaxclass() {
        return taxclass;
    }

    public void setTaxclass(String taxclass) {
        this.taxclass = taxclass;
    }

    public String getConcurrencyvalue() {
        return concurrencyvalue;
    }

    public void setConcurrencyvalue(String concurrencyvalue) {
        this.concurrencyvalue = concurrencyvalue;
    }

    public int getIsdeleted() {
        return isdeleted;
    }

    public void setIsdeleted(int isdeleted) {
        this.isdeleted = isdeleted;
    }



}
