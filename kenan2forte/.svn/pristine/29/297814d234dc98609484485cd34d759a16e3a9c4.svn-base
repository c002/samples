package au.com.aapt.forte;

import java.util.Date;

public class ForteCustomer extends Table {

  //  static private ResultSet results=null;
  //  static private Statement stmt = null;


    private int customerId=0;

    private String name=null;
    private Date modifyDate=null;
    private String modifyuser=null;
    private String concurrencyvalue=null;
    private String accountManager=null;
    private String category=null;
    private String status=null;
    private int isdeleted=0;

    private boolean doInsert=false;
    private boolean doUpdate=false;

    private static String TABLE="orders_f.customer";

    public ForteCustomer() throws K2FException
    {
        super();
    }

    public void update(DBConnect db) throws K2FException
    {
        String query=null;
        String squery=
            String.format("select count(*) from %s where " +
                    "customerid=%d"
                    ,TABLE
                    ,getCustomerId());


        String upquery=
            String.format("update %s " +
                            "set modifydate=sysdate, " +
                            "name='%s' " +
                            "where customerid=%d"
                        ,TABLE
                        ,getName()
                        ,getCustomerId());


        String inquery=
            String.format("insert into %s " +
                          "(customerid, concurrencyvalue, category, modifydate," +
                          " modifyuser, name, status, accountmanager, isdeleted) values " +
                          " (%d,'%s','%s',sysdate,'%s','%s','%s','%s',%d) "
                            ,TABLE
                            ,getCustomerId()
                            ,getConcurrencyvalue()
                            ,getCategory()
                            ,getModifyuser()
                            ,getName()
                            ,getStatus()
                            ,getAccountManager()
                            ,getIsdeleted());
                            
        if (AppProps.doInsert==false)
            return;
        
        runQueries(db, squery, upquery, inquery);
 
    }

    public int getCustomerId() {
        return customerId;
    }

    public void setCustomerId(int customerId) {
        this.customerId = customerId;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Date getModifyDate() {
        return modifyDate;
    }

    public void setModifyDate(Date modifyDate) {
        this.modifyDate = modifyDate;
    }

    public String getModifyuser() {
        return modifyuser;
    }

    public void setModifyuser(String modifyuser) {
        this.modifyuser = modifyuser;
    }

    public String getConcurrencyvalue() {
        return concurrencyvalue;
    }

    public void setConcurrencyvalue(String concurrencyvalue) {
        this.concurrencyvalue = concurrencyvalue;
    }

    public String getAccountManager() {
        return accountManager;
    }

    public void setAccountManager(String accountManager) {
        this.accountManager = accountManager;
    }

    public String getCategory() {
        return category;
    }

    public void setCategory(String category) {
        this.category = category;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public int getIsdeleted() {
        return isdeleted;
    }

    public void setIsdeleted(int isdeleted) {
        this.isdeleted = isdeleted;
    }

}
