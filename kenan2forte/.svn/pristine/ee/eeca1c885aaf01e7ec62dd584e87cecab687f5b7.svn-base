package au.com.aapt.forte;

import java.util.Date;


public class ForteAddress extends Table
{

    private String addressid=null;
    private int clientid=0;
    private String addresstype=null;
    private int isdeleted=0;
    private Date modifydate=null;
    private String modifyuser="Auto";
    private String concurrencyvalue=null;
    private String detail=null;
    private String suburb=null;
    private String state=null;
    private String postcode=null;

    private boolean doInsert=false;
    private boolean doUpdate=false;

    private static String TABLE="cust.vforge_address";

    public void update(DBConnect db) throws K2FException
    {
        String query=null;
        String squery=
                String.format("select count(*) from %s where "+
                      " clientid=%d and addresstype='%s'"
                      ,TABLE
                      ,getClientid()
                      ,getAddresstype());

        String upquery=
               String.format("update %s " +
                            " set detail='%s', " +
                            " suburb='%s', " +
                            " state='%s', " +
                            " postcode='%s' where " +
                            " clientid=%d and addresstype='%s' "
                           , TABLE
                           ,escquote(getDetail())
                           ,escquote(getSuburb())
                           ,getState()
                           ,getPostcode()
                           ,getClientid()
                           ,getAddresstype());


        String inquery =
            String.format("insert into %s " +
                     "(addressid, clientid, addresstype, isdeleted, detail, " +
                     " suburb,state, postcode, modifydate, concurrencyvalue) values " +
                     "('%s',%d,'%s',0,'%s','%s','%s','%s',sysdate,'%s' )"
                         ,TABLE
                         ,getAddressid()
                         ,getClientid()
                         ,getAddresstype()
                         ,escquote(getDetail())
                         ,escquote(getSuburb())
                         ,getState()
                         ,getPostcode()
                         ,getConcurrencyvalue());

        if (AppProps.doInsert==false)
            return;

        runQueries(db, squery, upquery, inquery);

    }

    public String getAddressid() {
        return addressid;
    }

    public void setAddressid(String addressid) {
        this.addressid = addressid;
    }

    public int getClientid() {
        return clientid;
    }

    public void setClientid(int clientid) {
        this.clientid = clientid;
    }

    public String getAddresstype() {
        return addresstype;
    }

    public void setAddresstype(String addresstype) {
        this.addresstype = addresstype;
    }

    public int getIsdeleted() {
        return isdeleted;
    }

    public void setIsdeleted(int isdeleted) {
        this.isdeleted = isdeleted;
    }

    public Date getModifydate() {
        return modifydate;
    }

    public void setModifydate(Date modifydate) {
        this.modifydate = modifydate;
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

    public String getDetail() {
        return detail;
    }

    public void setDetail(String detail) {
        this.detail = detail;
    }

    public String getSuburb() {
        return suburb;
    }

    public void setSuburb(String suburb) {
        this.suburb = suburb;
    }

    public String getState() {
        return state;
    }

    public void setState(String state) {
        this.state = state;
    }

    public String getPostcode() {
        return postcode;
    }

    public void setPostcode(String postcode) {
        this.postcode = postcode;
    }


}
