package au.com.aapt.forte;

import java.util.Date;

public class ForteClient extends Table {

    private int clientid=0;
    private int isdeleted=0;
    private Date modifydate=null;
    private String concurrencyvalue=null;
    private String name=null;
    private String status=null;
    private String Category=null;
    private Date creationdate=null;
    private String routeType=null;
    private String salesforceId=null;
    private String salesrep=null;
    private String accountManager=null;
    private String accountStatus=null;
    private String custCareRep=null;
    private String modifyUser=null;
    private int paymentterm=14;

    private boolean doInsert=false;
    private boolean doUpdate=false;
    private static String TABLE="cust.vforge_client";


    public String getAccountManager() {
        return accountManager;
    }

    public void setAccountManager(String accountManager) {
        this.accountManager = accountManager;
    }

    public String getAccountStatus() {
        return accountStatus;
    }

    public void setAccountStatus(String accountStatus) {
        this.accountStatus = accountStatus;
    }

    public String getCustCareRep() {
        return custCareRep;
    }

    public void setCustCareRep(String custCareRep) {
        this.custCareRep = custCareRep;
    }

    public String getModifyUser() {
        return modifyUser;
    }

    public void setModifyUser(String modifyUser) {
        this.modifyUser = modifyUser;
    }


    public ForteClient() throws K2FException
    {
        super();
    }

    public void update(DBConnect db) throws K2FException
    {
            String query=null;
            String squery=
                String.format("select count(*) from %s where " +
                        "clientid=%d"
                        ,TABLE
                        ,getClientid() );

            String upquery=
                String.format("update %s " +
                                "set salesforceid='%s' where clientid=%d"
                                , TABLE
                                ,getSalesforceId()
                                ,getClientid());

            String inquery=
                String.format("insert into %s " +
                              "(clientid, accountmanager, isdeleted, creationdate, " +
                              " concurrencyvalue, modifyuser, modifydate," +
                              "name,category, route_type,custcarerep,salesrep, status, salesforceid, paymentterm) " +
                              " values (%d,'%s',%d, to_date('%s','YYYYMMDD'), '%s','%s', sysdate," +
                              "'%s','%s','%s','%s', '%s','%s','%s',%d)"
                              ,TABLE
                              ,getClientid()
                              ,getAccountManager()
                              ,getIsdeleted()
                              ,date2str(getCreationdate())
                              ,getConcurrencyvalue()
                              ,getModifyUser()
                              ,getName()
                              ,getCategory()
                              ,getRouteType()
                              ,getCustCareRep()
                              ,getSalesrep()
                              ,getStatus()
                              ,getSalesforceId()
                              ,getPaymentterm());

            log.debug(this.getClass().getName() +", DEBUG SQL: " + squery);
            log.debug(this.getClass().getName() +", DEBUG SQL: " + inquery);
            log.debug(this.getClass().getName() +", DEBUG SQL: " + upquery);

            if (AppProps.doInsert==false)
                return;

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
                } else if (doUpdate==true) {
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

    public String getCategory() {
        return Category;
    }

    public void setCategory(String Category) {
        this.Category = Category;
    }

    public int getClientid() {
        return clientid;
    }

    public void setClientid(int clientid) {
        this.clientid = clientid;
    }

    public int getIsdeleted() {
        return isdeleted;
    }

    public void setIsdeleted(int isdeleted) {
        this.isdeleted = isdeleted;
    }

    public String getConcurrencyvalue() {
        return concurrencyvalue;
    }

    public void setConcurrencyvalue(String concurrencyvalue) {
        this.concurrencyvalue = concurrencyvalue;
    }

    public Date getModifydate() {
        return modifydate;
    }

    public void setModifydate(Date modifydate) {
        this.modifydate = modifydate;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public Date getCreationdate() {
        return creationdate;
    }

    public void setCreationdate(Date creationdate) {
        this.creationdate = creationdate;
    }

    public String getRouteType() {
        return routeType;
    }

    public void setRouteType(String routeType) {
        this.routeType = routeType;
    }

    public String getSalesforceId() {
        return salesforceId;
    }

    public void setSalesforceId(String salesforceId) {
        this.salesforceId = salesforceId;
    }

    public String getSalesrep() {
        return salesrep;
    }

    public void setSalesrep(String salesrep) {
        this.salesrep = salesrep;
    }

    public int getPaymentterm() {
        return paymentterm;
    }

    public void setPaymentterm(int paymentterm) {
        this.paymentterm = paymentterm;
    }

}
