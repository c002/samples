package au.com.aapt.forte;

import java.util.Date;

public class ForteMasterGroup extends Table {

  //  static private ResultSet results=null;
  //  static private Statement stmt = null;

    private String mastergroupId=null;
    private String mastergroupName=null;
    private String lastUpdateBy=null;
    private Date lastUpdateDate=null;
    private Date processDate=null;

    private boolean doInsert=false;
    private boolean doUpdate=false;
    private static String TABLE="cust_class.master_group";

    public ForteMasterGroup() throws K2FException
    {
        super();
    }

    public void update(DBConnect db) throws K2FException
    {
        String query=null;
        String squery=
            String.format("select count(*) from %s " +
                    "where mastergroupname='%s'"
                    ,TABLE
                    ,getMastergroupName());

        String inquery=
            String.format("insert into %s " +
                    " (mastergroupId, mastergroupName, lastupdateby, lastupdatedate) values " +
                    " (cust_class.master_group_id_seq.nextval,'%s','%s',sysdate)"
                    ,TABLE
                    ,getMastergroupName()
                    ,getLastUpdateBy());

        if (AppProps.doInsert==false)
            return;

        runQueries(db, squery, null, inquery);

    }

    public String getMastergroupId() {
        return mastergroupId;
    }

    public void setMastergroupId(String mastergroupId) {
        this.mastergroupId = mastergroupId;
    }

    public String getMastergroupName() {
        return mastergroupName;
    }

    public void setMastergroupName(String mastergroupName) {
        this.mastergroupName = mastergroupName;
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

    public Date getProcessDate() {
        return processDate;
    }

    public void setProcessDate(Date processDate) {
        this.processDate = processDate;
    }


}
