package au.com.aapt.forte;

import java.util.Date;

public class ForteContact extends Table {

  //  static private ResultSet results=null;
  //  static private Statement stmt = null;

    private String contactId=null;
    private int clientId=0;
    private String role=null;
    private int isDeleted=0;
    private Date modifyDate=null;
    private String title=null;
    private String firstName=null;
    private String lastName=null;
    private String workphone=null;
    private String notes="Imported by kenan2forte";
    private String concurrencyvalue=null;

    private boolean doInsert=false;
    private boolean doUpdate=false;

    private static String TABLE="cust.vforge_contact";

    public ForteContact() throws K2FException
    {
        super();
    }

    public void update(DBConnect db) throws K2FException
    {
        String query=null;
        String squery=
            String.format("select count(*) from %s where " +
                    "clientId=%d and role='%s'"
                    ,TABLE
                    ,getClientId()
                    ,getRole());

        String upquery=
            String.format("update %s " +
                            "set title='%s', " +
                            "firstname='%s', " +
                            "lastname='%s', " +
                            "workphone='%s', " +
                            "notes='%s' " +
                            "where clientid=%d and role='%s'"
                        ,TABLE
                        ,getTitle()
                        ,escquote(getFirstName())
                        ,escquote(getLastName())
                        ,getWorkphone()
                        ,getNotes()
                        ,getClientId()
                        ,getRole());


        String inquery=
            String.format("insert into %s " +
                          "(contactid, clientid, concurrencyvalue, role, modifydate, isdeleted," +
                          " title, firstname, lastname, workphone, notes) values " +
                          " ('%s',%d,'%s','%s',sysdate,0,'%s','%s','%s','%s','%s') "
                            ,TABLE
                            ,getContactId()
                            ,getClientId()
                            ,getConcurrencyvalue()
                            ,getRole()
                            ,getTitle()
                            ,escquote(getFirstName())
                            ,escquote(getLastName())
                            ,getWorkphone()
                            ,getNotes());

        if (AppProps.doInsert==false)
            return;

        runQueries(db, squery, upquery, inquery);

    }

    public String getContactId() {
        return contactId;
    }

    public void setContactId(String contactId) {
        this.contactId = contactId;
    }

    public int getClientId() {
        return clientId;
    }

    public void setClientId(int clientId) {
        this.clientId = clientId;
    }

    public String getRole() {
        return role;
    }

    public void setRole(String role) {
        this.role = role;
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

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getFirstName() {
        return firstName;
    }

    public void setFirstName(String firstName) {
        this.firstName = firstName;
    }

    public String getLastName() {
        return lastName;
    }

    public void setLastName(String lastName) {
        this.lastName = lastName;
    }

    public String getWorkphone() {
        return workphone;
    }

    public void setWorkphone(String workphone) {
        this.workphone = workphone;
    }

    public String getNotes() {
        return notes;
    }

    public void setNotes(String notes) {
        this.notes = notes;
    }

    public String getConcurrencyvalue() {
        return concurrencyvalue;
    }

    public void setConcurrencyvalue(String concurrencyvalue) {
        this.concurrencyvalue = concurrencyvalue;
    }

}
