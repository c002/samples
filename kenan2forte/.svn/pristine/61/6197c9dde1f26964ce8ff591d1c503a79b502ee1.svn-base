package au.com.aapt.forte;

import java.util.Date;

public class ForteMasterGroupLink extends Table {

  //  static private ResultSet results=null;
  //  static private Statement stmt = null;

    private int mastergroupId=0;
    private int clientid=0;
    private String lastUpdateBy=null;
    private Date lastUpdateDate=null;
    private Date fromdate=null;

    private boolean doInsert=false;
    private boolean doUpdate=false;
    private static String TABLE="cust_class.master_group_client_link";

    public ForteMasterGroupLink() throws K2FException
    {
        super();
    }

    public void update(DBConnect db) throws K2FException
    {
        String query=null;
        String squery=
            String.format("select count(*) from %s " +
                    "where clientid=%d"
                    ,TABLE
                    ,getClientid());

        String inquery=
            String.format("insert into %s " +
                    " (mastergroupId, clientid, lastupdateby, lastupdatedate, fromdate) values " +
                    " (%d,%d,'%s',sysdate, sysdate)"
                    ,TABLE
                    ,getMastergroupId()
                    ,getClientid()
                    ,getLastUpdateBy());

        if (AppProps.doInsert==false)
            return;

        runQueries(db, squery, null, inquery);
  
    }

    public int getMastergroupId() {
        return mastergroupId;
    }

    public void setMastergroupId(int mastergroupId) {
        this.mastergroupId = mastergroupId;
    }

    public int getClientid() {
        return clientid;
    }

    public void setClientid(int clientid) {
        this.clientid = clientid;
    }

    public String getLastUpdateBy() {
        return lastUpdateBy;
    }

    public void setLastUpdateBy(String lastUpdateBy) {
        this.lastUpdateBy = lastUpdateBy;
    }

    public Date getLastUpdateDate() {
        return lastUpdateDate;
    }

    public void setLastUpdateDate(Date lastUpdateDate) {
        this.lastUpdateDate = lastUpdateDate;
    }

    public Date getFromdate() {
        return fromdate;
    }

    public void setFromdate(Date fromdate) {
        this.fromdate = fromdate;
    }


}
