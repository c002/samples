package au.com.aapt.forte;

import java.util.Date;

public class ForteCompany extends Table {

  //  static private ResultSet results=null;
  //  static private Statement stmt = null;

    private String companyId=null;
    private int clientId=0;
    private int isDeleted=0;
    private Date modifyDate=null;
    private String modifyUser=null;
    private String concurrencyvalue=null;
    private String type=null;
    private String abn=null;

    private static String TABLE="cust.vforge_company";
    private boolean doInsert=false;

    public ForteCompany() throws K2FException
    {
        super();
    }

    public void update(DBConnect db) throws K2FException
    {
        String query=null;
        String squery=
            String.format("select count(*) from %s where clientid=%d"
                        , TABLE
                        ,getClientId());

        String inquery=
            String.format("insert into %s " +
                    " (companyid, clientid, isdeleted, abn, type, modifydate,modifyuser,concurrencyvalue) values " +
                    " (cust.vforge_companyid_s.nextval, %d ,0,'%s','%s',sysdate,'%s', '%s')"
                    , TABLE
                    ,getClientId()
                    ,getAbn()
                    ,getType()
                    ,getModifyUser()
                    ,getConcurrencyvalue());

        if (AppProps.doInsert==false)
            return;

        runQueries(db, squery, null, inquery);

    }

    public String getCompanyId() {
        return companyId;
    }

    public void setCompanyId(String companyId) {
        this.companyId = companyId;
    }

    public int getClientId() {
        return clientId;
    }

    public void setClientId(int clientId) {
        this.clientId = clientId;
    }

    public int getIsDeleted() {
        return isDeleted;
    }

    public void setIsDeleted(int isDeleted) {
        this.isDeleted = isDeleted;
    }

    public Date getModifyDate() {
        return modifyDate;
    }

    public void setModifyDate(Date modifyDate) {
        this.modifyDate = modifyDate;
    }

    public String getConcurrencyvalue() {
        return concurrencyvalue;
    }

    public void setConcurrencyvalue(String concurrencyvalue) {
        this.concurrencyvalue = concurrencyvalue;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getAbn() {
        return abn;
    }

    public void setAbn(String abn) {
        this.abn = abn;
    }

    public String getModifyUser() {
        return modifyUser;
    }

    public void setModifyUser(String modifyUser) {
        this.modifyUser = modifyUser;
    }

}
